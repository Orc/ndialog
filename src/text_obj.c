/*
 * text_obj: functions for the text object and subclasses
 *
 * Copyright (C) 1996-2017 David L Parsons.
 * The redistribution terms are provided in the COPYRIGHT file that must
 * be distributed with this source code.
 */
#include <config.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "curse.h"
#include "ndwin.h"
#include "nd_objects.h"

#include "html.h"
#include "bytecodes.h"

/*
 * newText() creates a new textbox
 */
void*
newText(int x, int y, int width, int depth, int bfrsize,
        char* prompt, char* prefix, char* bfr, pfo callback, char* help)
{
    Obj *tmp;
    extern int newTextData(Obj *obj);

    if (width < 1 || depth < 1 || bfrsize < 1 || bfr == (char*)0) {
	errno = EINVAL;
	return 0;
    }

    tmp = _nd_newObj(callback, O_TEXT, bfr, prompt, prefix,
			      x, y, width, depth, help);

    if (tmp != (Obj*)0) {
	if (newTextData(tmp) < 0) {
	    deleteObj(tmp);
	    return 0;
	}
	tmp->sely --;		/* adjust the clickable area so that people */
	tmp->seldepth += 2;	/* can click on the scroll tabs */
	tmp->selx --;
	tmp->selwidth += 2;
	tmp->item.text.class = T_IS_TEXT;
    }
    return tmp;
} /* newText */


/*
 * newHelp creates a helpfile object, which is a Text object with the
 * html attribute
 */
void *
newHelp(int x, int y, int width, int height,
        char *document, pfo callback, char *help)
{
    Page *page;
    FILE *f;
    Obj *tmp = 0;
    char *label;
    int llen = 0;
    unsigned char *pg;
    char *filename = alloca(strlen(document)+5);
    int yp;
    int ix;

    strcpy(filename, document);

    /* pull off #labels */
    if ((label = strchr(filename, '#')) != (char*)0) {
	*label++ = 0;
	llen = strlen(label);
    }

    if ((f = fopen(filename, "r")) != (FILE*)0) {
	page = render(f, width);
	fclose(f);
    }
    else {
	page = calloc(1, sizeof *page);
	if (page) {
	    page->title    = strdup("File Not Found");
	    page->titlelen = strlen(page->title);
	    page->page     = malloc(100+strlen(filename));
	    if (page->page)
		sprintf((char*)(page->page), "%s: %s\n", filename, strerror(errno));
	    page->pagelen = strlen((char*)(page->page));
	}
    }
    if (!page)
	return 0;

    tmp = newText(x, y, width, height,
		  page->pagelen, 0, "", (char*)(page->page), callback, help);
    if (tmp) {
	tmp->item.text.class = T_IS_HTML;
	tmp->item.text.width = tmp->width;
	tmp->item.text.extra = (void*)page;
	tmp->item.text.bs    = malloc(sizeof(short)*width*height);
	memset(tmp->item.text.bs, -1, sizeof(short)*width*height);
	tmp->item.text.href  = -1;

	if (tmp->item.text.bs == 0) {
	    deleteObj(tmp);
	    tmp = 0;
	}
	if (label) {
	    /* locate ourself at the label inside this document, if that
	     * label exists.  If it doesn't exist, we'll just locate ourself
	     * at the top of the document.
	     */
	    for (pg = page->page, ix = yp = 0; ix < page->pagelen; ix++) {
		if (pg[ix] == bctID) {

		    if (pg[++ix] == bctLABEL) {
			++ix;
			if (strncmp((char*)(pg+ix), label, llen) == 0
				     && pg[ix+llen] == bctID)
			    break;
		    }
		    while (pg[ix] != bctID && ix < page->pagelen)
			++ix;
		}
		else if (pg[ix] == '\n')
		    yp++;
	    }
	    if (ix < page->pagelen)
		tmp->item.text.topy = yp;
	} /* if label */
    }

    return tmp;
} /* newHelp */


/*
 * nonpublished space-filler string used to blank-fill ends of lines
 */
extern char rillyrillylongblankstring[];

/*
 * drawTextLine() draws a single line from a Text object
 */
static void
drawTextLine(WINDOW *win, Obj *obj, int idx)
{
    int xp;
    char *p;

    for (xp=0, p = obj->item.text.lines[idx]; *p != 0 && *p != '\n'; ++p, ++xp)
	if (xp >= obj->item.text.off_x && xp < obj->item.text.off_x+obj->width)
	    waddch(win, *p);
} /* drawTextLine */


/*
 * drawHtmlLine() draws a single HTML line from a Text object
 */
static void
drawHtmlLine(WINDOW *win, Obj *obj, int yp)
{
    int x;
    unsigned char *line = (unsigned char*)(obj->item.text.lines[yp]);
    int indent = 0;
    int t;
    short href = -1;
    int wx, wy;

    wy = yp - obj->item.text.topy;

    if (*line == DLE) {
	indent = line[1]-' ';
	line += 2;		/* move start of line over the indent code */
    }
    if (obj->item.text.off_x < indent) {
	/* part of the line indent is visible, so, um, display it */

	t = indent - obj->item.text.off_x;
	if (t > obj->width)
	    /* indent fills the entire line */
	    return;
	while (t > 0) {
	    waddch(win, ' ');
	    t--;
	}
    }
    x = indent;

    for ( ; *line && *line != '\n'
		  && x < obj->item.text.off_x+obj->width;
	    ++line) {

	wx = x - obj->item.text.off_x;

	if (*line == bcfID) {
	    switch (*++line) {
#if WITH_NCURSES
	    case bcfSET_I:
			wattron(win, A_DIM);
			break;
	    case bcfCLEAR_I:
			wattroff(win, A_DIM);
			break;
	    case bcfSET_B:
			wattron(win, A_BOLD);
			break;
	    case bcfCLEAR_B:
			wattroff(win, A_BOLD);
			break;
#else
	    case bcfSET_I:
	    case bcfSET_B:
			wstandout(win);
			break;
	    case bcfCLEAR_I:
	    case bcfCLEAR_B:
			wstandend(win);
			break;
#endif
	    case bcfID:
			goto printch;
	    default:
			break;
	    }
	}
	else if (*line == bctID) {
	    if (*++line == bctID)
		goto printch;
	    if (*line == 'a')
		href = -1;
	    else if (*line == 'A')
		href = atoi((char*)(1+line));
	    while (*line != bctID)
		++line;
	}
	else
    printch:
	    if (wx >= 0) {
		if (href >= 0) {
#if WITH_NCURSES
		    wattron(win, (href==obj->item.text.href)
				       ? A_REVERSE
				       : A_UNDERLINE);
		    waddch(win, *line );
		    wattroff(win,(href==obj->item.text.href)
				       ? A_REVERSE
				       : A_UNDERLINE);
#else
		    wstandout(win);
		    waddch(win, *line);
		    wstandend(win);
#endif
		}
		else
		    waddch(win, *line );

		obj->item.text.bs[wx + (wy*obj->width)] = href;
		x++;
	    }
    }
} /* drawHtmlLine */


/*
 * drawText() draws a text box.  This should probably be a subclass of the
 * list object, but that's being left as an exercise for the future.
 */
void
drawText(void *o, void* w)
{
    Obj *obj = OBJ(o);
    int rc;
    int yp = 0;
    WINDOW *win = Window(w);
    int x = WX(w),
	y = WY(w);

    if (o == 0 || objType(o) != O_TEXT || obj->item.text.lines == 0)
	return;
    rc = _nd_drawObjCommon(obj, w);
    _nd_adjustXY(rc, obj, &x, &y);

    if (obj->item.text.bs)
	memset(obj->item.text.bs, -1, sizeof(short) * obj->width * obj->depth);

    setcolor(win, WINDOW_COLOR);
    for (yp = 0; yp < obj->depth; yp++) {
	wmove(win, y+yp, x);
	waddnstr(win, rillyrillylongblankstring, obj->width);
	wmove(win, y+yp, x);
	if (yp+obj->item.text.topy < obj->item.text.nrlines)
	    switch (obj->item.text.class) {
	    case T_IS_HTML:
		drawHtmlLine(win, obj, yp + obj->item.text.topy);
		break;
	    default:
		drawTextLine(win, obj, yp + obj->item.text.topy);
		break;
	    }
    }

    if ((rc & DREW_A_BOX) && obj->width > 3) {
	setcolor(win, WIDGET_COLOR);
	if (obj->item.text.topy > 0)
	    mvwaddstr(win, y-1, x+obj->width-3, "(-)");
	if (obj->item.text.topy + obj->depth < obj->item.text.nrlines)
	    mvwaddstr(win, y+obj->depth, x+obj->width-3, "(+)");

	if (obj->item.text.off_x)
	    mvwaddch(win, y+obj->depth-1, x-1, NT_LARROW);
	if (obj->item.text.off_x + obj->width < obj->item.text.width)
	    mvwaddch(win, y+obj->depth-1, x+obj->width, NT_RARROW);
    }

    if ((rc & DREW_A_BOX) && obj->width > 8 && obj->item.text.nrlines > 0) {
	int percent = ((obj->item.text.topy+obj->depth)*100) / obj->item.text.nrlines;
	char bfr[8];

	if (percent < 100)
	    sprintf(bfr, "%d%%", percent);
	else
	    strcpy(bfr, "100%");
	setcolor(win, WINDOW_COLOR);
	mvwaddstr(win, y+obj->depth, x+obj->width-(3+strlen(bfr)), bfr);
    }
} /* drawText */


/*
 * scan_for_tag() is a local function that looks on the current page for
 * a html tag that's different from the current tag.  If it finds it, it
 * updates item.text.href, otherwise it does nothing.
 *
 * return values:
 *		1:	found and set item.text.href to the new tag.
 *		0:	could NOT find an appropriate tag on this page.
 *
 * What it does:
 *	if item.text.href is NOT set
 *		set the first tag found and return 1 {found on this page}
 *		else return 0 {not found}
 *	else if item.text.href is on this page
 *		if we found the next tag, set it and return 1
 *		else return 0
 *	else if we found any tags at all
 *		set the first tag found and return 1
 *		else return 0
 */
static int
scan_for_tag(Obj *obj, int direction)
{
    int x, y;
    int y_off;

    short href;
    short first_href = -1;
    int no_href = (obj->item.text.href < 0);
    int found_current_here = 0;

    if (direction > 0) {
	for (y=0; y < obj->depth; y++) {
	    y_off = y * obj->width;
	    for (x=0; x < obj->width; x++) {
		if ((href = obj->item.text.bs[x + y_off]) >= 0) {

		    if (no_href) {
			obj->item.text.href = href;
			return 1;
		    }
		    if (first_href < 0)
			first_href = href;
		    if (href == obj->item.text.href)
			found_current_here++;
		    else if (href > obj->item.text.href) {
			obj->item.text.href = href;
			return 1;
		    }
		}
	    }
	}
    }
    else {
	for (y = obj->depth-1; y >= 0; y--) {
	    y_off = y * obj->width;
	    for (x = obj->width-1; x >= 0; --x) {
		if ((href = obj->item.text.bs[x + y_off]) >= 0) {
		    if (no_href) {
			obj->item.text.href = href;
			return 1;
		    }
		    if (first_href < 0)
			first_href = href;
		    if (href == obj->item.text.href)
			found_current_here++;
		    else if (href < obj->item.text.href) {
			obj->item.text.href = href;
			return 1;
		    }
		}
	    }
	}
    }
    if (first_href >= 0 && found_current_here == 0) {
	obj->item.text.href = first_href;
	return 1;
    }
    return 0;
} /* scan_for_tag */


/* convenience macros for various subclass editing functions
 */
#define TOPY		(obj->item.text.topy)
#define NRLINES		(obj->item.text.nrlines)

/*
 * editHtmlText() is a local function that handles navigation on a html page
 */
static editCode
editHtmlText(Obj *obj, void *w)
{
    register int c;
    int x=0, href;
    int cb_stat;
    int rescan_tags = 0;
    int touch = 0;

    while ((c = ndgetch(w)) != EOF) {

	switch (c) {
	case KEY_F(1):	_nd_help(objHelp(obj));	break;
	case KEY_RIGHT:
	case '\r':	/* RETURN on a html tag fires the callback */
	case '\n':	href = obj->item.text.href;

			/* except we don't want to fire the callback if
			 * the current tag isn't on the page */
			if (href >= 0)
			    for (x = (obj->depth * obj->width)-1; x>=0; --x)
				if (href == obj->item.text.bs[x])
				    break;
			if (x < 0 || href < 0)	/* can't get away if */
			    continue;	/* the tag isn't on this page */

			if ((cb_stat = _nd_callback(obj, w)) == 0)
			    continue;
			if (cb_stat != 0)
			    return (cb_stat < 0) ? eEXITFORM : eRETURN;
			break;
#if VERMIN
	case KEY_MOUSE:	return eEVENT;
#endif
	case 'R'-'@':	return eREFRESH;
	case KEY_LEFT:
	case ESCAPE:	return eESCAPE;
	case KEY_BTAB:
	case KEY_UP:	if (scan_for_tag(obj, -1) == 0) {
			    TOPY -= obj->depth;
			    if (TOPY < 0)
				TOPY = 0;
			    rescan_tags = -1;
			}
			touch++;
			break;
	case KEY_DOWN:
	case '\t':	if (scan_for_tag(obj, 1) == 0) {
			    TOPY += obj->depth;
			    if (TOPY + obj->depth > NRLINES) {
				TOPY = NRLINES - obj->depth;
				if (TOPY < 0)	/* paranoia */
				    TOPY = 0;
			    }
			    rescan_tags = 1;
			}
			touch++;
			break;

	case 'U'-'@':
	case KEY_PPAGE:			/* PAGE UP */
	    TOPY -= obj->depth;
	    if (TOPY < 0)
	case KEY_HOME:			/* HOME.  And, yes, this is supposed */
		TOPY = 0;		/* to be here */
	    touch++;
	    break;

	case 'D'-'@':
	case KEY_NPAGE:			/* PAGE DOWN */
	    TOPY += obj->depth;
	    if (TOPY + obj->depth > NRLINES) {
	case KEY_END:			/* END.  See KEY_HOME comment */
		TOPY = NRLINES - obj->depth;
		if (TOPY < 0)	/* paranoia */
		    TOPY = 0;
	    }
	    touch++;
	    break;

	case '>':
	    if (obj->item.text.off_x + obj->width < obj->item.text.width) {
		obj->item.text.off_x ++;
		touch++;
	    }
	    break;

	case '<':
	    if (obj->item.text.off_x > 0) {
		obj->item.text.off_x--;
		touch++;
	    }
	    break;
	}

	if (touch) {
	    drawObj(obj, w);
	    if (rescan_tags) {
		if (scan_for_tag(obj, rescan_tags) == 1)
		    drawObj(obj, w);
		rescan_tags = 0;
	    }
	    touch = 0;
	}
    }
    return eESCAPE;
} /* editHtmlCode */


/*
 * editPlainText() is a local function that edits a regular text object
 */
static editCode
editPlainText(Obj* obj, void *w)
{
    register int c;
    int touch = 0;

    while ((c = ndgetch(w)) != EOF) {

	switch (c) {
	case KEY_F(1):	_nd_help(objHelp(obj));	break;
#if VERMIN
	case KEY_MOUSE:	return eEVENT;
#endif
	case 'R'-'@':	return eREFRESH;
	case 'Q':
	case ESCAPE:	return eESCAPE;
	case KEY_BTAB:
	case KEY_LEFT:	return eBACKTAB;
	case KEY_RIGHT:
	case '\t':	return eTAB;

	case 'U'-'@':
	case KEY_PPAGE:			/* PAGE UP */
	    TOPY -= obj->depth;
	    if (TOPY < 0)
	case KEY_HOME:			/* HOME.  And, yes, this is supposed */
		TOPY = 0;		/* to be here */
	    touch++;
	    break;

	case '-':
	case KEY_UP:
	    if (TOPY == 0)
		continue;
	    TOPY--;
	    touch++;
	    break;
	
	case 'D'-'@':
	case KEY_NPAGE:			/* PAGE DOWN */
	    TOPY += obj->depth;
	    if (TOPY + obj->depth > NRLINES) {
	case KEY_END:			/* END.  See KEY_HOME comment */
		TOPY = NRLINES - obj->depth;
		if (TOPY < 0)	/* paranoia */
		    TOPY = 0;
	    }
	    touch++;
	    break;

	case '>':
	    if (obj->item.text.off_x + obj->width < obj->item.text.width) {
		obj->item.text.off_x ++;
		touch++;
	    }
	    break;

	case '<':
	    if (obj->item.text.off_x > 0) {
		obj->item.text.off_x--;
		touch++;
	    }
	    break;

	case '+':
	case KEY_DOWN:
	    if (TOPY < NRLINES-obj->depth) {
		TOPY++;
		touch++;
	    }
	    break;
	}

	if (touch) {
	    drawObj(obj, w);
	    touch = 0;
	}
    }
    return eESCAPE;
} /* editPlainText */


/*
 * editText() does editing operations on a Text Object and its subclasses
 */
editCode
editText(void* o, void* w, MEVENT* ev, editCode cc)
{
    Obj *obj = OBJ(o);
#if HAVE_CURS_SET
    int cursor_visibility;	/* hide the cursor when doing a menu */
#endif
    editCode rc = eESCAPE;	/* default to bailing out */

    if (obj == 0 || obj->Class != O_TEXT || obj->item.text.lines == 0) {
	errno = EINVAL;
	return eERROR;
    }

#if HAVE_CURS_SET
    /*
     * set up the initial cursor position and handle any mouse
     * events that may have sent us here.
     */
    cursor_visibility = curs_set(0);
#endif

#if VERMIN
    if (cc == eEVENT) {
	int cb_stat;
	int dy;			/* delta y for page up/page down movement */

	/* need to do initial positioning from mouse events */
	int yp = (ev->y - obj->dty);
	int xp = (ev->x - obj->dtx);

	if (yp < 0) {
	    /* scroll backwards */
	    dy = obj->depth;
	    TOPY -= dy;
	    if (TOPY < 0)
		TOPY = 0;
	}
	else if (yp == obj->depth) {
	    /* scroll forwards */
	    dy = obj->depth;
	    yp = TOPY+obj->depth;

	    if (yp+dy >= obj->item.list.nritems)
		dy = NRLINES-yp;

	    TOPY += dy;
	}
	else if (xp < 0) {
	    obj->item.text.off_x -= obj->width/2;
	    if (obj->item.text.off_x < 0)
		obj->item.text.off_x = 0;
	}
	else if (xp == obj->width && obj->item.text.off_x+obj->width < obj->item.text.width)
	    obj->item.text.off_x += obj->width/2;
	else if (obj->item.text.class == T_IS_HTML) {
	    if (obj->item.text.bs[xp + (yp*obj->width)] != -1) {
		obj->item.text.href = obj->item.text.bs[xp + (yp*obj->width)];

		if (ev->bstate & BUTTON1_DOUBLE_CLICKED)
		    if ((cb_stat = _nd_callback(obj, w)) != 0)
			return (cb_stat < 0) ? eEXITFORM : eRETURN;
	    }
	}
	drawObj(obj, w);
    }
#endif
    /* and away we go! */

    switch (obj->item.text.class) {
    case T_IS_HTML:
	rc = editHtmlText(obj, w);
	break;
    default:
	rc = editPlainText(obj, w);
	break;
    }
#if HAVE_CURS_SET
    curs_set(cursor_visibility);
#endif
    return rc;
} /* editText */



/*
 * a HelpCursor is a private structure that lets callers set and restore
 * their current location in a helpfile
 */
struct HelpCursor {
    int topy;		/* Y offset */
    int off_x;		/* X offset */
    int href;		/* and current href */
} ;

/*
 * getHelpCursor() returns a pointer to a help cursor
 */
void *
getHelpCursor(void *o)
{
    Obj *obj = OBJ(o);
    struct HelpCursor* tmp;

    if (o == 0 || objType(o) != O_TEXT)
	return 0;
    
    if ((tmp = malloc(sizeof *tmp)) == 0)
	return 0;
    tmp->topy  = obj->item.text.topy;
    tmp->off_x = obj->item.text.off_x;
    tmp->href  = obj->item.text.href;

    return tmp;
} /* getHelpCursor */


/*
 * setHelpCursor() sets the help cursor
 */
int
setHelpCursor(void* o, void* c)
{
    Obj *obj = OBJ(o);
    struct HelpCursor *cursor = (struct HelpCursor*)c;

    if (o == 0 || objType(o) != O_TEXT || cursor == 0)
	return 0;

    if (cursor->topy > obj->item.text.nrlines)
	return 0;

    obj->item.text.topy  = cursor->topy;
    obj->item.text.off_x = cursor->off_x;
    obj->item.text.href  = cursor->href;
    return 1;
} /* setHelpCursor */

/*
 * setTextCursor() sets the cursor from the outside world
 */
int
setTextCursor(Obj *obj, int position)
{
    if (position == -1) {
	TOPY = NRLINES - obj->depth;
	if (TOPY < 0)
	    TOPY = 0;
	obj->item.text.off_x =  0;
    }
    else if (position == 0)
	TOPY = obj->item.text.off_x = 0;
    else {
	errno = ERANGE;
	return -1;
    }
    if (obj->item.text.class == T_IS_HTML) {
	obj->item.text.href = -1;
	scan_for_tag(obj, 1);
    }
    return 1;
} /* setTextCursor */
