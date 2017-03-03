/*
 * list_widget.c: editable list widget (W_LIST)
 * Copyright (C) 1996-2017 David L Parsons.
 * The redistribution terms are provided in the COPYRIGHT file that must
 * be distributed with this source code.
 */

/*
 *        |<--- y --->|
 *         (1,1)
 *          |
 * - (0,0)+-v---------+
 * ^     <| ADD       |> _____ (y,3)
 * | (0,2)+---------^-+ /
 * |      | LIST      | [DELETE]
 * x      | \________ |___ (1,3)
 * |      |           |
 * v      |           |
 * _ (0,x)+---------v-+
 */
#include <config.h>

#include <nd_objects.h>
#include <dialog.h>
#include "ndwin.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>


static int listwidget_insert(Obj*, void*);
static int listwidget_delete(Obj*, void*);

#if DYNAMIC_BINDING

#define DYN_STATIC	static

DYN_STATIC void drawListWidget(void *o, void *w);
DYN_STATIC void deleteListWidget(ndObject o);
DYN_STATIC editCode editListWidget(void* o, void* w, MEVENT *mev, editCode cc);

static int listType = O_ERROR;
#define W_LIST	listType

#else

#define DYN_STATIC

#endif

/*
 * newListWidget() creates a new (ta dah!) list widget
 * width and depth are the dimensions of the list box; the other dimensions
 * are computed automagically.
 */
ndObject
newListWidget(int x, int y, int width, int depth, LIA list,
	      char* prompt,
	      pfo insert_callback, pfo delete_callback, pfo dummy,
	      char* help)
{
    return newEditableList(x,y,width,depth,list,prompt,
			   insert_callback, delete_callback, 0, help);
} /* newListWidget */


/*
 * newEditableList() creates a new (optionally editable) list widget.
 * This differs from the previously published newListWidget function
 * in that it has an optional update_callback button that can be used
 * to activate a [MODIFY] button.
 */
ndObject
newEditableList(int x, int y, int width, int depth, LIA list,
	      char* prompt,
	      pfo insert_callback, pfo delete_callback, pfo update_callback,
	      char* help)
{
    Obj* tmp;
    char* bfr;

#define WIDGET_WIDTH	(8+1+2)		/* size of [DELETE], plus a space, plus
					 * the list frame
					 */
#define WIDGET_DEPTH	(4)
					/* size of the add string, plus 3 for
					 * the list frame
					 */

    if (width < 0 || depth < 0)  {
	errno = EINVAL;
	return 0;
    }

    if (width+WIDGET_WIDTH > COLS-2 || depth+WIDGET_DEPTH > LINES-4) {
	/* object too big to fit on the screen */
	errno = EOVERFLOW;
	return 0;
    }
    if (prompt)
	depth++;

#if DYNAMIC_BINDING
    if (W_LIST == O_ERROR) {
	static struct _nd_object_table t = { 0,  (nd_edit)editListWidget,
						 (nd_draw)drawListWidget,
						 (nd_free)deleteListWidget,
						 0, 0, 0, 0 };
	W_LIST = nd_register_objtab(sizeof t, &t);
    }
    if (W_LIST == -1) {
	errno = ENFILE;
	return 0;
    }
#endif
    tmp = _nd_newObj(0, W_LIST, 0, prompt, 0,
				x, y, width+WIDGET_WIDTH, depth+WIDGET_DEPTH,
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
    /*
     first build the list widget, then build a string widget that's as wide
     as the list widget, then add the ADD and DELETE buttons
     */
    tmp->item.listwidget.list = newMenu(x,y+2,width,depth,
					 LIAcount(list), LIAlist(list),
					 0, "",
					 MENU_SELECTION|DEL_LIST|ALWAYS_HIGHLIT,
					 (pfo)listwidget_delete, help);
    if (tmp->item.listwidget.list == 0) {
	deleteObj(tmp);
	return 0;
    }
    OBJ(tmp->item.listwidget.list)->parent = tmp;

    /* We've built the list object;  now build a string object that sits
     * right above it.
     */
    if ((bfr = malloc(width+1)) == 0) {
	deleteObj(tmp);
	return 0;
    }
    memset(bfr, 0, width+1);
    tmp->item.listwidget.add = newString(x,y,width,width,bfr,0,"",
					  (pfo)listwidget_insert, help);

    if (tmp->item.listwidget.add == 0) {
	free(bfr);
	deleteObj(tmp);
	return 0;
    }
    OBJ(tmp->item.listwidget.add)->parent = tmp;


    /* an ADD button that sits by the string */
    tmp->item.listwidget.addbutton = newButton(0,"ADD",
						(pfo)listwidget_insert,help);
    if (tmp->item.listwidget.addbutton == 0) {
	deleteObj(tmp);
	return 0;
    }
    else {
	/* we need to manually position the button
	 */
	OBJ(tmp->item.listwidget.addbutton)->parent = tmp;
	setButtonDataArea(tmp->item.listwidget.addbutton,
			    x + width + 3,  y + 1);
    }

    /* if there's an update callback, the user probably wants an
     * UPDATE button to sit besides the list.
     */
    if (update_callback) {
	tmp->item.listwidget.update = newButton(0, "UPDATE", update_callback, help);
	if (tmp->item.listwidget.update == 0) {
	    deleteObj(tmp);
	    return 0;
	}
	else {
	    /* we need to manually position the button
	     */
	    OBJ(tmp->item.listwidget.update)->parent = tmp;
	    setButtonDataArea(tmp->item.listwidget.update,
				x + width + 3,  y + 3);
	}
    }

    /* and finally a DELETE button that sits beside the list */
    tmp->item.listwidget.delete = newButton(0,"DELETE",
						(pfo)listwidget_delete,help);

    if (tmp->item.listwidget.delete == 0) {
	deleteObj(tmp);
	return 0;
    }
    else {
	/* we need to manually position the button
	 */
	OBJ(tmp->item.listwidget.delete)->parent = tmp;
	setButtonDataArea(tmp->item.listwidget.delete,
			    x + width + 3,  y + (update_callback ? 5 : 3));
    }

    /* All done, finally */
    tmp->item.listwidget.thelist = list;
    tmp->item.listwidget.active  = lw_A;
    tmp->item.listwidget.insert_check = insert_callback;
    tmp->item.listwidget.delete_check = delete_callback;
    tmp->item.listwidget.update_check = update_callback;
    return tmp;
} /* newListWidget */


/*
 * drawListWidget() draws a list widget
 */
DYN_STATIC void
drawListWidget(void *o, void *w)
{
    Obj* obj = OBJ(o);
    int color;
    int width = 0;
    int top, cur;

    if (o == 0 || objType(o) != W_LIST)
	return;

    if (IS_CURRENT(obj))
	color = ACTIVE_COLOR;
    else
#if WITH_NCURSES
	color = WINDOW_COLOR | (OBJ_READONLY(obj) ? A_DIM : 0);
#else
	color = WINDOW_COLOR;
#endif

    if (obj->title)
	mvwaddstr(Window(w), obj->y + WY(w), obj->x + WX(w), obj->title);

    if (obj->item.listwidget.list && obj->item.listwidget.add
				  && obj->item.listwidget.delete) {
	width = OBJ(obj->item.listwidget.list)->width;
	drawbox(Window(w), obj->y + WY(w) + strdepth(obj->title),
		           obj->x + WX(w), obj->depth-strdepth(obj->title),
		           width+2, 2, color, color);

	top = OBJ(obj->item.listwidget.list)->item.list.topy;
	cur = OBJ(obj->item.listwidget.list)->item.list.cury;

	setObjData(obj->item.listwidget.list, 0,
		   LIAlist(obj->item.listwidget.thelist),
		   LIAcount(obj->item.listwidget.thelist));

	if (LIAcount(obj->item.listwidget.thelist) >= cur) {
	    OBJ(obj->item.listwidget.list)->item.list.topy = top;
	    OBJ(obj->item.listwidget.list)->item.list.cury = cur;
	}

	drawString(obj->item.listwidget.add,       w);
	drawButton(obj->item.listwidget.addbutton, w);
	drawList  (obj->item.listwidget.list,      w);
	drawButton(obj->item.listwidget.delete,    w);
	if (obj->item.listwidget.update)
	    drawButton(obj->item.listwidget.update,w);
    }
} /* drawListWidget */


/*
 * deleteListWidget() does what you'd expect it to.
 */
DYN_STATIC void
deleteListWidget(ndObject o)
{
    Obj* obj = OBJ(o);

    if (o == 0 || objType(o) != W_LIST)
	return;

    if (obj->item.listwidget.delete)
	deleteObj(obj->item.listwidget.delete);
    if (obj->item.listwidget.update)
	deleteObj(obj->item.listwidget.update);
    if (obj->item.listwidget.list)
	deleteObj(obj->item.listwidget.list);
    if (obj->item.listwidget.addbutton)
	deleteObj(obj->item.listwidget.addbutton);
    if (obj->item.listwidget.add) {
	if (OBJ(obj->item.listwidget.add)->content)
	    free(OBJ(obj->item.listwidget.add)->content);
	deleteObj(obj->item.listwidget.add);
    }
} /* deleteListWidget */


/*
 * list widget callbacks.
 * listwidget_insert: is called when someone has entered a string and
 *                    pressed [return] or clicked [ADD] to accept it
 * listwidget_delete: is called when someone presses [DEL] in the list
 *                    box or clicks on [DELETE].
 *
 * We don't bother to do any error or type checking in these functions,
 * because they should only be called by the list widget.  If we ever
 * publish them, then we'll have to go back and make them safe for public
 * consumption.
 *
 * All of these functions end up calling the user-defined functions
 * insert_callback or delete_callback, which simply do validation checking
 * on the item being inserted or deleted.
 */

/*
 * listwidget_insert() is called when someone enters a string
 */
static int
listwidget_insert(Obj* obj, void* w)
{
    Obj* parent = obj->parent;
    char *bfr = objDataPtr(obj);
    LIA list = parent->item.listwidget.thelist;
    int rc;
    /*int sel;*/

    if (objType(obj) == O_BUTTON) { /* we are being called from the button */
	rc = listwidget_insert(parent->item.listwidget.add, w);
	return (rc < 0) ? rc : 0;
    }

    /*
     * make sure the user is actually entering something
     */
    if (strlen(bfr) == 0)
	return 0;

    /*
     * validate the user input
     */
    if (parent->item.listwidget.insert_check) {
	rc = (*(parent->item.listwidget.insert_check))(obj, w);
	if (rc == 0)
	    return 0;
    }
    else rc = 1;

    /* add the new string to the list
     */
    addToLIA(list, 0, bfr, 0);
    setObjData(parent->item.listwidget.list, 0, LIAlist(list), LIAcount(list));
    setObjCursor(parent->item.listwidget.list, LIAcount(list)-1);

    /* then clear the string
     */
    memset(bfr, 0, obj->width);
    setObjCursor(obj, 0);

    drawObj(obj->parent, w);

    /* and we're done */
    return rc;
} /* listwidget_insert */


/*
 * listwidget_delete() is called when someone presses [DEL] to remove
 * an item from the list, or when someone presses the [DELETE] button.
 */
static int
listwidget_delete(Obj *obj, void* w)
{
    int rc, sel;
    LIA list;

    if (objType(obj) == O_BUTTON) /* we are being called from the button */
	return listwidget_delete(obj->parent->item.listwidget.list, w);

    /* can't delete if there's nothing here! */
    if (getObjListSize(obj) < 1)
	return 0;

    /* do user validation */
    if (obj->parent->item.listwidget.delete_check) {
	rc = (*(obj->parent->item.listwidget.delete_check))(obj, w);
	if (rc == 0)
	    return 0;
    }
    else rc = 0;

    /* delete the current selection from the list
     */
    sel = getObjCursor(obj);
    list = obj->parent->item.listwidget.thelist;
    delFromLIA(list, sel);
#if 0
    setObjData(obj, 0, LIAlist(list), LIAcount(list));
#endif
    if (sel < LIAcount(list))
	setObjCursor(obj, sel);
    else if (sel > 0)
	setObjCursor(obj, sel-1);

    drawObj(obj->parent, w);

    return 0;
} /* listwidget_delete */


/*
 * editListWidget() lets the user manipulate the contents of a list widget
 * the edit loop here is a smaller version of the MENU() edit loop, which
 * should be abstracted out of MENU() so that everyone can join in the fun.
 */
DYN_STATIC editCode
editListWidget(void* o, void* w, MEVENT *mev, editCode cc)
{
    Obj *obj = OBJ(o);
#define NROBJS	5/* add, add-button, list, update, delete */
    Obj *lwlist[NROBJS];
    int idx;

    if (o == 0 || objType(o) != W_LIST) {
	errno = EINVAL;
	return eERROR;
    }

    /* build our little navigation array */
    lwlist[lw_A] = obj->item.listwidget.add;
    lwlist[lw_AB]= obj->item.listwidget.addbutton;
    lwlist[lw_L] = obj->item.listwidget.list;
    lwlist[lw_U] = obj->item.listwidget.update;
    lwlist[lw_D] = obj->item.listwidget.delete;

    /* set the initial position in the widget */
    switch (cc) {
#ifdef VERMIN
    case eEVENT:	/* mouse click */
	    for (idx=0; idx<NROBJS; idx++)
		if (lwlist[idx] && _nd_inside(lwlist[idx], mev))
		    break;
	    if (idx == NROBJS)
		idx = obj->item.listwidget.active;
	    break;
#endif
    case eBACKTAB:	/* backtab */
	    idx = LIAcount(obj->item.listwidget.thelist) ? lw_D : lw_AB;
	    break;
    case eTAB:		/* tab */
	    idx = 0;
	    break;
    default:
	    idx = obj->item.listwidget.active;
	    break;
    }


    /* chomp happily through the listwidget
     */

    while (1) {
	int incr;

	cc = editObj(lwlist[idx], w, mev, cc);

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
	default:
		incr = 0;
		break;
	}
	drawObj(o, w);

	if (incr > 0) {
	forward:
	    if (idx == NROBJS-1)
		break;
	    else {
		idx++;
		if (!lwlist[idx])
		    goto forward;
		if (idx==lw_AB && !*(char*)objDataPtr(obj->item.listwidget.add))
		    goto forward;
		if (idx==lw_D && LIAcount(obj->item.listwidget.thelist)==0)
		    goto forward;
	    }
	}
	else if (incr < 0) {
	backward:
	    if (idx == 0)
		break;
	    else {
		--idx;
		if (!lwlist[idx])
		    goto backward;
		if (idx==lw_L && LIAcount(obj->item.listwidget.thelist)==0)
		    goto backward;
		if (idx==lw_AB && !*(char*)objDataPtr(obj->item.listwidget.add))
		    goto backward;
	    }
	}
    }
byebye:
    obj->item.listwidget.active = idx;
    return cc;
} /* editListWidget */



/*
 * whichListWidgetObj() returns the object that's active inside the list widget.
 */
ndObject
whichListWidgetObj(ndObject o)
{
    Obj* obj = OBJ(o);

    if (o == 0 || objType(o) != W_LIST) {
	errno = EINVAL;
	return 0;
    }
    switch (obj->item.listwidget.active) {
    case lw_A:	return obj->item.listwidget.add;
    case lw_AB: return obj->item.listwidget.addbutton;
    case lw_L:  return obj->item.listwidget.list;
    case lw_D:	return obj->item.listwidget.update;
    default:	return obj->item.listwidget.delete;	/* delete is the default */
    }
} /* whichListWidgetObj */
