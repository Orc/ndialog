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
 * new dialog library: utility functions.
 */
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include "nd_objects.h"
#include "ndwin.h"
#include "html.h"

/*-----------------------------------------------*
 *                                               *
 *       U T I L I T Y   F U N C T I O N S       *
 *                                               *
 *-----------------------------------------------*/

/*
 * objType() returns the object type
 */
enum Class
objType(void *o)
{
    if (o == 0)
	return O_ERROR;
    return OBJ(o)->Class;
} /* objType */


/*
 * objId() returns a string saying what type of object this is
 */
char *
objId(void *o)
{
    switch ( OBJ(o)->Class) {
    case O_STRING:	if (OBJ(o)->flags & PASSWORD_STRING)
			    return "password";
			else
			    return "string";
    case O_CHECK:	return "checkbox";
    case O_BUTTON:	switch (OBJ(o)->item.button.kind) {
			case CANCEL_BUTTON:	return "cancel button";
			case CONFIRM_BUTTON:	return "ok button";
			default:		return "button";
			}
    case O_LIST:	return "list";
    case O_TEXT:	return "text";
    default:		return "widget";
    }
} /* objId */

/*
 * set the title on an object
 */
void
setObjTitle(void *obj, char *title)
{
    if (obj == 0)
	errno = EINVAL;
    else {
	if (OBJ(obj)->title)
	    free(OBJ(obj)->title);
	OBJ(obj)->title = strdup(title);
    }
} /* setObjTitle */


/*
 * objTitle() tells us the current object title
 */
char *
objTitle(void *obj)
{
    return obj ? OBJ(obj)->title : 0;
} /* objTitle */


/*
 * newTextData() is an unpublished procedure that
 * builds the lines[] list for a Text object (used by newText() and
 * setObjData())
 */
int
newTextData(Obj *o)
{
    int nrlines = 0;
    int i;
    char *p;
    int width = 0;

    for (p=(char*)(o->content); *p; ++p)
	if (*p == '\n')
	    nrlines++;
    if (p > (char*)(o->content) && p[-1] != '\n')
	nrlines++;
    if ((o->item.text.lines = malloc(nrlines * sizeof(char*))) == 0)
	return -1;
    o->item.text.nrlines = nrlines;
    o->item.text.width = 0;
    for (p=(char*)(o->content), i=0; i<nrlines; i++) {
	o->item.text.lines[i] = p;
	if ((p = strchr(p, '\n')) == (char*)0) {
	    if ((width = strlen(o->item.text.lines[i])) > o->item.text.width)
		o->item.text.width = width;
	    break;
	}
	else {
	    if ((width = (p - o->item.text.lines[i])) > o->item.text.width)
		o->item.text.width = width;
	    ++p;
	}
    }

    if (i < nrlines)
	o->item.text.nrlines = i;
    return nrlines;
} /* newTextData */


#if DYNAMIC_BINDING
/*
 * nd_bindToType() binds something to an object's
 * content pointer.
 */
va_list*
nd_bindToType(void *obj, va_list *ptr)
{
    OBJ(obj)->content = va_arg(*ptr, void*);
    return ptr;
} /* nd_bindToString */

/*
 * nd_bindToString() binds a string to an O_STRING object
 */
va_list*
nd_bindToString(void *obj, va_list *ptr)
{
    ptr = nd_bindToType(obj, ptr);
    OBJ(obj)->item.string.maxlen = va_arg(*ptr, int);
    OBJ(obj)->item.string.startx = 0;
    OBJ(obj)->item.string.curx   = 0;
    return ptr;
} /* nd_bindToString */


/*
 * nd_bindToList() binds a ListItem array to an O_LIST object
 */
va_list*
nd_bindToList(void *obj, va_list *ptr)
{
    va_list *ret = nd_bindToType(obj, ptr);

    if ( ret ) {
	OBJ(obj)->item.list.items    = va_arg(*ret, ListItem*);
	OBJ(obj)->item.list.nritems  = va_arg(*ret, int);
	OBJ(obj)->item.list.topy     = 0;
	OBJ(obj)->item.list.cury     = 0;
    }
    return ret;
} /* nd_bindToList */


/*
 * nd_bindToText() binds new text to an O_TEXT object
 */
va_list*
nd_bindToText(void *obj, va_list *ptr)
{
    va_list *ret = nd_bindToType(obj,ptr);

    if ( ret ) {
	if (OBJ(obj)->item.text.lines)
	    free(OBJ(obj)->item.text.lines);
	newTextData(obj);
	OBJ(obj)->item.text.topy   = 0;
	OBJ(obj)->item.text.off_x  = 0;
    }
    return ret;
} /* nd_bindToText */
#endif


/*
 * setObjData() binds the object to some particular variable.
 */
void
setObjData(void *obj,...)
{
    va_list ptr;
    
    if (obj == 0)
	return;

    UNTOUCH_OBJ(obj);
    va_start(ptr, obj);
#if DYNAMIC_BINDING
    if (nd_object_table[OBJ(obj)->Class].bind)
	(nd_object_table[OBJ(obj)->Class].bind)(obj, &ptr);
#else
    OBJ(obj)->content = va_arg(ptr, void*);
    switch (OBJ(obj)->Class) {
    case O_STRING:
		OBJ(obj)->item.string.maxlen = va_arg(ptr, int);
		OBJ(obj)->item.string.startx = 0;
		OBJ(obj)->item.string.curx   = 0;
		break;
    case O_LIST:
		OBJ(obj)->item.list.items    = va_arg(ptr, ListItem*);
		OBJ(obj)->item.list.nritems  = va_arg(ptr, int);
		OBJ(obj)->item.list.topy     = 0;
		OBJ(obj)->item.list.cury     = 0;
		break;
    case O_TEXT:
		if (OBJ(obj)->item.text.lines)
		    free(OBJ(obj)->item.text.lines);
		newTextData(obj);
		OBJ(obj)->item.text.topy   = 0;
		OBJ(obj)->item.text.off_x  = 0;
		break;
    default:
		break;
    }
#endif
    va_end(ptr);
} /* setObjData */


/*
 * setObjHelp() sets the help information for an object
 */
void
setObjHelp(void *obj, char *help)
{
    if (obj == 0)
	return;
    OBJ(obj)->help = help;
} /* setObjHelp */


/*
 * objHelp() returns the help information from an object
 */
char *
objHelp(void *o)
{
    Obj *obj = OBJ(o);

    if (objType(o)==O_LIST && obj->item.list.items && obj->item.list.items[obj->item.list.cury].help)
	return obj->item.list.items[obj->item.list.cury].help;
    return obj ? obj->help : 0;
} /* objHelp */


/*
 * isOKbutton() tells us if this object is an OK button
 */
int
isOKbutton(void *obj)
{
    return obj && objType(obj) == O_BUTTON
	       && OBJ(obj)->item.button.kind == CONFIRM_BUTTON;
} /* isOKbutton */


/*
 * isCANCELbutton() tells us if this object is a CANCEL button
 */
int
isCANCELbutton(void *obj)
{
    return obj && objType(obj) == O_BUTTON
	       && OBJ(obj)->item.button.kind == CANCEL_BUTTON;
} /* isCANCELbutton */


/*
 * setButtonDataArea() sets the data area for a button
 */
int
setButtonDataArea(void *obj, int x, int y)
{
    if (obj == 0 || objType(obj) != O_BUTTON) {
	errno = EINVAL;
	return -1;
    }
    OBJ(obj)->dtx = OBJ(obj)->selx = x;
    OBJ(obj)->dty = OBJ(obj)->sely = y;
    return 0;
} /* setButtonDataArea */


/*
 * currentSelection() tells us which list item we're pointing at
 */
int
currentSelection(void *obj)
{
    if (obj && objType(obj) == O_LIST) return OBJ(obj)->item.list.cury;
    errno = EINVAL;
    return -1;
} /* currentSelection */


/*
 * getObjList() returns a pointer to the list associated with a list
 * object
 */
ListItem*
getObjList(void *obj)
{
    if (obj && objType(obj) == O_LIST) return OBJ(obj)->item.list.items;
    errno = EINVAL;
    return (ListItem*)0;
} /* getObjList */


/*
 * getObjListSize() returns the number of items in the list associated with
 * a list object.
 */
int
getObjListSize(void *obj)
{
    if (obj && objType(obj) == O_LIST) return OBJ(obj)->item.list.nritems;
    errno = EINVAL;
    return -1;
} /* getObjList */


/*
 * setWritable() makes this object read/write
 */
void
setWritable(void *obj)
{
    if (obj) { RW_OBJ(obj); touchObj(obj); }
} /* setWritable */


/*
 * setReadonly() makes the object readonly
 */
void
setReadonly(void* obj)
{
    if (obj) { RO_OBJ(obj); touchObj(obj); }
} /* setReadonly */


/*
 * writable() tells us whether the object is read/write or not
 */
int
writable(void* obj)
{
    return obj && !(OBJ_READONLY(obj) || HIDDEN_OBJ(obj));
} /* writable */


/*
 * hideObj() makes this object invisible
 */
void
hideObj(void* obj)
{
    if (obj) OBJ(obj)->flags |= OBJ_HIDDEN;
} /* hideObj */


void
unhideObj(void* obj)
{
    if (obj) OBJ(obj)->flags &= ~OBJ_HIDDEN;
} /* unhideObj */


/*
 * touchObj() marks this object as needing to be redisplayed the next
 * time anyone gets around to it.
 */
void
touchObj(void* obj)
{
    if (obj) {
	OBJ(obj)->flags |= OBJ_WRITTEN;
	OBJ(obj)->flags &= ~OBJ_DIRTY;
    }
} /* touchObj */


/*
 * untouchObj() marks this object as NOT needing to be redisplayed the
 * next time the song gets around to it.
 */
void
untouchObj(void* obj)
{
    if (obj) OBJ(obj)->flags &= ~(OBJ_WRITTEN|OBJ_DIRTY);
} /* untouchObj */


/*
 * touched() tells us if this object needs to be redisplayed
 */
int
touched(void *obj)
{
    return obj && (OBJ(obj)->flags & OBJ_WRITTEN);
} /* touched */


/*
 * isDirty() tells us if the object has been scribbled upon
 */
int
isDirty(void *obj)
{
    return obj && (OBJ(obj)->flags & OBJ_DIRTY);
} /* isDirty */


/*
 * setObjCursor() sets the cursor for a cursor-addressable Object
 */
int
setObjCursor(void *o, int cursor)
{
    nd_setp setp;

    if (o == 0) {
	errno = EFAULT;
	return -1;
    }
    else if ( (setp = nd_object_table[OBJ(o)->Class].setp) != 0)
	return (*setp)(o, cursor);

    errno = EINVAL;
    return -1;
} /* setObjCursor */


/*
 * setListCursor() sets the cursor for a List
 */
int
setListCursor(Obj *obj, int cursor)
{
    if (cursor < 0 || cursor >= obj->item.list.nritems) {
	errno = EOVERFLOW;
	return -1;
    }
    obj->item.list.cury = cursor;
    if (cursor < obj->item.list.topy)
	obj->item.list.topy = cursor;
    else if (cursor >= obj->item.list.topy+obj->depth)
	obj->item.list.topy = (cursor - obj->depth) + 1;
    return 0;
} /* setListCursor */


/*
 * setStringCursor() does what you'd expect for a String
 */
int
setStringCursor(Obj *obj, int cursor)
{
    if (cursor < 0 || cursor >= obj->item.string.maxlen) {
	errno = EOVERFLOW;
	return -1;
    }
    obj->item.string.curx = cursor;
    if (cursor < obj->item.string.startx)
	obj->item.string.startx = cursor;
    else if (cursor >= obj->item.string.startx+obj->width)
	obj->item.string.startx = (cursor - obj->width) + 1;
    return 0;
} /* setStringCursor */


/*
 * getObjCursor() returns the current cursor location
 */
int
getObjCursor(void *o)
{
    Obj* obj = OBJ(o);
    nd_getp getp;

    if (obj == 0) {
	errno = EFAULT;
	return -1;
    }
    else if ( (getp = nd_object_table[OBJ(o)->Class].getp) != 0)
	return (*getp)(o);
    errno = EINVAL;
    return -1;
} /* getObjCursor */


/*
 * getStringCursor() does what you'd expect
 */
int
getStringCursor(Obj *o)
{
    return o->item.string.curx;
} /* getStringCursor */


/*
 * getListCursor() also does what you'd expect
 */
int
getListCursor(Obj *o)
{
    return o->item.list.cury;
} /* getListCursor */


/*
 * objDataPtr() returns obj->content
 */
void *
objDataPtr(void *o)
{
    Obj *obj = OBJ(o);

    return o ? obj->content : 0;
} /* objDataPtr */


/*
 * getHelpTopic() returns the title of a helpfile
 */
char*
getHelpTopic(void *o)
{
    Obj *obj = OBJ(o);

    if (o && objType(o) == O_TEXT && obj->item.text.class == T_IS_HTML)
	return ((Page*)(obj->item.text.extra))->title;

    return 0;
} /* getHelpTopic */


/*
 * currentHtmlTag() returns the selected html tag from
 * a Help object.
 */
char*
currentHtmlTag(void *o)
{
    Obj *obj = OBJ(o);
    Page *ptr;

    if (o && objType(o) == O_TEXT && obj->item.text.class == T_IS_HTML)
	if (obj->item.text.href >= 0) {
	    ptr = (Page*)(obj->item.text.extra);

	    if (ptr && ptr->nrhrefs >= obj->item.text.href)
		return ptr->hrefs[obj->item.text.href];
	}
    return 0;
} /* currentHtmlTag */


/*
 * the struct _LIA is the internal representation of the LIA object.
 * We define the structure here for object-oriented Purity Of Essence,
 * and also because it's likely to change drastically and we don't want
 * users assuming any format
 */
struct _LIA {
    ListItem *list;		/* the ListItem array */
    int count;			/* how many items live in the array */
    int changed : 1;		/* has the array been changed?  If it
				 * hasn't, we're pointing at the array
				 * that was passed in to us, otherwise
				 * we're pointing at a newly allocated
				 * one
				 */
} ;

/*
 * newLIA() creates a new listItem array
 */
LIA
newLIA(ListItem *list, int size)
{
    struct _LIA* tmp;

    if ((tmp = malloc(sizeof *tmp)) != 0) {
	tmp->count = size;
	tmp->list = list;
	tmp->changed = 0;
    }
    return tmp;
} /* newLia */


/*
 * deleteLIA() deletes a LIA and the ListItem array it contains
 */
void
deleteLIA(LIA o)
{
    struct _LIA *obj = (struct _LIA*)o;
    int i;

    if (o != 0) {
	if (obj->list && obj->changed) {
	    for (i=0; i<obj->count; i++) {
		if (obj->list[i].id)
		    free(obj->list[i].id);
		free(obj->list[i].item);
		if (obj->list[i].help)
		    free(obj->list[i].help);
	    }
	    free(obj->list);
	}
	free(obj);
    }
} /* deleteLIA */


/*
 * LIAallocate() allocates a writable ListItem array for a LIA
 */
void
LIAallocate(struct _LIA* obj)
{
    int i;
    ListItem *tmp;

    tmp = malloc( (obj->count+1) * sizeof tmp[0] );
    for (i=0; i<obj->count; i++) {
	tmp[i].id   = obj->list[i].id ? strdup(obj->list[i].id) : 0;
	tmp[i].item = strdup(obj->list[i].item);
	tmp[i].help = obj->list[i].help ? strdup(obj->list[i].help) : 0;
    }
    obj->changed = 1;
    obj->list = tmp;
} /* LIAallocate */


/*
 * addToLIA() unconditionally adds a ListItem to a LIA
 */
int
addToLIA(LIA o, char *id, char *item, char *help)
{
    struct _LIA *obj = (struct _LIA*)o;

    if (o == 0 || item == 0) {
	errno = EFAULT;
	return EOF;
    }
    if (obj->changed)
	obj->list = realloc(obj->list, (1+obj->count) * sizeof obj->list[0]);
    else
	LIAallocate(obj);

    if (obj->list == 0) {
	obj->count = 0;
	return EOF;
    }
    obj->list[obj->count].id   = id ? strdup(id) : 0;
    obj->list[obj->count].item = strdup(item);
    obj->list[obj->count].help = help ? strdup(help) : 0;
    obj->list[obj->count].selected = 0;
    return ++(obj->count);
} /* addListItem */


/*
 * delFromLIA() deleted a numbered ListItem from a LIA
 */
int
delFromLIA(LIA o, int del)
{
    struct _LIA *obj = (struct _LIA*)o;
    int x;

    if (o == 0 || del < 0 || del >= obj->count)
	return EOF;

    if (obj->changed == 0)
	LIAallocate(obj);

    if (obj->list[del].id)
	free(obj->list[del].id);
    if (obj->list[del].help)
	free(obj->list[del].help);
    free(obj->list[del].item);

    for (x=del; x < (obj->count-1); x++)
	obj->list[x] = obj->list[x+1];

    return --(obj->count);
} /* delListItem */


/*
 * LIAcount() returns how many items live in the given LIA
 */
int
LIAcount(LIA o)
{
    return (o != 0) ? (((struct _LIA*)o)->count) : 0;
} /* LIAcount */


/*
 * LIAlist() returns the array of ListItems
 */
ListItem*
LIAlist(LIA o)
{
    return (o != 0) ? (((struct _LIA*)o)->list) : 0;
} /* LIAlist */


/*
 * _nd_inside() tells us whether a mousepress is inside an object's data
 * entry area. (If we don't have mouse support, it becomes a macro
 * that always returns 0.)
 */

#if VERMIN

int
_nd_inside(Obj *obj, MEVENT *m)
{
    /* we can select an object if it's NOT readonly (TODO: we need to
     * be able to pick up scrollbars no matter what) and if we're inside
     * its hot spot.
     */
    return !(OBJ_READONLY(obj) || HIDDEN_OBJ(obj))
	    && m->x >= obj->selx && m->x < obj->selx + obj->selwidth
	    && m->y >= obj->sely && m->y < obj->sely + obj->seldepth;
}

#else

int
_nd_inside(Obj *obj, MEVENT *m)
{
    return 0;
}
#endif


/*
 * _nd_setflag() sets a flag bit in the object flags array
 */
int
_nd_setflag(Obj *o, int bit)
{
    return o->flags |= bit;
} /* _nd_setflag */


/*
 * _nd_clearflag() clears a flag bit inthe object flags array
 */
int
_nd_clearflag(Obj *o, int bit)
{
    return o->flags &= ~bit;
} /* _nd_clearflag */


/*
 * ndDisplay things
 */

/*
 * newDisplay creates a brand new Display
 */
ndDisplay
newDisplay(void* win, int x, int y)
{
    Display *ptr;

    if ((ptr = malloc(sizeof *ptr)) != 0) {
	ptr->window = win;
	ptr->x = x;
	ptr->y = y;
    }
    return ptr;
} /* newDisplay */


/*
 * deleteDisplay() throws a display away
 */
void
deleteDisplay(Display *junk)
{
    free(junk);
} /* deleteDisplay */



/*
 * ndgetch() gets input from the display.  Currently, we merely map meta
 * keys to KEY_F(x) combinations (M digit => KEY_F(digit) is the only one
 * implemented), but if we want to do more sophisticated input in the
 * future (like grabbing ALT keys from a IBM PC console), we will do it
 * here.
 */
int
ndgetch(Display *from)
{
    int c;

#if WITH_BSD_CURSES
    wrefresh(Window(from));
#endif
    c = ndwgetch(Window(from));

    if (c == 'X'-'@') {
	/* meta key; do one-character lookahead */
	c = ndwgetch(Window(from));

	switch (c) {
	case '1': case '2': case '3': case '4': case '5':
	case '6': case '7': case '8': case '9':
			return KEY_F(c-'0');
	case '\t':	return KEY_BTAB;
	case 'x':	return KEY_DC;
	case 'j':	return KEY_DOWN;
	case 'g':	return KEY_END;
	case 's':	return KEY_HOME;
	case 'i':	return KEY_IC;
	case 'h':	return KEY_LEFT;
	case 'f':	return KEY_NPAGE;
	case 'u':	return KEY_PPAGE;
	case 'l':	return KEY_RIGHT;
	case 'k':	return KEY_UP;
	default:	return c;
	}
    }
#if !WITH_NCURSES
    else if (c == erasechar())
	return KEY_BACKSPACE;
#endif
#if 1 /* kludge for loadandgo */
    else if (c == '\177' || c == '\010')
	return KEY_BACKSPACE;
#endif
    return c;
} /* ndgetch */



#if !HAVE_WATTR_SET
void
setcolor(WINDOW *w, int c)
{
    if (c & CURRENT_COLOR)
	wstandout(w);
    else
	wstandend(w);
}
#endif

#if !HAVE_WADDNSTR
void
waddnstr(WINDOW *w, char *s, int len)
{
    int i;

    for (i=0; s[i] && i < len; i++)
	waddch(w, s[i]);
}
#endif

#if !HAVE_BEEP
void
beep()
{
    write(fileno(stdout), "\007", 1);
}
#endif


/*
 * from_dialog() switches the terminal settings back to what they were
 *               before dialog started.
 */
void
from_dialog()
{
    echo();
    nl();
    noraw();
    resetty();
} /* from_dialog */


/*
 * to_dialog() return to dialog mode.
 */
void
to_dialog()
{
    savetty();
    noecho();
    nonl();
    raw();
#if !HAVE_KEYPAD
    if (KS) { fputs(KS,stdout); fflush(stdout); }
#endif
} /* to_dialog() */
