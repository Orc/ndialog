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
 *   (orc@pell.chi.il.us)
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
static const char rcsid[] = "$Id$";
/*
 * ndedit: routines to modify things on the screen
 */
#include <errno.h>
#include <unistd.h>
#include <ctype.h>

#include "curse.h"
#include "ndwin.h"
#include "nd_objects.h"


/*-----------------------------------------------*
 *                                               *
 *        O B J E C T   E D I T I N G            *
 *                                               *
 *-----------------------------------------------*/

/*
 * _nd_callback() runs the callback function, if any, and returns the
 * appropriate return code to the caller.
 *
 * The editing functions expect that callbacks will return:
 *	 0 -- callback failed, retry input.
 *	>0 -- callback succeeded, you may proceed.
 *	<0 -- callback succeeded, please exit the form now.
 */
inline int
_nd_callback(Obj *obj, void* win)
{
    int rc;

    if (obj->callback) {
	rc = (*(obj->callback))(obj, win);
#if HAVE_DOUPDATE
	doupdate();
#else
	wrefresh(Window(win));
#endif
	return rc;
    }
    else
	return 1;
} /* _nd_callback */


/* 
 * editString() allows editing of a string object.  Pretty much as
 * you'd expect, but we put <| and |> tabs on the editing frame to
 * show overlap, and let the user reposition the cursor with the mouse.
 *
 * If it's a PASSWORD string, we echo '*' instead of the character the
 * user is trying to enter.
 *
 * in a perfect world, this would handle wchar_t.  It's not a perfect
 * world yet :-(
 */
editCode
editString(void* o, void* win, MEVENT* ev, editCode cc)
{
    Obj *obj = OBJ(o);
    int c;
    int rc = 0;
    int touch = 0;		/* flag marking changes to the display */
    int datalen;		/* how long is the string right now? */
    int insert_mode;		/* inserting or overwriting characters? */
    char *data;

    int xdata, ydata;

#define STARTX	(obj->item.string.startx)
#define CURX	(obj->item.string.curx)

    if (obj == 0 || obj->Class != O_STRING) {
	errno = EINVAL;
	return eERROR;
    }

    xdata = obj->dtx + WX(win);
    ydata = obj->dty + WY(win);

    data = (char*)(obj->content);

    /* can't edit fields with no data or fields set readonly */
    if (data == 0 || OBJ_READONLY(obj))
	return eTAB;

    datalen = strlen(data);
    insert_mode = (obj->flags & INSERT_MODE);

    /*
     * first, set up the initial cursor position and handle any mouse
     * events that may have sent us here.
     */

#if VERMIN
    if (cc == eEVENT) {
	int xp;

	/* need to do initial positioning from mouse events */
	xp = 1+(ev->x - xdata);

	if (xp < 0) {
	    STARTX -= obj->width/2;
	    if (STARTX < 0)
		STARTX = 0;
	    if (CURX > STARTX+obj->width)
		CURX = STARTX;
	    drawObj(obj, win);
	}
	else if (xp == obj->width) {
	    if (STARTX+obj->width < datalen) {
		STARTX += obj->width/2;
		if (STARTX+obj->width >= datalen)
		    STARTX = datalen - obj->width;
		if (STARTX > CURX)
		    CURX = STARTX;
		drawObj(obj, win);
	    }
	}
	else {
	    /* convert xp into an absolute string index */
	    xp += STARTX;
	    if (xp > datalen)
		xp = datalen;

	    /* and assign it back into the object */
	    CURX = xp;
	}
    }
#endif

    wmove(Window(win), ydata, xdata+CURX-STARTX);
    /* and away we go! */
    while ((c = ndgetch(win)) != EOF) {

	switch (c) {
	case KEY_F(1):	_nd_help(objHelp(obj));	break;
#if VERMIN
	case KEY_MOUSE:	return eEVENT; 	/* mouse press; bail back to caller */
#endif
	case 'R'-'@':	return eREFRESH;
	case ESCAPE:	return eESCAPE;
	case KEY_BTAB:
	case KEY_UP:	return eBACKTAB;
	case KEY_DOWN:
	case '\t':	return eTAB;
	case KEY_IC:	if ((insert_mode = !insert_mode) != 0)
			    obj->flags |= INSERT_MODE;
			else
			    obj->flags &= ~INSERT_MODE;
			break;

	case KEY_BACKSPACE:
		if (CURX > 0) {
		    int dix;

		    for (dix=CURX; dix <= datalen; dix++)
			data[dix-1] = data[dix];
		    touch++;
		    CURX--;
		    datalen--;
		    if (CURX < STARTX)
			STARTX = CURX;
		}
		break;

	case KEY_LEFT:
	    if (CURX == 0)
		continue;
	    CURX--;
	    if (CURX < STARTX)
		STARTX = CURX;
	    touch++;
	    break;
	
	case KEY_RIGHT:
	    if (CURX >= datalen)
		continue;
	    else {
		if (CURX-STARTX >= obj->width) {
		    STARTX = (CURX - obj->width)+1;
		    touch++;
		}
		CURX++;
	    }
	    break;

	case '\r':
	    if ((rc = _nd_callback(obj, win)) == 0)
		continue;
	    return (rc < 0) ? eEXITFORM : eRETURN;

	default:
	    if ((c & ~0xff) || c < ' ' || CURX >= obj->item.string.maxlen) {
		beep();
		break;
	    }
	    if (insert_mode) {
		/* if we're inserting, we need to PUSH the string over to
		 * the right, a'la levee.
		 */
		int ix;

		if (datalen >= obj->item.string.maxlen-1) {
		    beep();
		    break;
		}
		for (ix=datalen; ix>=CURX; --ix)
		    data[ix+1] = data[ix];
		datalen++;
	    }
	    data[CURX++] = c;
	    if (CURX > datalen) {
		data[CURX] = 0;
		datalen++;
	    }
	    touch++;
	    if (CURX-STARTX >= obj->width && CURX < obj->item.string.maxlen)
		STARTX = (CURX-obj->width)+1;
	    break;

	}

	if (touch) {
	    drawObj(obj, win);
	    obj->flags |= OBJ_DIRTY;
	    touch = 0;
	}
	wmove(Window(win), ydata, xdata+CURX-STARTX);
    }
    return eESCAPE;		/* bail out on EOF */
} /* editString */


/*
 * editButton() handles button objects.  These are very boring input devices,
 * for all you can do is press [return] or mouseclick to activate them, or
 * press <-, ->, [tab], or [backtab] to pass them by.  The only interesting
 * thing they do is call their callback whenever you click on them.
 */
#define BUTTON_CLICK_DELAY	100000
editCode
editButton(void* o, void* win, MEVENT* ev, editCode cc)
{
    Obj* obj = OBJ(o);
    int xdata, ydata;
    int rc = 0;
    unsigned int c;

    if (obj == 0 || obj->Class != O_BUTTON) {
	errno = EINVAL;
	return eERROR;
    }

    xdata = obj->dtx + WX(win);
    ydata = obj->dty + WY(win);

#if VERMIN
    if (cc == eEVENT) {
	/* if we clicked the mouse here, toggle the value now */

	CLICKED(obj);
	drawObj(obj,win);
	REGULAR(obj);
	wmove(Window(win), ydata, xdata);
	wrefresh(Window(win));
	usleep(BUTTON_CLICK_DELAY);


	if ((rc = _nd_callback(obj,win)) == 0)
	    drawObj(obj,win);
	else
	    return (rc < 0) ? eEXITFORM : eRETURN;
    }
#endif

    wmove(Window(win), ydata, xdata);
    while ((c = ndgetch(win)) != EOF) {
	switch (c) {
	case KEY_F(1):	_nd_help(objHelp(obj));	break;
#if VERMIN
	case KEY_MOUSE:	return eEVENT;
#endif
	case ESCAPE:	return eESCAPE;
	case 'R'-'@':	return eREFRESH;
	case KEY_UP:
	case KEY_LEFT:
	case KEY_BTAB:	return eBACKTAB;
	case KEY_DOWN:
	case KEY_RIGHT:
	case '\t':	return eTAB;
	case '\r':	
			CLICKED(obj);
			drawObj(obj,win);
			REGULAR(obj);
			wmove(Window(win), ydata, xdata);
			wrefresh(Window(win));
			usleep(BUTTON_CLICK_DELAY);
			if ((rc = _nd_callback(obj,win)) == 0)
			    break;
			drawObj(obj,win);
			return (rc < 0) ? eEXITFORM : eRETURN;
	default:	beep();				break;
	}
	wmove(Window(win), ydata, xdata);
	wrefresh(Window(win));
    }
    return eESCAPE;
} /* editButton */


/*
 * editCheck() lets you edit a check box.  There's not much you can do with
 * one of these.  You can press [space] to toggle it, click on it with a
 * mouse, or pass right on through with [tab] or [return].
 *
 * Checkboxes have their callback called every time they change state.
 */
editCode
editCheck(void* o, void* win, MEVENT* ev, editCode cc)
{
    Obj* obj = OBJ(o);
    int xdata, ydata;
    int touch = 0;
    int rc;
    unsigned int c;
    char *data;

    if (obj == 0 || obj->Class != O_CHECK) {
	errno = EINVAL;
	return eERROR;
    }

    if (obj->content == 0)
	return eTAB;

    xdata = obj->dtx + WX(win);
    ydata = obj->dty + WY(win);

    data = (char*)(obj->content);

#if VERMIN
    if (cc == eEVENT) {
	/* if we clicked the mouse here, toggle the value now */

	*data = !(*data);
	if ((rc = _nd_callback(obj,win)) == 0)
	    *data = !(*data);
	else if (rc < 0)
	    return eEXITFORM;
	else
	    drawObj(obj,win);
    }
#endif

    wmove(Window(win), ydata, xdata);
    while ((c = ndgetch(win)) != EOF) {
	switch (c) {
	case KEY_F(1):	_nd_help(objHelp(obj));	break;
#if VERMIN
	case KEY_MOUSE:	return eEVENT;
#endif
	case ESCAPE:	return eESCAPE;
	case 'R'-'@':	return eREFRESH;
	case KEY_UP:
	case KEY_LEFT:
	case KEY_BTAB:	return eBACKTAB;
	case KEY_DOWN:
	case KEY_RIGHT:
	case '\t':	return eTAB;
	case '\r':	return eRETURN;
	case ' ':	*data = !(*data);
			if ((rc = _nd_callback(obj,win)) == 0)
			    *data = !(*data);
			else if (rc < 0)
			    return eEXITFORM;
			else
			    touch++;
			break;
	default:	beep();				break;
	}
	if (touch) {
	    obj->flags |= OBJ_DIRTY;
	    drawObj(obj,win);
	    touch = 0;
	}
	wmove(Window(win), ydata, xdata);
	wrefresh(Window(win));
    }

    return eESCAPE;
} /* editCheck */


/*
 * listItemToggle() is a local function that changes the state of a list
 * item.
 */
static editCode
listItemToggle(Obj* obj, void* win, int index)
{
    int rc = 0;

    if (obj->item.list.items == 0 || index >= obj->item.list.nritems)
	return eNOP;

#define TOGGLE(x)	((x) = !(x))
    if (obj->flags & MENU_LIST) {
	TOGGLE(obj->item.list.items[index].selected);
	if ((rc = _nd_callback(obj, win)) == 0)
	    TOGGLE(obj->item.list.items[index].selected);
	return (rc < 0) ? eEXITFORM : (rc > 0) ? eRETURN : eNOP;
    }
    else if (obj->flags & RADIO_LIST) {
	if (!obj->item.list.items[index].selected) {
	    int x;

	    obj->item.list.items[index].selected = 1;
	    for (x=0; x<obj->item.list.nritems; x++)
		if (x != index)
		    obj->item.list.items[x].selected = 0;
	}
	rc = _nd_callback(obj, win);
    }
    else {
	TOGGLE(obj->item.list.items[index].selected);
	if ((rc = _nd_callback(obj, win)) == 0)
	    TOGGLE(obj->item.list.items[index].selected);
    }
#undef TOGGLE
    return (rc < 0) ? eEXITFORM : eNOP;
} /* listItemToggle */


/*
 * editList() does editing on any of the family of list objects.
 */
editCode
editList(void* o, void* win, MEVENT* ev, editCode cc)
{
    Obj *obj = OBJ(o);
    unsigned int c;
    int dy;			/* delta y for page up/page down movement */
    int ntopy, off_y, ncury;
    int yp;			/* cursor */
    int touch = 0;		/* flag marking changes to the display */
#if HAVE_CURS_SET
    int cursor_visibility;	/* hide the cursor when doing a menu */
#endif
    editCode rc = eESCAPE;	/* default to bailing out */
    int withids;		/* are we displaying IDs? */
    int ismenu;			/* if it's a menu list, there is some
				 * special handling
				 */
    int x;
    int xdata, ydata;

#define TOPY		(obj->item.list.topy)
#define CURY		(obj->item.list.cury)
#define NRITEMS		(obj->item.list.nritems)
#define WINY		(obj->depth)

    if (obj == 0 || obj->Class != O_LIST) {
	errno = EINVAL;
	return eERROR;
    }

    xdata = obj->dtx;
    ydata = obj->dty;

    /* can't edit fields set readonly */
    if (OBJ_READONLY(obj))
	return eTAB;

    withids = obj->flags & SHOW_IDS;

    if ((ismenu = (obj->flags & MENU_LIST)) != 0) {
	/*
	 * if we're a menu list, unselect everything
	 */
	if (obj->item.list.items)
	    for (yp=0; yp<obj->item.list.nritems; yp++)
		obj->item.list.items[yp].selected = 0;
    }

    /*
     * then set up the initial cursor position and handle any mouse
     * events that may have sent us here.
     */

#if VERMIN
    if (cc == eEVENT) {
	/* need to do initial positioning from mouse events */
	yp = (ev->y - ydata);

	if (yp < 0 && TOPY > 0) {
	    /* scroll backwards */
	    dy = obj->depth;
	    TOPY -= dy;
	    if (TOPY < 0) {
		dy += TOPY;
		TOPY = 0;
	    }
	    CURY -= dy;
	}
	else if (yp == obj->depth && TOPY < obj->item.list.nritems) {
	    /* scroll forwards */
	    ntopy = TOPY + WINY;
	    off_y = CURY - TOPY;

	    if (ntopy + WINY < NRITEMS)
		TOPY = ntopy;
	    else if (NRITEMS - WINY > TOPY)
		TOPY = NRITEMS - WINY;

	    ncury = TOPY + off_y;
	    CURY = (ncury < NRITEMS) ? ncury : (NRITEMS-1);
	}
	else {
	    /* convert yp into an list item index */

	    yp += TOPY;
	    if (yp > obj->item.list.nritems)
		yp = obj->item.list.nritems;

	    /* and assign it back into the object */
	    CURY = yp;

	    if (ismenu && ((obj->flags & DEL_LIST) || !(ev->bstate & BUTTON1_DOUBLE_CLICKED)) )
		rc = eNOP;
	    else
		rc = listItemToggle(obj, win, CURY);

	    drawObj(obj, win);
	    if (rc == eEXITFORM || rc == eRETURN)
		return rc;
	}
	drawObj(obj, win);
    }
#endif

#if HAVE_CURS_SET
    cursor_visibility = curs_set(0);
#endif
    /* and away we go! */
    while ((c = ndgetch(win)) != EOF) {

	switch (c) {
	case KEY_F(1): _nd_help(objHelp(obj)); break;
#if VERMIN
	case KEY_MOUSE:	rc = eEVENT; 	goto bailout;
#endif
	case 'R'-'@':	rc = eREFRESH;	goto bailout;
	case ESCAPE:	rc = eESCAPE;	goto bailout;
	case KEY_BTAB:
	case KEY_LEFT:	rc = eBACKTAB;	goto bailout;
	case KEY_RIGHT:
	case '\t':	rc = eTAB;	goto bailout;

	case KEY_HOME:
	    TOPY = 0;
	    if (CURY > obj->depth)
		CURY = 0;
	    touch++;
	    break;

	case KEY_END:
	    TOPY = obj->item.list.nritems - obj->depth;
	    if (CURY < TOPY)
		CURY = TOPY;
	    touch++;
	    break;

	case 'U'-'@':
	case KEY_PPAGE:
	    dy = obj->depth;
	    TOPY -= dy;
	    if (TOPY < 0) {
		dy += TOPY;
		TOPY = 0;
	    }
	    CURY -= dy;
	    touch++;
	    break;

	case '-':
	case KEY_UP:
	    if (CURY == 0)
		continue;
	    CURY--;
	    if (CURY < TOPY)
		TOPY = CURY;
	    touch++;
	    break;
	
	case 'D'-'@':
	case KEY_NPAGE:
	    /* scroll forwards */
	    ntopy = TOPY + WINY;
	    off_y = CURY - TOPY;

	    if (ntopy + WINY < NRITEMS)
		TOPY = ntopy;
	    else if (NRITEMS - WINY > TOPY)
		TOPY = NRITEMS - WINY;

	    ncury = TOPY + off_y;
	    CURY = (ncury < NRITEMS) ? ncury : (NRITEMS-1);

	    touch++;
	    break;

	case '+':
	case KEY_DOWN:
	    if (CURY < obj->item.list.nritems-1) {
		CURY++;
		if (CURY >= TOPY+obj->depth)
		    TOPY++;
		touch++;
	    }
	    break;

	case KEY_BACKSPACE:
	case KEY_DC:
	    if (!(obj->flags & DEL_LIST))
		break;
	    rc = listItemToggle(obj, win, CURY);
	    if (rc == eEXITFORM || rc == eRETURN)
		goto bailout;
	    touch = 1;
	    break;

	case ' ':
	    if (ismenu)
		continue;
	case '\r':
	    if (!(obj->flags & CR_LIST))
		break;
	    rc = listItemToggle(obj, win, CURY);
	    if (rc == eEXITFORM || rc == eRETURN)
		goto bailout;
	    touch = 1;
	    break;

	default:
	    if (obj->flags & NO_HOTKEYS)
		break;
	    if (obj->item.list.items == 0)
		break;
	    c = toupper(c);
	    for (x = (CURY+1) % obj->item.list.nritems;
					    x != CURY;
		    x = (x+1) % obj->item.list.nritems) {

		if (withids) {
		    if (c == toupper(obj->item.list.items[x].id[0]))
			break;
		}
		else if (c == toupper(obj->item.list.items[x].item[0]))
			break;
	    }
	    if (x != CURY) {
		CURY = x;
		if (x < TOPY)
		    TOPY=x;
		else if (x >= TOPY+obj->depth)
		    TOPY = (CURY-obj->depth)+1;

		touch = 1;
	    }
	    break;
	}

	if (touch) {
	    obj->flags |= OBJ_DIRTY;
	    drawObj(obj, win);
	    touch = 0;
	}
    }
#undef TOPY
#undef CURY

bailout:
#if HAVE_CURS_SET
    curs_set(cursor_visibility);
#endif
    return rc;
} /* editList */
