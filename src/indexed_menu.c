/*
 *   Copyright (c) 2000 David Parsons. All rights reserved.
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
 * indexed_menu.c: two-level menu widget
 *
 *        |<--------------- y ---------------> |
 * - (0,0)+--------------+ +-------------------+
 * ^      |              | |                   |
 * |      |              | |                   |
 * x      |              | |                   |
 * |      |              | |                   |
 * v      |              | |                   |
 * - (0,x)+--------------+ +-------------------+
 */

#include <config.h>

#include "nd_objects.h"
#include "curse.h"
#include "dialog.h"
#include "ndwin.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>


#if DYNAMIC_BINDING

#define DYN_STATIC	static

DYN_STATIC void drawIndexedMenu(void *o, void *w);
DYN_STATIC void deleteIndexedMenu(ndObject o);
DYN_STATIC editCode editIndexedMenu(void* o, void* w, MEVENT *mev, editCode cc);

static int listType = O_ERROR;
#define W_IMENU	listType

#else

#Error "This widget won't work unless DYNAMIC_BINDING is set"

#endif

/* our generic object class */
typedef struct {
    Obj *index;		/* the index menu */
    Obj *menu;		/* the actual menu */
    int ikey;		/* currently selected index */
    int mkey;		/* and currently selected menu */
    int which_list;	/* 0=index, 1=menu */
    pfo callback;	/* user-supplied callback */
} Imenu;

static int indexselector(ndObject obj, ndDisplay win);
static int menuselector (ndObject obj, ndDisplay win);


/*
 * newIndexedMenu() creates a new (ta dah!) indexed menu.
 * width and depth are the dimensions of the list box; the other dimensions
 * are computed automagically.
 */
ndObject
newIndexedMenu(int x, int y, int width, int depth, int indexwidth, LIA list,
	      char* prompt, pfo select_callback, char *help)
{
    Obj* tmp;
    Imenu* local;
    int idx, l;

    if (depth < 0 || width < 0) {
	errno = EINVAL;
	return 0;
    }

    if (width+x > COLS-2 || depth+y > LINES-4) {
	errno = EOVERFLOW;
	return 0;
    }

    width -= 4; /* compensate for frames */

    /* automatically size the index width */
    if (indexwidth < 0) {
	for (idx = 0; idx < LIAcount(list); idx++)
	    if ((l = strlen(LIAlist(list)[idx].item)) > indexwidth)
		indexwidth = l;
	indexwidth+=2;
    }
    if (indexwidth >= width) {
	errno = EOVERFLOW;
	return 0;
    }

    if (prompt)
	depth++;

#if DYNAMIC_BINDING
    if (W_IMENU == O_ERROR) {
	static struct _nd_object_table t = { 0, (nd_edit)editIndexedMenu,
						(nd_draw)drawIndexedMenu,
						(nd_free)deleteIndexedMenu,
						0, 0, 0, 0} ;
	W_IMENU = nd_register_objtab(sizeof t, &t);
    }
    if (W_IMENU == -1) {
	errno = ENFILE;
	return 0;
    }
#endif

    tmp = _nd_newObj(0, W_IMENU, 0, prompt, 0,
				x, y, width, depth,
				help);

    if (tmp == 0)
	return 0;

    if (prompt) {
	/* adjust the dimensions of the widget so we can fit the prompt
	 * into it.
	 */
	y++;
	depth--;
    }

    if ((tmp->item.generic = malloc(sizeof *local)) == 0) {
	deleteObj(tmp);
	return 0;
    }
    local = (Imenu*)tmp->item.generic;

    local->ikey = local->mkey = 0;
    local->callback = select_callback;
    local->which_list = 0;

    local->index = newMenu(x,y, indexwidth-2, depth-2,
			    LIAcount(list), LIAlist(list),
			    0, "", 0,
			    indexselector, help);
    local->menu =  newMenu(x+indexwidth+2, y, width-(indexwidth+4), depth-2,
			    0, 0,
			    0, "", 0,
			    menuselector, help);
    if (local->index == 0 || local->menu == 0) {
	deleteObj(tmp);
	return 0;
    }
    local->index->parent = tmp;
    local->index->flags |= OBJ_DRAW;
    local->menu->parent = tmp;
    local->menu->flags |= OBJ_DRAW;
    return tmp;
} /* newIndexedMenu */


/*
 * drawIndexedMenu() draws an indexed menu
 */
DYN_STATIC void
drawIndexedMenu(void *o, void *w)
{
    Obj* obj = OBJ(o);
    int key;
    Imenu* local;
    ListItem *menu;

    if (o == 0 || objType(o) != W_IMENU)
	return;

    local = (Imenu*)(obj->item.generic);

    menu = getObjList(local->index);

    key = currentSelection(local->index);
    if (LIAlist(menu[key].id) != getObjList(local->menu)) {
	setObjData(local->menu, 0, LIAlist(menu[key].id),
				   LIAcount(menu[key].id));
	if (key == local->ikey)
	    setObjCursor(local->menu,  local->mkey);
    }

    if (obj->title)
	mvwaddstr(Window(w), obj->y + WY(w), obj->x + WX(w), obj->title);

    if (local->which_list == 0) {
	drawList( local->menu,  w);
	drawList( local->index, w);
    }
    else {
	drawList( local->index, w);
	drawList( local->menu,  w);
    }
} /* drawIndexedMenu */


/*
 * deleteIndexedMenu() does what you'd expect it to.
 */
DYN_STATIC void
deleteIndexedMenu(ndObject o)
{
    Obj* obj = OBJ(o);
    Imenu *local;

    if (o == 0 || objType(o) != W_IMENU)
	return;

    local = (Imenu*)(obj->item.generic);

    if (local) {
	if (local->index)
	    deleteObj(local->index);
	if (local->menu)
	    deleteObj(local->menu);
	free(local);
    }
} /* deleteIndexedMenu */


/*
 * editIndexedMenu() lets the user manipulate the contents of an indexed menu.
 * the edit loop here is a smaller version of the MENU() edit loop, which
 * should be abstracted out of MENU() so that everyone can join in the fun.
 */
DYN_STATIC editCode
editIndexedMenu(void* o, void* w, MEVENT *mev, editCode cc)
{
    Obj *obj = OBJ(o);
    Imenu *local;
#define NROBJS	2/* index, menu */
    Obj* lists[NROBJS];
    int idx;

    if (o == 0 || objType(o) != W_IMENU) {
	errno = EINVAL;
	return eERROR;
    }
    local = (Imenu*) (obj->item.generic);

    /* build our little navigation array */
    lists[0] = local->index;
    lists[1] = local->menu;

    /* set the initial position in the widget */
    switch (cc) {
#ifdef VERMIN
    case eEVENT:	/* mouse click */
	    for (idx=0; idx<NROBJS; idx++)
		if (lists[idx] && _nd_inside(lists[idx], mev))
		    break;
	    if (idx == NROBJS)
		idx = local->which_list;
	    break;
#endif
    case eBACKTAB:	/* backtab */
	    idx = LIAcount(local->menu) ? 1 : 0;
	    break;
    case eTAB:		/* tab */
	    idx = 0;
	    break;
    default:
	    idx = local->which_list;
	    break;
    }


    /* chomp happily through the indexed menu
     */

    while (1) {
	int incr = 0;

	cc = editObj(lists[idx], w, mev, cc);

	switch (cc) {
	case eTAB:
		incr = 1;
		break;
	case eBACKTAB:
		incr = -1;
		break;

	case eEVENT:
	case eREFRESH:
	case eEXITFORM:
	case eESCAPE:
		drawObj(o, w);
		goto byebye;

	case eERROR:
		Error("listwidget");
		incr = 1;
		break;
	case eRETURN:
		if (idx == 0) {
		    incr++;
		    break;
		}
	default:
		incr = 0;
		break;
	}
	drawObj(o, w);

	if (incr > 0) {
	    if (idx == 1 || getObjListSize(local->menu) == 0)
		break;
	    idx++;
	}
	else if (incr < 0) {
	    if (idx == 0)
		break;
	    --idx;
	}
    }
byebye:
    local->which_list = idx;
    return cc;
} /* editIndexedMenu */


/*
 * indexselector() populates the menu list according to the current selection
 * in the index list.
 */
static int
indexselector(ndObject obj, ndDisplay display)
{
    ListItem *menu;
    int ix;
    Imenu* local;

    menu = getObjList(obj);
    ix = currentSelection(obj);

    local = (Imenu*)(OBJ(obj)->parent->item.generic);

    if (menu[ix].id == 0) {
	/* selected something that doesn't have a submenu */
	local->ikey = ix;
	local->mkey = 0;
	return local->callback(OBJ(obj)->parent, display);
    }

    setObjData(local->menu, 0, LIAlist(menu[ix].id), LIAcount(menu[ix].id));

    if (ix == local->ikey)
	setObjCursor(local->menu, local->mkey);

    drawObj(local->menu, display);
    return 1;
} /* indexselector */


/*
 * menuselector() handles a menu selection.
 */
static int
menuselector(ndObject obj, ndDisplay display)
{
    Imenu *local;

    local = (Imenu*)(OBJ(obj)->parent->item.generic);

    local->ikey = currentSelection(local->index);
    local->mkey = currentSelection(local->menu);

    return local->callback(OBJ(obj)->parent, display);
} /* menuselector */


/*
 * getIndexedMenuSelection() returns the ikey and mkey to the caller.
 */
int
getIndexedMenuSelection(void* o, int *ikey, int *mkey)
{
    Obj* obj = OBJ(o);
    Imenu *local;

    if (o == 0 || objType(o) != W_IMENU) {
	errno = EINVAL;
	return EOF;
    }
    local = (Imenu*)(obj->item.generic);
    *ikey = local->ikey;
    *mkey = local->mkey;
    return 0;
} /* getIndexedMenuSelection */


/*
 * setIndexedMenuSelection() sets the ikey and mkey
 */
int
setIndexedMenuSelection(void* o, int ikey, int mkey)
{
    Obj* obj = OBJ(o);
    Imenu* local;
    ListItem *midx;

    if (o == 0 || objType(o) != W_IMENU) {
	errno = EINVAL;
	return EOF;
    }
    local = (Imenu*)(obj->item.generic);
    setObjCursor(local->index, local->ikey = ikey);
    midx = getObjList(local->index);

    if (midx[ikey].id) {
	setObjData(local->menu, 0, LIAlist(midx[ikey].id),
				   LIAcount(midx[ikey].id));
	setObjCursor(local->menu,  local->mkey = mkey);
    }
    else
	setObjData(local->menu, 0, 0, 0);

    return 0;
} /* setIndexedMenuSelection */
