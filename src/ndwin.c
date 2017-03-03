/*
 * ndwin: routines to display things on the screen
 *
 * Copyright (C) 1996-2017 David L Parsons.
 * The redistribution terms are provided in the COPYRIGHT file that must
 * be distributed with this source code.
 */
#include <config.h>

#include <errno.h>
#include <string.h>

#include "curse.h"
#include "ndwin.h"
#include "nd_objects.h"

#ifndef ACS_HLINE
#   define	ACS_HLINE	'-'
#   define	ACS_LARROW	'>'
#   define	ACS_LLCORNER	'+'
#   define	ACS_LRCORNER	'+'
#   define	ACS_LTEE	'+'
#   define	ACS_RARROW	'<'
#   define	ACS_RTEE	'+'
#   define	ACS_ULCORNER	'+'
#   define	ACS_URCORNER	'+'
#   define	ACS_VLINE	'|'
#endif

/* rillyrillylongblankstring is used to space-fill menu and text rows
 * if we don't have actual content to put there
 */
char rillyrillylongblankstring[] =
"                                                                            "
"                                                                            "
"                                                                            "
"                                                                            ";


/*-----------------------------------------------*
 *                                               *
 *        O B J E C T   P L A C E M E N T        *
 *                                               *
 *-----------------------------------------------*/

#if DYNAMIC_BINDING
/*
 * nd_buttonSize() returns the size of a button
 */
int
nd_buttonSize(void* obj, int flag, int* dx, int* dy)
{
    /* buttons are a special case.  They are one line high, but
     * they don't really have a `width', being automatically
     * formatted onto the screen
     */
    *dx = 0;
    *dy = 1;
    return 0;
} /* nd_buttonSize */


/*
 * nd_typeSize() returns the size of any of the other basic types
 */
int
nd_typeSize(void *o, int flag, int *dx, int *dy)
{
    int title_size;
    Obj *obj = OBJ(o);

    /* set X and Y, general case */

    *dx = obj->width;
    if (obj->prefix)
	*dx += strlen(obj->prefix);
    if (obj->suffix)
	*dx += strlen(obj->suffix);

    if (obj->title) {
	title_size = strlen(obj->title);
	if (title_size > *dx)
	    *dx = title_size;
    }

    *dy = obj->depth;
    if (obj->title)
	(*dy)++;
    switch (obj->Class) {
    case O_CHECK:   (*dx) += 2; /* `[' and `]' */
		    break;
    case O_GAUGE:
    case O_STRING:	/* if strings, gauges, or lists have */
    case O_LIST:	/* prefixes or suffixes, we put a frame */
			/* around them to visually isolate the */
			/* data entry field */
		    if (obj->prefix || obj->suffix) {
			(*dx) += 2;
			(*dy) += 2;
		    }
		    break;

    case O_TEXT:    (*dy) += 2;		/* textboxes always have */
		    (*dx) += 2;		/* frames around them */
		    break;

    default:	/* to shut lint up.  mumble */
		    break;
    }
    return 0;
} /* nd_typeSize */
#endif


/*
 * objSizeof() returns the x and y dimensions (including prompts and
 * borders) of the object in question.  If maximize is > 0, it returns
 * the maximum size of the object, if it's == 0, it returns the current
 * size of the object, and if it's < 0, it returns the minimum size of
 * the object.
 *
 * objSizeof() returns:
 *	0 if it can determine the size of the object.
 *	-1 if it can't (errno will be set with a guess as to why)
 */
int
objSizeof(void* o, int flag, int *dx, int *dy)
{
    Obj *obj = OBJ(o);
#if !DYNAMIC_BINDING
    int title_size;
#endif

    if (obj == 0) {
	errno = EINVAL;
	return -1;
    }

#if DYNAMIC_BINDING
    if (nd_object_table[obj->Class].size)
	return (nd_object_table[obj->Class].size)(o, flag, dx, dy);
    errno = EINVAL;
    return -1;
#else
    switch (obj->Class) {
    case O_BUTTON:
	    /* buttons are a special case.  They are one line high, but
	     * they don't really have a `width', being automatically
	     * formatted onto the screen
	     */
	    *dx = 0;
	    *dy = 1;
	    break;
    default:
	    /* set X and Y, general case */

	    *dx = obj->width;
	    if (obj->prefix)
		*dx += strlen(obj->prefix);
	    if (obj->suffix)
		*dx += strlen(obj->suffix);

	    if (obj->title) {
		title_size = strlen(obj->title);
		if (title_size > *dx)
		    *dx = title_size;
	    }

	    *dy = obj->depth;
	    if (obj->title)
		(*dy)++;
	    switch (obj->Class) {
	    case O_CHECK:   (*dx) += 2; /* `[' and `]' */
			    break;
	    case O_GAUGE:
	    case O_STRING:	/* if strings, gauges, or lists have */
	    case O_LIST:	/* prefixes or suffixes, we put a frame */
				/* around them to visually isolate the */
				/* data entry field */
			    if (obj->prefix || obj->suffix) {
				(*dx) += 2;
				(*dy) += 2;
			    }
			    break;

	    case O_TEXT:    (*dy) += 2;		/* textboxes always have */
			    (*dx) += 2;		/* frames around them */
			    break;

	    default:	/* to shut lint up.  mumble */
			    break;
	    }
	    break;
    }
    return 0;
#endif
} /* _objSize */


/*
 * objSize() returns the x and y dimensions (including prompts and
 * borders) of the object in question.
 * objSize() returns:
 *	0 if it can determine the size of the object.
 *	-1 if it can't (errno will be set with a guess as to why)
 */
int
objSize(void* o, int* dx, int* dy)
{
    return objSizeof(o, 0, dx, dy);
}


/*
 * objAt() returns the x,y position of the upper right hand corner of the
 * object.
 */
int
objAt(void* obj, int* x, int* y)
{
    *x = *y = 0;

    if (obj == 0) {
	errno = EINVAL;
	return -1;
    }

    if (objType(obj) != O_BUTTON) {
	*x = OBJ(obj)->x;
	*y = OBJ(obj)->y;
    }

    return 0;
} /* objAt */


/*
 * objDataAt() returns the x,y,dx,dy area containing the data (not the
 * prompts or windowframes) for the object.
 */
int
objDataAt(void* obj, int* x, int* y, int* dx, int *dy)
{
    int dummy;

    if (obj == 0) {
	errno = EINVAL;
	return -1;
    }

    if (x == 0)   x = &dummy;
    if (y == 0)   y = &dummy;
    if (dx == 0) dx = &dummy;
    if (dy == 0) dy = &dummy;

    *x  = OBJ(obj)->dtx;
    *y  = OBJ(obj)->dty;
    *dx = OBJ(obj)->width;
    *dy = OBJ(obj)->depth;
    return 0;
} /* objDataAt */


/*-----------------------------------------------*
 *                                               *
 *        O B J E C T   D R A W I N G            *
 *                                               *
 *-----------------------------------------------*/

/*
 * drawButton() draws a button.
 */
void
drawButton(void *obj, void *w)
{
    int highlight = 0;
    int boxcolor;
    int objcolor;
    WINDOW* win = Window(w);
    int x = WX(w),
	y = WY(w);

    if (obj == 0 || objType(obj) != O_BUTTON)
	return;

    x += OBJ(obj)->dtx;
    y += OBJ(obj)->dty;

    boxcolor = IS_CURRENT(obj) ? ACTIVE_COLOR : WINDOW_COLOR;
    objcolor = BUTTON_COLOR | (OBJ_READONLY(obj) ? READONLY_COLOR : 0);

    wmove(win, y, x-1);
    setcolor(win, boxcolor);
    waddch(win, '[');
    highlight = (OBJ(obj)->content && *((int*)(OBJ(obj)->content)) != 0);
    getyx(win,y,x);
    if (OBJ(obj)->flags & OBJ_CLICKED) {
	setcolor(win, PRESSED_COLOR);
	waddstr(win, OBJ(obj)->title);
    }
    else {
	setcolor(win, IS_CURRENT(obj) ? (objcolor|CURRENT_COLOR) : objcolor);
	waddstr(win,OBJ(obj)->title);
    }
    setcolor(win, boxcolor);
    waddch(win, ']');
    wmove(win,y,x);
} /* drawButton */


/*
 * drawCheck() draws a checkbox
 */
void
drawCheck(void *obj, void *w)
{
    int boxcolor;
    int objcolor;
    WINDOW *win = Window(w);
    int x = WX(w),
	y = WY(w);
    int vx,vy;

    if (obj == 0 || OBJ(obj)->Class != O_CHECK)
	return;

    boxcolor = IS_CURRENT(obj) ? ACTIVE_COLOR : WINDOW_COLOR;
    objcolor = WINDOW_COLOR | (OBJ_READONLY(obj) ? READONLY_COLOR : 0);
    ADJUSTXY(OBJ(obj), x, y);
    wmove(win, y, x);
    setcolor(win, objcolor);
    if (OBJ(obj)->title) {
	if (OBJ(obj)->prefix)
	    wmove(win, y, x+strlen(OBJ(obj)->prefix));
	waddstr(win, OBJ(obj)->title);
	y++;
    }
    wmove(win, y, x);
    if (OBJ(obj)->prefix)
	waddstr(win, OBJ(obj)->prefix);
    setcolor(win, boxcolor);
    waddch(win, '[');
    getyx(win,vy,vx);
    waddstr(win, " ]");
    setcolor(win, objcolor);
    if (OBJ(obj)->suffix)
	waddstr(win, OBJ(obj)->suffix);
#if 0
    if (IS_CURRENT(obj))
	setcolor(win, SELECTED_COLOR);
#endif
    wmove(win,vy,vx);
    waddch(win, (OBJ(obj)->content && *(char*)(OBJ(obj)->content)) ? 'X' : ' ');
#if 0
    setcolor(win, objcolor);
#endif
} /* drawCheck */


#define DREW_TITLE	0x01
#define DREW_PREFIX	0x02
#define DREW_SUFFIX	0x04
#define DREW_A_BOX	0x08
/*
 * _nd_drawObjCommon() draws the common parts of string, menu, list, and text
 * objects.  It returns a bitmap of what it drew:
 *
 *	rc & DREW_TITLE		-- drew a title
 *	rc & DREW_PREFIX	-- drew a prefix
 *	rc & DREW_SUFFIX	-- drew a suffix
 *	rc & DREW_A_BOX		-- drew a little box around where the data
 *				   is supposed to go.
 *
 */
int
_nd_drawObjCommon(void *o, void* w)
{
    Obj *obj = OBJ(o);
    int preflen;		/* length of any prefix */
    int sufflen;		/* length of any suffix */
    int status = 0;
    int boxcolor, objcolor;
    WINDOW *win = Window(w);
    int x = WX(w),
	y = WY(w);

    if (obj == 0)
	return 0;

    ADJUSTXY(obj, x, y);

    /* If we're part of a widget, we either return DREW_A_BOX if we've
     * got an (ignored) prefix or suffix or 0.
     */
    if (obj->parent != 0 && (obj->flags & OBJ_DRAW) == 0)
	return (obj->prefix || obj->suffix) ? DREW_A_BOX : 0;

    /* otherwise we draw like mad */
    boxcolor = IS_CURRENT(o) ? ACTIVE_COLOR : WINDOW_COLOR;
    objcolor = WINDOW_COLOR | (OBJ_READONLY(o) ? READONLY_COLOR : 0);

    preflen = obj->prefix ? strlen(obj->prefix) : 0;
    sufflen = obj->suffix ? strlen(obj->suffix) : 0;

    setcolor(win, objcolor);
    /* print any title */
    if (obj->title) {
	/* If we have a prefix or suffix, the data will be boxed, so
	 * don't forget to adjust the title position to account for
	 * the box frame.
	 */
	if (obj->prefix || obj->suffix)
	    wmove(win, y, x+preflen+1);
	else
	    wmove(win, y, x);
	waddstr(win, obj->title);
	status |= DREW_TITLE;
	y++;
    }
    /* print a box around the data field, if needed */
    if (obj->prefix || obj->suffix || objType(obj) == O_TEXT) {
	drawbox(win, y, x+preflen, obj->depth+2, obj->width+2, 0,
					boxcolor, boxcolor);
	status |= DREW_A_BOX;
	y++;
    }
    setcolor(win, objcolor);
    /* print any prefix */
    if (obj->prefix) {
	wmove(win, y, x);
	status |= DREW_PREFIX;
	waddstr(win, obj->prefix);
    }

    /* print any suffix */
    if (obj->suffix) {
	wmove(win, y, x+preflen+2+obj->width);
	waddstr(win, obj->suffix);
	status |= DREW_SUFFIX;
    }
    return status;
} /* _nd_drawObjCommon */


/*
 * _nd_adjustXY() is an unpublished function that
 * adjusts a x/y pair according to a return code from
 * _nd_drawObjCommon.
 */
void
_nd_adjustXY(int rc, void* o, int* x, int* y)
{
    ADJUSTXY(OBJ(o), (*x), (*y));
    if (rc & DREW_TITLE)
	(*y)++;
    if (rc & DREW_A_BOX) {
	(*y)++;
	(*x)++;
    }
    if (rc & DREW_PREFIX)
	(*x) += strlen(OBJ(o)->prefix);
} /* _nd_adjustXY */


/*
 * drawString() draws a string
 */
void
drawString(void *o, void *w)
{
    Obj *obj = OBJ(o);
    int idx;			/* all god's chillums need indices */
    int start;			/* string window start */
    int moretofollow = 0;	/* does more string follow to the right
				 * of the window?
				 */
    int rc;
    WINDOW *win = Window(w);
    int x = WX(w),
	y = WY(w);

    if (obj == 0 || obj->Class != O_STRING)
	return;

    rc = _nd_drawObjCommon(o, w);
    _nd_adjustXY(rc, o, &x, &y);
    
    /* print the data contained in the field */
    /* we want to show the cursor, so people can come back to this
     * field without having their brains bleed.
     */
    start = obj->item.string.startx;
    wmove(win, y, x);
    if (IS_CURRENT(obj))
	setcolor(win, SELECTED_COLOR);
    for (idx=start; idx-start < obj->width && ((char*)(obj->content))[idx]; idx++) {
	if (obj->flags & PASSWORD_STRING)
	    waddch(win, '*');
	else
	    waddch(win, ((char*)(obj->content))[idx]);
    }
    if (idx-start == obj->width && ((char*)(obj->content))[idx])
	moretofollow = 1;

    /* erase any text poop left in the field, so everything will look
     * beautiful
     */
    for (;idx-start < obj->width; idx++)
	waddch(win, ' ');

#if 0
    if (IS_CURRENT(obj))
	setcolor(win, WINDOW_COLOR);
#endif
    if (rc & DREW_A_BOX) {
	setcolor(win, WIDGET_COLOR);
	if (start > 0)
	    mvwaddch(win, y, x-1, ACS_LARROW);
	if (moretofollow)
	    mvwaddch(win, y, x+obj->width, ACS_RARROW);
    }
} /* drawString */


/*
 * drawCheckItem() is a local function that draws a check list item (for
 * drawList)
 */
static void
drawCheckItem(Obj *obj, int idx, WINDOW *win, int x, int y)
{
    int current = obj->item.list.cury;
    int selected = obj->item.list.items[idx].selected;
    int isradio = (obj->flags & RADIO_LIST);
    int iscurrent = 0;

    wmove(win, y, x);
    setcolor(win, WINDOW_COLOR);
    waddch(win, isradio ? '(' : '[');
    if (IS_CURRENT(obj) && current == idx) {
	iscurrent = 1;
	setcolor(win, SELECTED_COLOR);
	waddch(win, selected? 'X' : ' ');
    }
    else
	waddch(win, selected? 'X' : ' ');

    setcolor(win, WINDOW_COLOR);

    waddch(win, isradio ? ')' : ']');
    waddch(win, ' ');
    if (obj->flags & SHOW_IDS) {
	char *p = obj->item.list.items[idx].id;
	if ((obj->flags & NO_HOTKEYS) == 0) {
	    setcolor(win, iscurrent ? SELECTED_COLOR : HOTKEY_COLOR);
	    waddch(win, (*p++));
	}
	setcolor(win, WIDGET_COLOR);
	waddstr(win, p);
    }
    setcolor(win, WINDOW_COLOR);
    wmove(win, y, x+obj->item.list.itemoffset);
    waddnstr(win, obj->item.list.items[idx].item, obj->width - obj->item.list.itemoffset);
} /* drawCheckItem */


/*
 * drawMenuItem() is a local function that draws a menu list item (for
 * drawList)
 */
static void
drawMenuItem(Obj *obj, int idx, WINDOW *win, int x, int y)
{
    int current = obj->item.list.cury;
    int iscurrent = (current == idx && (IS_CURRENT(obj) || (obj->flags & ALWAYS_HIGHLIT)));
    int color = iscurrent ? SELECTED_COLOR : WINDOW_COLOR;

    if (iscurrent && !IS_CURRENT(obj))
	color |= READONLY_COLOR;

    wmove(win, y, x);

    if (obj->flags & SHOW_IDS) {
	char *p = obj->item.list.items[idx].id;
	if ((obj->flags & NO_HOTKEYS) == 0) {
	    setcolor(win, iscurrent ? SELECTED_COLOR : HOTKEY_COLOR);
	    waddch(win, (*p++));
	}
	setcolor(win, color);
	waddstr(win, p);
    }

    setcolor(win, color);
    wmove(win, y, x+obj->item.list.itemoffset);
    waddnstr(win, obj->item.list.items[idx].item, obj->width - obj->item.list.itemoffset);
    setcolor(win, WINDOW_COLOR);
} /* drawMenuItem */


/*
 * drawHighlitItem() is a local function that draws a highlit
 * checklist item (for drawList)
 */
static void
drawHighlitItem(Obj* obj, int idx, WINDOW* win, int x, int y)
{
    int current = obj->item.list.cury;
    int selected = obj->item.list.items[idx].selected;
    int color = selected ? SELECTED_COLOR : WINDOW_COLOR;

    if ((current == idx) && IS_CURRENT(obj)) {
	mvwaddch(win, y, x+obj->width-1, ACS_LARROW);
	mvwaddch(win, y, x, ACS_RARROW);
    }
    wmove(win, y, x + 1);

    if (obj->flags & SHOW_IDS) {
	char *p = obj->item.list.items[idx].id;
	if ((obj->flags & NO_HOTKEYS) == 0) {
	    setcolor(win, HOTKEY_COLOR);
	    waddch(win, (*p++));
	}
	setcolor(win, color);
	waddstr(win, p);
    }
    setcolor(win, color);
    wmove(win, y, x+obj->item.list.itemoffset);
    waddnstr(win, obj->item.list.items[idx].item, obj->width - obj->item.list.itemoffset);
    setcolor(win, WINDOW_COLOR);
} /* drawHighlitItem */


/*
 * drawListElement() is a local that draws a single list item
 */
static void
drawListElement(Obj *obj, WINDOW *win, int idx, int y, int x, int start)
{
    /* clear this line */
    wmove(win, y+idx-start, x);
    waddnstr(win, rillyrillylongblankstring, obj->width);

    /* don't try to draw anything unless there's something to draw */
    if (obj->item.list.items && idx < obj->item.list.nritems)
	switch (obj->item.list.kind) {
	default:	/* we'll default to a checked item if the sky falls */
	case LO_CHECK:
		    drawCheckItem(obj, idx, win, x, y+idx-start);
		    break;
	case LO_HIGHLIT:
		    drawHighlitItem(obj, idx, win, x, y+idx-start);
		    break;
	case LO_MENU:
		    drawMenuItem(obj, idx, win, x, y+idx-start);
		    break;
	}
}


/*
 * drawList() draws a listbox
 */
void
drawList(void *o, void* w)
{
    Obj *obj = OBJ(o);
    int start, rc, idx;
    WINDOW *win = Window(w);
    int x = WX(w),
	y = WY(w);
    int currentitem = -1;

    if (obj == 0 || objType(obj) != O_LIST)
	return;
    rc = _nd_drawObjCommon(obj, w);
    _nd_adjustXY(rc, obj, &x, &y);

    start = obj->item.list.topy;

    if ((rc & DREW_A_BOX) && obj->width > 3) {
	if (start > 0) {
	    setcolor(win, WIDGET_COLOR);
	    mvwaddstr(win, y-1, x+obj->width-3, "(-)");
	}
	if (start+obj->depth < obj->item.list.nritems) {
	    setcolor(win, WIDGET_COLOR);
	    mvwaddstr(win, y+obj->depth, x+obj->width-3, "(+)");
	}
    }
    setcolor(win, WINDOW_COLOR);

    for (idx = start; idx-start < obj->depth; idx++) {
	if ((obj->item.list.cury == idx) &&
		(IS_CURRENT(obj) || (obj->flags & ALWAYS_HIGHLIT)))
	    currentitem = idx;
	else
	    drawListElement(obj,win,idx,y,x,start);
    }
    if (currentitem >= 0)
	drawListElement(obj,win,currentitem,y,x,start);
} /* drawList */


/*
 * drawGauge() draws a progress bar
 */
void
drawGauge(void *o, void* w)
{
    Obj *obj = OBJ(o);
    int percent;
    int fillwidth;
    int rc;
    WINDOW *win = Window(w);
    int x = WX(w),
	y = WY(w);
    char bfr[5];

    if (obj == 0 || obj->Class != O_GAUGE)
	return;

    percent = obj->content ? *((int*)(obj->content)) : 0;

    fillwidth = (obj->width * percent) / 100;

    rc = _nd_drawObjCommon(o, w);
    _nd_adjustXY(rc, o, &x, &y);
    
    /* clear the gauge area */
    wmove(win, y, x);
    setcolor(win, WINDOW_COLOR);
    waddnstr(win, rillyrillylongblankstring, obj->width);

    if (obj->width > 4) {
	/* draw the percentage in the middle of the progress bar,
	 * appropriately shaded */
	sprintf(bfr, "%d%%", percent);

	wmove(win, y, x + ((obj->width-strlen(bfr))/2) );
	waddstr(win, bfr);
    }

#if WITH_NCURSES
    /* highlight the progress bar as appropriate */
    mvwchgat(win, y, x, fillwidth, A_REVERSE, PAIR_NUMBER(WINDOW_COLOR), 0);
#else
    wstandout(win);
    for (rc = 0; rc < fillwidth; rc++)
	mvwaddch(win, y, x+rc, mvwinch(win, y, x+rc));
    wstandend(win);
#endif

} /* drawGauge */


/*-----------------------------------------------*
 *                                               *
 *        W I N D O W   D R A W I N G            *
 *                                               *
 *-----------------------------------------------*/

/*
 * fancywin() draws a fancy window.  A fancy window is an ordinary
 * window (with optional buttonbar slice) with an extra window inside,
 * for a cheesy 3-d effect.
 */
void
fancywin(WINDOW *win, int lines, int cols, char *title, int formy, int wb)
{
    int dx;
    simplewin(win, lines, cols, title, formy, wb);

    dx = (lines - (wb ? 4 : 2)) - formy + 2;
    drawbox(win, formy-1, 1, dx, cols-2, 0, WINDOW_COLOR, RELIEF_COLOR);
} /* fancywin */


/*
 * simplewin() draws a simple window
 */
void
simplewin(WINDOW *win,			/* ... in the given WINDOW */
	  int lines,			/* of this many lines */
	  int cols,			/* and this many cols */
	  char *title,			/* with this title */
	  int formy,			/* starting here */
	  int withbuttons)		/* and with buttons, perhaps? */
{
    werase(win);
    drawbox(win, 0, 0, lines, cols, withbuttons ? lines-3 : 0,
				    RELIEF_COLOR, WINDOW_COLOR);

    if (title) {
	int titlex = (cols - strlen(title)) / 2;

	setcolor(win, TITLE_COLOR);
	if (titlex < 1) {
	    wmove(win, 0, 0);
	    waddnstr(win, title, cols-2);
	}
	else
	    mvwaddstr(win, 0, titlex, title);
    }
    setcolor(win, WINDOW_COLOR);
} /* simplewin */


/*
 * drawbox: draw a box with an optional horizontal dividing line.
 */
void
drawbox(WINDOW *win,			/* ... in the given WINDOW */
        int y, int x,			/* at this origin */
	int height, int width,		/* this high, this wide */
	int slice,			/* with a dividing line */
	int sunlight, int shade)	/* cheesy 3-d effects */
{
    int i, j;
    int right = width-1,
	bottom = height-1;

    for (i = 0; i < height; i++) {
	wmove(win, i+y, x);
	for (j = 0; j < width; j++) {
	    if (i == 0)			/* topline */
		if (j == 0) {		/* upper left corner */
		    setcolor(win, sunlight);
		    waddch(win, ACS_ULCORNER);
		}
		else if (j == right) {	/* upper right corner */
		    setcolor(win, shade);
		    waddch(win, ACS_URCORNER);
		}
		else {
		    setcolor(win, sunlight);
		    waddch(win, ACS_HLINE);
		}
	    else if (i == bottom)	/* bottom line */
		if (j == 0) {		/* lower left corner */
		    setcolor(win, sunlight);
		    waddch(win, ACS_LLCORNER);
		}
		else if (j == right) {	/* lower right corner */
		    setcolor(win, shade);
		    waddch(win, ACS_LRCORNER);
		}
		else {
		    setcolor(win, shade);
		    waddch(win, ACS_HLINE);
		}
	    else if (i == slice)	/* dividing line */
		if (j == 0) {		/* left side */
		    setcolor(win, sunlight);
		    waddch(win, ACS_LTEE);
		}
		else if (j == right) {	/* or right side */
		    setcolor(win, shade);
		    waddch(win, ACS_RTEE);
		}
		else {
		    setcolor(win, sunlight);
		    waddch(win, ACS_HLINE);
		}
	    else {
		if (j == 0) {		/* left side */
		    setcolor(win, sunlight);
		    waddch(win, ACS_VLINE);
		}
		else if (j == right) {	/* or right side */
		    setcolor(win, shade);
		    waddch(win, ACS_VLINE);
		}
		else {
		    setcolor(win, WINDOW_COLOR);
		    waddch(win, ' ');
		}
	    }
	}
    }
} /* drawbox */
