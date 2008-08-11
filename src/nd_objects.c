/*
 *   Copyright (c) 1996-2001 David Parsons. All rights reserved.
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
 * new dialog library: object primitives.
 */
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>

#include "nd_objects.h"
#include "ndwin.h"
#include "html.h"


/*-----------------------------------------------*
 *                                               *
 *            C O N S T R U C T O R S            *
 *                                               *
 *-----------------------------------------------*/

/* 
 * _nd_newObj() creates a new Obj and populates common fields
 */
Obj *
_nd_newObj(pfo callback, enum Class Class, void *data, char *title,
           char *prefsuff, int x, int y, int width, int depth, char *help)
{
    Obj *tmp = (Obj*)malloc(sizeof(Obj));
    char *sep;
    int prefix=0;
    int suffix=0;

    if (tmp == (Obj*)0)
	return tmp;

    memset(tmp, 0, sizeof *tmp);

    /*
     * first, deal with the prefix/suffix special case 
     *
     * prefixes and suffixes are passed in as one string, with a
     * | separating them.  We pick them apart into their constituent
     * pieces here.
     */
    if (prefsuff) {
	if ((sep = strchr(prefsuff, '|')) != 0) {
	    if (sep > prefsuff) {
		prefix = 1;
		if ((tmp->prefix = malloc(1+(sep-prefsuff))) != (char*)0) {
		    memcpy(tmp->prefix, prefsuff, (sep-prefsuff));
		    tmp->prefix[sep-prefsuff] = 0;
		}
	    }
	    suffix = 1;
	    tmp->suffix = strdup(1+sep);
	}
	else {
	    prefix = 1;
	    tmp->prefix = strdup(prefsuff);
	}
    }

    /* check to make sure all the memory allocations worked; bomb out if they
     * didn't
     */
    if (prefix && !tmp->prefix) {
	deleteObj(tmp);
	return 0;
    }
    if (suffix && !tmp->suffix) {
	deleteObj(tmp);
	return 0;
    }

    /* set up the title */
    if (title && (tmp->title = strdup(title)) == (char*)0) {
	deleteObj(tmp);
	return 0;
    }

    /* set up x/y locations of the object and the data area in the
     * object (these are bogus for O_BUTTON objects)
     */
    tmp->dtx = tmp->x = x;
    tmp->dty = tmp->y = y;

    if (tmp->title)
	tmp->dty ++;
    if (Class == O_CHECK)
	tmp->dtx ++;
    else if (tmp->prefix || tmp->suffix || Class == O_TEXT) {
	tmp->dty ++;
	tmp->dtx ++;
    }
    if (tmp->prefix)
	tmp->dtx += strlen(tmp->prefix);

    /* all the other fields are just assignments */
    tmp->Class = Class;
    tmp->width = width;
    tmp->depth = depth;
    tmp->selx = tmp->dtx;
    tmp->sely = tmp->dty;
    tmp->selwidth = width;
    tmp->seldepth = depth;
    tmp->content = data;
    tmp->help = help;
    tmp->callback = callback;
    tmp->parent = 0;

    tmp->flags = OBJ_WRITABLE;

    /* ta da! */
    return tmp;
} /* _nd_newObj */


/*
 * copyObj() duplicates an object
 */
void *
copyObj(void *ob)
{
    Obj *tmp = malloc(sizeof *tmp);

    if (tmp == 0)
	return 0;

    memcpy(tmp, ob, sizeof *tmp);
    tmp->title = tmp->prefix = tmp->suffix = 0;

    if (OBJ(ob)->title && (tmp->title = strdup(OBJ(ob)->title)) == (char*)0) {
	deleteObj(tmp);
	return 0;
    }
    if (OBJ(ob)->prefix && (tmp->prefix = strdup(OBJ(ob)->prefix)) == (char*)0){
	deleteObj(tmp);
	return 0;
    }
    if (OBJ(ob)->prefix && (tmp->prefix = strdup(OBJ(ob)->prefix)) == (char*)0){
	deleteObj(tmp);
	return 0;
    }
    tmp->next = tmp->prev = 0;

    return tmp;
} /* copyObj */


/*
 * newString() creates a new string
 */
void*
newString(int x, int y, int width, int size, char *bfr,
	    char *prompt, char *prefix, pfo callback, char *help)
{
    Obj *tmp;

    if (width > size)
	width = size;

    tmp = _nd_newObj(callback,O_STRING,bfr,prompt,prefix,x,y,width,1,help);

    if (tmp) {
	tmp->item.string.maxlen = size;
	if (bfr) {
	    /* If a data field is supplied, put the cursor at the
	     * END of the data field.
	     */
	    int curx = strlen(bfr);
	    tmp->item.string.curx = curx;
	    if (curx > width)
		tmp->item.string.startx = (curx-width)+1;
	}
	if (tmp->prefix || tmp->suffix) {
	    /* if we're a boxed string, we may have scroll tabs that
	     * we can click upon.
	     */
	    tmp->selx --;
	    tmp->selwidth += 2;
	}
    }
    return tmp;
} /* newString */


/*
 * newPWString() creates a new string for a hidden entry field
 */
void*
newPWString(int x, int y, int width, int size, char *bfr,
	    char *prompt, char *prefix, pfo cb, char *help)
{
    Obj *tmp = newString(x, y, width, size, bfr, prompt, prefix, cb, help);

    if (tmp != (Obj*)0)
	tmp->flags |= PASSWORD_STRING;

    return tmp;
} /* newPWString */


/*
 * newCheck() creates a new check button
 */
void*
newCheck(int x, int y, char* prompt, char* prefix, char* bfr,
          pfo callback, char *help)
{
    Obj *tmp = _nd_newObj(callback,O_CHECK,bfr,prompt,prefix,x,y,1,1,help);

    return tmp;
} /* newCheck */


/*
 * newButton() creates a new button object
 */
void*
newButton(int x, char* prompt, pfo callback, char *help)
{
    Obj *tmp;

    /* buttons _must_ be titled */
    if (prompt == 0)
	return 0;

    tmp = _nd_newObj(callback,O_BUTTON,0,prompt,0,x,0,strlen(prompt),1,help);

    if (tmp != (Obj*)0) {
	tmp->dtx = tmp->dty = 0;
	tmp->item.button.kind = REGULAR_BUTTON;
    }

    return tmp;
} /* newButton */


/*
 * newCancelButton() creates a new CANCEL button object
 */
void*
newCancelButton(int x, char* prompt, pfo callback, char *help)
{
    Obj *tmp = (Obj*)newButton(x,prompt,callback,help);

    if (tmp != (Obj*)0)
	tmp->item.button.kind = CANCEL_BUTTON;
    return tmp;
} /* newCancelButton */


/*
 * newOKButton() creates a new CONFIRM button object
 */
void*
newOKButton(int x, char* prompt, pfo callback, char *help)
{
    Obj *tmp = (Obj*)newButton(x,prompt,callback,help);

    if (tmp != (Obj*)0)
	tmp->item.button.kind = CONFIRM_BUTTON;
    return tmp;
} /* newOKButton */


/*
 * newList() creates a new list object
 */
void *
newList(int x, int y, int width, int depth, int nritems, ListItem *items,
	char *prompt, char *prefix, int flags, pfo callback, char *help)
{
    Obj *tmp;
    int ix, iw, itemoffset = 0, computedwidth, itemwidth, idwidth;

#if 0
    if (items == 0) {
	errno = EFAULT;
	return 0;
    }
#endif
    if (depth < 0)
	depth = nritems;

    if (items) {
	for (idwidth=itemwidth=ix=0; ix < nritems; ix++) {
	    if ((iw=strlen(items[ix].item)) > itemwidth)
		itemwidth = iw;
	    if ((flags & SHOW_IDS) && ((iw=strlen(items[ix].id)) > idwidth))
		idwidth = iw;
	}

	switch (flags & LO_SELECTION_MASK) {
	case CHECK_SELECTED:	computedwidth = itemoffset = 4;		break;
	case HIGHLIGHT_SELECTED:computedwidth = 2; itemoffset = 1;	break;
	default:		computedwidth = itemoffset = 0;		break;
	}

	if (flags & SHOW_IDS) {
	    computedwidth += idwidth + 2;
	    itemoffset += idwidth + 2;
	}
	computedwidth += itemwidth;

	if (width < 0)
	    width = computedwidth;
	else if (width < computedwidth) {
	    if (flags & DONT_CLIP_ITEMS) {
		errno = EINVAL;
		return 0;
	    }
	}
    }
    else if (depth < 0 || width < 0) {
	/* can't autosize the list if there are no items in it */
	errno = EFAULT;
	return 0;
    }

    tmp = _nd_newObj(callback,O_LIST,0,prompt,prefix,x,y,width,depth,help);

    if (tmp) {
	if (tmp->prefix || tmp->suffix) {
	    /* tweak the selectable area so the user can click on
	     * the scroll buttons
	     */
	    tmp->sely --;
	    tmp->seldepth += 2;
	}
	tmp->item.list.topy       = tmp->item.list.cury = 0;
	tmp->item.list.nritems    = nritems;
	tmp->item.list.items      = items;
	tmp->item.list.itemoffset = itemoffset;
	switch (flags & LO_SELECTION_MASK) {
	case HIGHLIGHT_SELECTED:tmp->item.list.kind = LO_HIGHLIT;
				break;

	case CHECK_SELECTED:	tmp->item.list.kind = LO_CHECK;
				break;

	case MENU_SELECTION:	tmp->item.list.kind = LO_MENU;
				break;

	default:		free(tmp);
				errno = EINVAL;
				return 0;
	}
	tmp->flags |= (flags & (SHOW_IDS|NO_HOTKEYS|CR_LIST|DEL_LIST|ALWAYS_HIGHLIT));
	if ((flags & DEL_LIST) == 0)
	    tmp->flags |= CR_LIST;
    }
    return tmp;
} /* newList */


/*
 * newRadioList() creates a listObj of radio buttons
 */
void *
newRadioList(int x, int y, int width, int depth, int nritems, ListItem *items,
	char *prompt, char *prefix, int flags, pfo callback, char *help)
{
    Obj *tmp = newList(x,y,width,depth,nritems,items,
		       prompt,prefix,flags,callback, help);

    if (tmp)
	tmp->flags |= RADIO_LIST;

    return tmp;
} /* newRadioList */


/*
 * newMenu() creates a menu listObj
 */
void*
newMenu(int x, int y, int width, int height, int nritems, ListItem *items,
	char *prompt, char *prefix, int flags, pfo callback, char *help)
{
    Obj *tmp;

    if ((flags & LO_SELECTION_MASK) == 0)
	flags |= MENU_SELECTION;

    tmp = newList(x, y, width, height, nritems, items, prompt,
                       prefix, flags, callback, help);

    if (tmp)
	tmp->flags |= MENU_LIST;

    return tmp;
} /* newMenu */


/*
 * newGauge() creates a new gauge object
 */
void *
newGauge(int x, int y, int width, int *percent, char *prompt, char *prefix)
{
    Obj *tmp = _nd_newObj(0,O_GAUGE,percent,prompt,prefix,x,y,width,1,0);

    if (tmp)
	RO_OBJ(tmp);

    return tmp;
} /* newGauge */


/*-----------------------------------------------*
 *                                               *
 *             D E S T R U C T O R S             *
 *                                               *
 *-----------------------------------------------*/

#if DYNAMIC_BINDING
/*
 * freeText() wipes out object-specific data for a TEXT
 * object
 */
static void
freeText(Obj *obj)
{
    if (obj->item.text.lines)
	free(obj->item.text.lines);
    if (obj->item.text.class == T_IS_HTML) {
	free(obj->item.text.bs);
	deletePage(obj->item.text.extra);
    }
} /* freeText */
#endif


/*
 * deleteObj() puts an object out of its misery
 */
void
deleteObj(void *o)
{
    Obj *obj = OBJ(o);

    if (!o)
	return;
    if (obj->title)
	free(obj->title);
    if (obj->prefix)
	free(obj->prefix);
    if (obj->suffix)
	free(obj->suffix);

#if DYNAMIC_BINDING
    if (nd_object_table[objType(obj)].free)
	(nd_object_table[objType(obj)].free)(obj);
#else
    switch (objType(obj)) {
    case O_TEXT:
		if (obj->item.text.lines)
		    free(obj->item.text.lines);
		if (obj->item.text.class == T_IS_HTML) {
		    free(obj->item.text.bs);
		    deletePage(obj->item.text.extra);
		}
		break;
    case W_LIST:
		deleteListWidget(obj);
		break;
#if HAS_FILESELECTOR
    case W_FILESEL:
		deleteFileSelector(obj);
		break;
#endif
    default:	/*nada*/
		break;
    }
#endif

    free(obj);
} /* deleteObj */


/*-----------------------------------------------*
 *                                               *
 *       D R A W I N G   F U N C T I O N S       *
 *                                               *
 *-----------------------------------------------*/

void
drawObj(void *obj, void *win)
{
    if (obj == 0)
	return;

    if (OBJ(obj)->flags & OBJ_HIDDEN)	/* don't draw hidden object */
	return;

    if (OBJ(obj)->parent)
	drawObj(OBJ(obj)->parent, win);
    else {
#if DYNAMIC_BINDING
	if (nd_object_table[OBJ(obj)->Class].draw)
	    (nd_object_table[OBJ(obj)->Class].draw)(obj,win);
#else
	switch (OBJ(obj)->Class) {
	case O_STRING:	drawString(obj,win);		break;
	case O_CHECK:	drawCheck(obj,win);		break;
	case O_BUTTON:	drawButton(obj,win);		break;
	case O_LIST:	drawList(obj,win);		break;
	case O_TEXT:	drawText(obj,win);		break;
	case O_GAUGE:	drawGauge(obj,win);		break;
	case W_LIST:	drawListWidget(obj,win);	break;
#if HAS_FILESELECTOR
	case W_FILESEL:	drawFileSelector(obj,win);	break;
#endif
	default:	/* to shut lint up */		break;
	}
#endif
    }
} /* drawObj */


/*-----------------------------------------------*
 *                                               *
 *           M A N I P U L A T O R S             *
 *                                               *
 *-----------------------------------------------*/


/*
 * editObj() is the function that actually lets users enter things into
 * the data fields of various objects.
 *
 * It expects:
 *	obj		-- the object we're playing with
 *	win		-- a display to write things into (_nd_display)
 *	event		-- an event pointer, containing the event (if any)
 *			   that we are called with, and containing the
 *			   event (if any) that we exit editObj() with.
 *	byevent		-- set if we are called with an event pending.
 *
 * It returns (codes are defined in ndialog.h):
 *	eERROR		-- something horrible went wrong.  Check errno for
 *			   for details.
 *	eCANCEL		-- user cancelled out of this field.
 *	eTAB		-- user [tab]bed out of this field.
 *	eBACKTAB	-- user [backtab]bed out of this field.
 *	eRETURN		-- user [return]ed out of this field.
 *	eEVENT		-- some event happened that punted us out of this
 *			   field.
 */
editCode
editObj(void *obj, void *win, void *event, editCode cc)
{
    editCode rc;
    if (obj == 0 || OBJ_READONLY(obj))
	return eTAB;

#if DYNAMIC_BINDING
    if (nd_object_table[objType(obj)].edit) {
	MAKE_CURRENT(obj);
	drawObj(obj,win);
	rc = (nd_object_table[objType(obj)].edit)(obj,win,event,cc);
	NOT_CURRENT(obj);
	touchObj(obj);
    }
    else {
	errno = EINVAL;
	rc = eERROR;
    }
#else
    switch (objType(obj)) {
    case O_STRING:	rc = editString(obj,win,event,cc);
			break;
    case O_BUTTON:	rc = editButton(obj,win,event,cc);
			break;
    case O_CHECK:	rc = editCheck(obj,win,event,cc);
			break;
    case O_LIST:	rc = editList(obj,win,event,cc);
			break;
    case O_TEXT:	rc = editText(obj,win,event,cc);
			break;
    case W_LIST:	rc = editListWidget(obj,win,event,cc);
			break;
#if HAS_FILESELECTOR
    case W_FILESEL:	rc = editFileSelector(obj,win,event,cc);
			break;
#endif
    default:		errno = EINVAL;
			rc = eERROR;
			break;
    }
    NOT_CURRENT(obj);
    touchObj(obj);
#endif
    return rc;
} /* editobj */


#if DYNAMIC_BINDING
/*-----------------------------------------------*
 *                                               *
 *     O B J E C T   R E G I S T R A T I O N     *
 *                                               *
 *-----------------------------------------------*/

struct _nd_object_table nd_object_table[ND_TABLE_SIZE] = {
    { 1,  },
    { 1, editString, drawString, 0,         nd_bindToString, nd_typeSize,
					    getStringCursor, setStringCursor },
    { 1, editCheck,  drawCheck,  0,         nd_bindToType,   nd_typeSize,  },
    { 1, editButton, drawButton, 0,         nd_bindToType,   nd_buttonSize },
    { 1, editList,   drawList,   0,         nd_bindToList,   nd_typeSize,
					    getListCursor,   setListCursor },
    { 1, editText,   drawText,   freeText,  nd_bindToText,   nd_typeSize,
					    0,               setTextCursor },
    { 1, 0,          drawGauge,  0,         nd_bindToType,   nd_typeSize   },
} ;


/*
 * nd_register_object() adds a new object type into the ndialog
 *                      framework.  If it can register this type,
 *                      it returns the new object class, otherwise
 *                      it returns -1 and sets errno to ENFILE to
 *                      indicate an array overflow.
 */
int
nd_register_object(nd_edit edit, nd_draw draw,
                   nd_free free, nd_bind bind,
		   nd_size size)
{
    struct _nd_object_table t;

    memset(&t, 0, sizeof t);
    t.edit = edit;
    t.draw = draw;
    t.free = free;
    t.bind = bind;
    t.size = size;

    return nd_register_objtab(sizeof t, &t);
}


/*
 * nd_register_objtab() adds a new object type into the ndialog
 *                      framework.  If it can register this type,
 *                      it returns the new object class, otherwise
 *                      it returns -1 and sets errno to ENFILE to
 *                      indicate an array overflow.
 *
 *			objtab takes a nd_register_table[] record
 *			and a size
 */
int
nd_register_objtab(int recsize, struct _nd_object_table *rec)
{
    int x;

    /* be paranoid about the record, just to avoid unpleasant surprises
     * we want to fail if
     *   1) the record size is too short,
     *   2) the record size is too long,
     *   3) the record size is misaligned
     */
    if ( (recsize <= sizeof rec->edit) || (recsize > sizeof *rec)
      || ((recsize - sizeof rec->used) % sizeof rec->edit) != 0) {
	errno = EINVAL;
	return -1;
    }

    /* walk the table looking for an open slot, hard as that may be to
     * believe
     */
    for (x=0; x < ND_TABLE_SIZE; x++)
	if (!nd_object_table[x].used) {
	    memset(&nd_object_table[x], 0, sizeof nd_object_table[x]);
	    memcpy(&nd_object_table[x], rec, recsize);
	    nd_object_table[x].used = 1;
	    if (rec->size == 0)
		nd_object_table[x].size = nd_typeSize;
	    return x;
	}
    errno = ENFILE;	/* `File table full'; descriptive, if not completely
			 * accurate
			 */
    return -1;	/* sorry! */
}
#endif


/*
 * setUserData() set a pointer to some user data area
 */
void
setUserData(void *o, void* data)
{
    if (o)
	OBJ(o)->user_data = data;
}


/*
 * getUserData() returns a previously set pointer (see above)
 */
void*
getUserData(void *o)
{
    return o ? OBJ(o)->user_data : 0;
}
