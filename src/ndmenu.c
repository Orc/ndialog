/*
 *   Copyright (c) 1996 David Parsons. All rights reserved.
 *   
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *  3. All advertising materials mentioning features or use of this
 *     software must display the following acknowledgement:
 *     
 *   This product includes software developed by David Parsons
 *   (orc@pell.portland.or.us)
 *
 *  4. My name may not be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *     
 *  THIS SOFTWARE IS PROVIDED BY DAVID PARSONS ``AS IS'' AND ANY
 *  EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 *  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 *  PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL DAVID
 *  PARSONS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 *  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 *  TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 *  IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 *  THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * nd_menu:  the MENU() function (ncurses function)
 *
 * running a menu takes several steps.
 * 1) first we scan the chain for all non-button objects, figuring how much
 *    real estate they will consume.
 * 2) we create a window, frame it (either single-frame or double-frame,
 *    depending on how much room we have left), and populate it with all
 *    the non-button objects.
 * 3) if there are button objects, place them on a button bar.
 * 4) if there are active fields, process them, and
 * 4a) if there is no OK or CANCEL button, exit.
 * 4b) if there is an OK or CANCEL button, wait for that one to be pressed.
 *
 * RETURN CODES:
 *               0 -- OK was pressed, or processing ended without complaint
 *              -1 -- some error happened during initialization (see errno)
 *               1 -- CANCEL was pressed
 *               2 -- ESC was pressed
 */
#include "ndwin.h"
#include "nd_objects.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>

/* MENU() keeps an array of object information for each object it wants to
 * deal with.  This array contains a pointer to the object, the origin of
 * the object, and the size of the object (for mouse-click handling.)
 */
typedef struct {
    Obj *obj;		/* object we are linked to */
    int x, y;		/* origin and area taken up by the whole object */
    int dx, dy;
} objLink;


/* Rather than having icky global variables or passing 1500 parameters to
 * the menu refresh function, we populate a Refresh variable with all the
 * information that the refresh function needs to know about.
 *
 * A better solution, of course, would be to convert MENU into a C++
 * class and toss all of these variables into the private: section.
 */
typedef struct {
    void* menu;			/* the window to refresh */
    char* title;		/* the title of the window */
    char* prompt;		/* circles and arrows and a paragraph on the
				 * back... */
    int fancy;			/* is it a fancy window? */
    int wholescreen;		/* will it take up the entire screen */
    int flags;			/* other flags passed to MENU() */
    int promptwidth;		/* widest part of the prompt */
    int depth;			/* number of lines in the menu */
    int width;			/* number of columns in the menu */
    int buttons;		/* this form has buttons. */
    Obj** items;		/* all the objects on this form. */
    int nritems;	 	/* just how many objects are there, anyway? */
} refreshParms;

static void refreshMenu(refreshParms* p);

#if !HAVE_PANEL
/* Panels are just used so that nested windows can be popped up and cleaned
 * up after.  If the curses library doesn't support panels (BSD curses),
 * fake panelling by keeping a list of active windows and refreshing the
 * whole stack at the first input after exiting a menu.
 */
struct frame {
    struct frame *up;
    struct frame *down;
    refreshParms *rf;
} ;

static struct frame bottom = { 0, 0, 0 };
static struct frame *top = &bottom;
static int needredraw = 0;

static void
push(refreshParms *win, struct frame *current)
{
    current->down = top;
    current->rf = win;
    current->up = 0;

    top->up = current;
    top = current;
}

static void
pop()
{
    top = top->down;
    top->up = 0;
    needredraw = 1;
}

void
ndredraw()
{
    struct frame *p;

    if (needredraw) {
	needredraw = 0;

	for (p = &bottom; p; p = p->up)
	    if (p->rf) {
		wclear( Window(top->rf->menu) );
		refreshMenu(top->rf);
	    }
    }
}
#endif

/*
 * sortbybuttononly() compares objects by button, and buttons by x position
 * on the scrollbar.  It's used by MENU to arrange a object chain into a
 * convenient order for tabbing through dialogues.
 */
static int
sortbybuttononly(const void *a, const void *b)
{
    Obj *ta = *(Obj**)a;
    Obj *tb = *(Obj**)b;

    if (ta->Class == O_BUTTON) {
	if (tb->Class != O_BUTTON)
	    return 1;
	return (ta->x) - (tb->x);
    }
    else if (tb->Class == O_BUTTON)
	return -1;
    return 0;
} /* sortbybuttononly */


/*
 * strwidth() is a local function (not published, but not static) to figure
 * how wide a given string will be.
 */
int
strwidth(char *str)
{
    int width = 0, x = 0;

    if (str == 0)
	return 0;

    do {
	if (*str == '\n') {
	    if (x > width)
		width = x;
	    x = 0;
	}
	else if (*str)
	    x++;
    } while (*str++);

    return (x > width) ? x : width;
} /* strwidth */


/*
 * strdepth() is a local function that tells us how many lines a string will
 * take up.
 */
int
strdepth(char *str)
{
    int x = 0, y = 0;

    if (str == 0)
	return 0;

    while (*str)
	if (*str++ == '\n') {
	    ++y;
	    x = 0;
	}
	else
	    ++x;

    return (x > 0) ? (1+y) : y;
} /* strdepth */


/*
 * refreshMenu() prints out a MENU() window and all the stuff contained in
 * it.
 */
void
refreshMenu(refreshParms* p)
{
    int idx;
    int y;
    char *str;
    int origin;
    WINDOW *win = Window(p->menu);
    int formy = WY(p->menu);
    Obj *current = 0;

    werase(win);
    setcolor(win, WINDOW_COLOR);
    if (p->wholescreen)
	/* no window! */;
    else if (p->fancy)
	fancywin(win, p->depth, p->width, p->title, formy, p->buttons);
    else
	simplewin(win, p->depth, p->width, p->title, formy, p->buttons);

    /* print any prompt text */
    if (p->prompt) {
	setcolor(win, (p->flags & ERROR_FLAG) ? ERROR_COLOR : WINDOW_COLOR);
	if (p->flags & ALIGN_RIGHT)
	    origin = p->width-(p->promptwidth + 1 + p->fancy);
	else if (p->flags & ALIGN_LEFT)
	    origin = 1+(p->fancy);
	else
	    origin = ((p->width - p->promptwidth) / 2) + p->fancy;
	wmove(win,1,origin);
	for (y=1, str=(p->prompt); *str; ++str) {
	    if (*str == '\n') {
		++y;
		wmove(win,y,origin);
	    }
	    else
		waddch(win, *str);
	}
    }
    setcolor(win, WINDOW_COLOR);

    /* display all the items on the list
     */
    for (idx=0; idx<p->nritems; idx++) {
	if (IS_CURRENT(p->items[idx])) {
	    if (current)
		drawObj(current, p->menu);
	    current = p->items[idx];
	}
	drawObj(p->items[idx], p->menu);
    }
    if (current)
	drawObj(current, p->menu);
#if !HAVE_DOUPDATE
    wrefresh(win);
#endif
} /* refreshMenu */


/*
 * MENU() builds up a form, then spits it out on the screen and
 * lets the user type into it.
 * If the display has a mouse (cf the ncurses mouse device support)
 * the user can use the mouse to navigate between fields on the form.
 *
 * Before calling menu for the first time, curses MUST be active and properly
 * configured (you don't need to set raw mode, but you do need to set mouse
 * event masks and color pairs.)
 *
 * MENU expects
 *	chain	-- an object chain, built by ObjChain or friends
 *	width	-- the expected width of the form (-1 for autosizing)
 *	depth	-- the expected depth of the form (-1 for autosizing)
 *	title	-- the titlebar at the top of the form.
 *	prompt	-- informative text that is placed at the top of the
 *		   form.
 *	flags	-- special display options (fancy windows, align prompt
 *		   text left or right, [more to follow, I'm sure])
 *
 * MENU returns
 *	0 if the user accepted the input on the form
 *	1 if the user cancelled out of the form (via a cancel button)
 *	2 if the user pressed ESCAPE to flee the form
 *     -1 if something horrible happened when setting up the form.
 */
int
MENU(void *chain, int width, int depth, char *title, char *prompt, int flags)
{
    extern void *coreSortObjChain(void *, int (*)(const void*, const void*));

    Obj *cur;
    Obj **items = 0;		/* list of items in the chain */
    int nritems = 0;		/* number of items in the chain */
    int hasOKbutton = 0;	/* set true if any OK buttons */
    int hasCANCELbutton = 0;	/* set true if any CANCEL buttons */
    int hasbuttons = 0;		/* set true if there are any buttons */
    WINDOW *menu;		/* the curses window we play with */
#if HAVE_PANEL
    PANEL *pan;			/* curses doesn't support backing stores,
				 * but the panel addon does.  So we'll
				 * do panelling to try and make the damned
				 * windows go away properly when they're
				 * done.
				 */
#else
    struct frame frame;
#endif
    int formx=1, formy=1;	/* origin of the data part of the window;
				 * all non-button object manipulation is
				 * relative to this origin
				 */
    int interiorwidth;		/* width of the active part of the menu */
    refreshParms menuInfo;	/* parameters passed to refreshMenu() */
    int firstbutton = 0;	/* first button (index into items[]) */
    int buttony;		/* start line of the button bar */
    int buttonwidth = 0;	/* how much real estate a button needs */
    int autosized = 0;		/* TRUE if we autosized the form */
    int idx;			/* items[] index to the object we are currently
				 * working with.
				 */
    int incr;			/* direction to go to reach the next object
				 * to edit
				 */
    int rc = eNOP;		/* return code from editObj() */
				/* it does anything else */
    int status = MENU_ERROR;	/* return status from input loop */
    int promptwidth;		/* how wide is the prompt? */
#if WITH_NCURSES
    int fancy = (flags & FANCY_MENU) ? 1 : 0;
#else
    int fancy = 0;
#endif
    int wholescreen = 0;
				/* fancy borders on windows? */
#if VERMIN
    int bymouse;		/* editing flag to tell editObj() that it */
				/* needs to deal with a mouse event before */
    MEVENT mouse;		/* for handling mouse clicks */
    mmask_t mev;
#else
    long mouse;			/* dummy the field if curses doesn't support */
				/* vermin */
#endif
    Display *display;		/* our magic display thing */


    if (chain != 0) {
	/* sort all buttons to the end of the chain */
	chain = coreSortObjChain(chain, sortbybuttononly);

	/* count up how many items this chain has */
	cur = OBJ(chain);
	nritems = 0;
	do {
	    if (ISCANCEL(cur))
		hasCANCELbutton = 1;
	    else if (ISCONFIRM(cur))
		hasOKbutton = 1;
	    nritems++;
	    cur = cur->next;

	    /* while we're counting, this is also a good time to register OK
	     * and CANCEL buttons, as well as make some simple consistancy
	     * checks.
	     */
	    if (cur == (Obj*)0 || cur->prev == (Obj*)0) {
		errno = EFAULT;
		return -1;
	    }
	} while (cur != OBJ(chain));
    } /* if (chain != 0) .. button registration, checking, and sorting */

    promptwidth = strwidth(prompt);

    /* figure out the dimensions of the form if we have to
     */
    if (width == 0 && depth == 0) {
	width = COLS;
	depth = LINES;
	wholescreen = 1;
	formy += strdepth(prompt);
    }
    else if (width == -1 || depth == -1) {
	/* We need to figure out the form dimensions from the
	 * objects in the form.
	 */
	int dx, dy;
	int xp, yp;

	autosized = 1;
	width = depth = 0;

	if (chain != 0) {
	    /* compute window size for non-button objects */
	    for (cur = OBJ(chain); objType(cur) != O_BUTTON; ) {

		if (objSizeof(cur, 1, &dx, &dy) == 0
		 && objAt(cur, &xp, &yp) == 0) {
		    yp += dy;
		    xp = (xp >= 0) ? (xp+dx) : dx;

		    if (xp > width)
			width = xp;
		    if (yp > depth)
			depth = yp;
		}

		cur = cur->next;
		if (cur == OBJ(chain))
		    break;
	    }

	    /* if there are buttons, set up for the button bar */
	    if (objType(cur) == O_BUTTON)
		depth += 2;	/* add a row for buttons and a
				 * row for a divider
				 */
	} /* if (chain != 0) .. non-button object initialization */

	/* if there's a prompt, add room for it */
	xp = promptwidth;	/* previously computed from strwidth() */
	dy = strdepth(prompt);
	formy += dy;

	if (xp > width)
	    width = xp;

	if (title && (xp=strlen(title)) > width)
	    width = xp;
	depth += dy;

	/* finally, add the frame size */
	width += 2;
	depth += 2;
    } /* computing form size */
    else {
	/* adjust the Y origin for the size of the prompt */
	formy += strdepth(prompt);
    }

    if (width > COLS || depth > LINES) {
	errno = EOVERFLOW;
	return -1;
    }

    /* the interior width is the space between the borders.  We use
     * it for autopositioning centered objects
     */
    interiorwidth = wholescreen ? width : (width-2);

    if (fancy && width < COLS-2 && depth < LINES-2) {
	/* we have enough room for a fancy embossed look to the window */
	fancy = 1;
	width += 2;	/* width and depth need to be wide enough for the */
	depth += 2;	/* extra lines in the screen */
	formx ++;	/* data entry origin needs to be tweaked for this */
	formy ++;	/* as well. */
    }
    else fancy = 0;
    
    /* the button bar will be the last line on the form.  If there are
     * no buttons, this will be gleefully ignored
     */
    buttony = wholescreen ? (depth-1) : (depth-2);

    if (chain != 0) {
	/* build the item[] array and populate it from all the non-button
	 * fields in the chain.
	 */

	items = malloc (nritems * sizeof items[0]);
	if (items == 0)
	    return -1;

	for (cur=OBJ(chain), idx=0; idx < nritems && objType(cur) != O_BUTTON;
								     idx++) {
	    if (OBJ(cur)->x < 0) {
		/* tweak the X position of this object so that the object will
		 * be centered in the menu.
		 */
		OBJ(cur)->x = ((interiorwidth - OBJ(cur)->width) / 2);
		OBJ(cur)->x -= wholescreen ? 2 : 1;
		if (OBJ(cur)->x < 0)
		    OBJ(cur)->x = 0;
		OBJ(cur)->selx += OBJ(cur)->x;
	    }
	    items[idx] = cur;
	    cur = cur->next;
	} /* everything but the buttons */

	/*
	 * finish populating the items[] array with all the buttons on the
	 * form.
	 */
	if (idx < nritems && objType(cur) == O_BUTTON) {
	    /* now we need to do some button magicke.
	     *
	     * We want to have all the buttons spread across one row, separated
	     * evenly.
	     */
	    Obj *walk;
	    int tmp, bx;
	    int spacing;
	    int nrbuttons = (nritems - idx);

	    firstbutton = idx;

	    if (nrbuttons == 0) {
		if (nritems) free(items);
		errno = EFAULT;
		return -1;
	    }
	    hasbuttons = 1;

	    for (walk = cur, bx=idx; bx < nritems; walk = walk->next, bx++) {
		if (walk->title == (char*)0) {
		    if (nritems) free(items);
		    errno = EFAULT;
		    return -1;
		}
		if ((tmp=strlen(walk->title)+2) > buttonwidth)
		    buttonwidth = tmp;
	    }
	    spacing = (width-2)/nrbuttons;

	    if (buttonwidth > spacing) {
		if (autosized) {
		    spacing = buttonwidth;
		    width = (buttonwidth * nrbuttons) + 2;
		}
		else {
		    if (nritems) free(items);
		    errno = EOVERFLOW;
		    return -1;
		}
	    }

	    for (bx=1, walk=cur; idx < nritems; walk=walk->next, idx++) {
		items[idx] = walk;
		setButtonDataArea(walk,
			    bx + (spacing-strlen(walk->title)-1)/2,
							     buttony-formy);
		bx += spacing;
	    }
	} /* populating items[] from the buttons */
    } /* if (chain != 0) ... buttonbar population */


    /* crank open a window and away we go
     */
    menu = newwin(depth, width, (LINES-depth)/2, (COLS-width)/2);
    if (menu == (WINDOW*)0) {
	/* can't create window.  Ooops */
	if (nritems) free(items);
	return MENU_ERROR;
    }
    if ((display = newDisplay(menu, formx, formy)) == 0) {
	/* can't create display object.  Drat */
	delwin(menu);
	if (nritems) free(items);
	return MENU_ERROR;
    }

#if HAVE_PANEL
    pan = new_panel(menu);
#else
    push(&menuInfo, &frame);
#endif

#if HAVE_KEYPAD
    keypad(menu, TRUE);
#endif
    raw();
    nonl();
    noecho();
    leaveok(menu, FALSE);

#if VERMIN
    mousemask(BUTTON1_PRESSED|BUTTON1_CLICKED|BUTTON1_DOUBLE_CLICKED, &mev);
#endif

    /*
     * set up the refresh control block (aka a simpler way of passing
     * lots of arguments to a function) and do the initial printing of
     * the window.
     *
     * The simple way to do this is rewrite MENU as a C++ class and
     * stuff all these variables into the MENU header.
     */
    menuInfo.menu        = display;
    menuInfo.fancy       = fancy;
    menuInfo.wholescreen = wholescreen;
    menuInfo.depth       = depth;
    menuInfo.width       = width;
    menuInfo.title       = title;
    menuInfo.prompt      = prompt;
    menuInfo.promptwidth = promptwidth;
    menuInfo.buttons     = hasbuttons;
    menuInfo.items       = items;
    menuInfo.nritems     = nritems;
    menuInfo.flags       = flags;

    refreshMenu(&menuInfo);

    if (chain == 0) {
	wrefresh(menu);
	status = MENU_OK;
	goto byebye_no_items;
    }
    tcflush(0, TCIFLUSH);	/* flush any typeahead on this menu */

    /* |vvv| should all this be another function? |vvv| */

    /* put the cursor on the first writable item */
    for (idx = (flags&AT_BUTTON) ? firstbutton : 0; (idx < nritems) && !writable(items[idx]); idx++)
	;
	
    if (idx >= nritems) {
	/* nothing to write?  Bummer. */
	wrefresh(menu);
	status = MENU_OK;
	goto byebye;
    }

    rc = eNOP;
    while (1) {
	int ix;

	rc = editObj(items[idx], display, &mouse, rc);

	incr = 0;

	switch (rc) {
	case eEXITFORM:	status = MENU_OK;
			goto byebye;

	case eERROR:	incr=1;				break;
	case eCANCEL:	/* usually after a callback failed */break;
	case eTAB:	incr=1;				break;
	case eBACKTAB:	incr=-1;			break;

		/* user pressed [return] and the callback worked */
	case eRETURN:	/* with RETURN, we may need to do some
			 * special handling for OK or CANCEL
			 * buttons
			 */
			if (isCANCELbutton(items[idx])) {
			    status = MENU_CANCEL;
			    goto byebye;
			}
			else if (isOKbutton(items[idx])) {
			    status = MENU_OK;
			    goto byebye;
			}
			else
			    incr=1;
			break;

		/* user clicked the mouse */
	case eEVENT:	/* here we need to look for what field the
			 * mouse click was in, then go to that field.
			 * if the mouse WASN'T in any other field,
			 * simply return back to the same object.
			 */
#if VERMIN
			/* first pick out the mouse event and tweak the
			 * coordinates so they are relative to this
			 * window
			 */
			getmouse(&mouse);
			mouse.x -= formx + (COLS-width)/2;
			mouse.y -= formy + (LINES-depth)/2;

			/* then look to see if the mouse was anywhere within
			 * a data entry field
			 */
			for (bymouse=ix=0; ix<nritems; ix++)
			    if (ix != idx && _nd_inside(items[ix], &mouse)) {
				idx = ix;
				bymouse = 1;
				break;
			    }
			/* or if the mouse was inside the current field */
			if (!bymouse)
			    bymouse = _nd_inside(items[idx], &mouse);

			if (!bymouse)
			    rc = eNOP;
#endif/*VERMIN*/
			break;

		/* object wants to resize the screen */
	case eRESIZE:
			/* fall through into eREFRESH */

		/* user pressed ^R */
	case eREFRESH:	wclear(menu);
			refreshMenu(&menuInfo);
			continue;

		/* user pressed ESCAPE */
	case eESCAPE:	status = MENU_ESCAPE;
			goto byebye;
	}

	/* after processing the return from editObj, we'll update anything
	 * that needs to be redisplayed (radio buttons, the formerly current
	 * item which probably is still highlighted from being in a CLICKED
	 * and/or CURRENT state.
	 */
	for (ix=0; ix<nritems; ix++)
	    if (touched(items[ix])) {
		drawObj(items[ix], display);
		untouchObj(items[ix]);
	    }

	/* we may need to adjust the cursor and/or bail out when we're
	 * done editing a field (incr != 0).   It Would Be Bad to end
	 * up sitting on a readonly field, so we walk around the list
	 * of items until we find a writable field.   If we DON'T find
	 * a writable field, the sky has officially fallen and we'll
	 * simply close up shop and return (0) to sender.
	 */
	if (incr != 0) {
	    /* walk the list, looking for the next writable item */
	    for (ix=idx+incr; ; ix += incr) {
		if (ix < 0)
		    ix = nritems-1;
		else if (ix >= nritems) {
		    if ((rc == eTAB) || hasOKbutton || hasCANCELbutton)
			ix = 0;
		    else {
			/* if there are no OK or Cancel buttons, we return
			 * as soon as we reach the end of the form.
			 */
			status = MENU_OK;
			goto byebye;
		    }
		}
		/* when we find a writable item or wrap around to the start,
		 * stop looking
		 */
		if (writable(items[ix]) || ix == idx)
		    break;
	    }
	    if (!writable(items[ix])) {
		/* If we didn't find anything writable, we are done!
		 */
		status = MENU_OK;
		goto byebye;
	    }
	    idx = ix;
	}
    }
byebye:
    free(items);
byebye_no_items:

#if VERMIN
    /* reset the mouse event mask back to what it was */
    mousemask(mev, (mmask_t*)0);
#endif

#if HAVE_PANEL
    del_panel(pan);
#else
    pop();
#endif
    delwin(menu);
    deleteDisplay(display);
#if HAVE_PANEL
    update_panels();
#endif

    return status;	/* default; assume everything is HUNKY-DORY */
} /* MENU */
