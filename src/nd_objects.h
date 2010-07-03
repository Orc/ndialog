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
/*
 * new dialog library: internal headers
 */
#ifndef ND_OBJECTS_D
#define ND_OBJECTS_D

#include <stdarg.h>			/* for va_list */

typedef int (*pfo)(void*, void*);

#ifdef NDIALOG_H
# error "don't include both ndialog.h and nd_objects.h"
#endif

/* include object classes */
#include "ndialog.h"

/*
 * a note on flags:  objects have a global flags variable that are used
 * for three things -- attributes specific to a subclass, attributes for
 * the generic object, and meta-attributes that describe how objects relate
 * inside widgets.
 *
 * each type of flag has a given bit range in a 32-bit number:
 * meta-attributes are bits 24-31	-- 0??000000h		_ND_META(x)
 * generic attributes are bits 16-23	-- 000??0000h		_ND_GENERIC(x)
 * subclass attributes are bits 0-15	-- 00000????h		_ND_SUB(x)
 */
#define _ND_META(x)	((x) & 0xFF000000L)
#define _ND_GENERIC(x)	((x) & 0x00FF0000L)
#define _ND_SUBCLASS(x)	((x) & 0x0000FFFFL)

/*
 * meta-attributes
 */
#define IN_A_WIDGET	0x01000000	/* widget type mask */
#define PART_OF_A_WIDGET(x)	((x).flags & IN_A_WIDGET)
#define WIDGET_TYPE(x)		(PART_OF_A_WIDGET(x) \
				    ? (x).parent->class \
				    : O_ERROR)

/*
 * A string object is simply a string field that you can type into
 */
typedef struct {
    int startx;		/* where the window into the data string begins */
    int curx;		/* where the cursor was when we were here before */
    int maxlen;		/* length of field, length of variable */
/* flag bits */
#define PASSWORD_STRING	0x01		/* is this a password string? */
#define INSERT_MODE	0x02		/* are we inserting or overwriting */
					/* characters in the string? */
} S_Obj;


/*
 * a button object is a pushbutton, with possibly some special abilities
 */
typedef struct {
    enum {CANCEL_BUTTON, CONFIRM_BUTTON, REGULAR_BUTTON} kind;
} B_Obj;


/*
 * a list object is a scrollable menu of check objects, which you can
 * check or uncheck items with [space].
 *
 * there are two variants to the basic list object.  The first, the
 * Radio list, is a regular list that only lets you press one button
 * at a time.  The second, the MENU, is a list that only lets you
 * press one button before leaving.
 */
enum LO_format {LO_CHECK, LO_HIGHLIT, LO_MENU};

typedef struct {
    int topy;		/* first item visible on the list */
    int cury;		/* item last touched before leaving the object */
    int nritems;	/* number of items in the list */
    ListItem *items;	/* the items in the list */
    int itemoffset;	/* when displaying both id and item, we need to
			 * properly offset the item in the field
			 */
/* generic flag bits */
#define RADIO_LIST	0x0100		/* this is a radio list */
#define MENU_LIST	0x0200		/* this is a menu list */
    enum LO_format kind;
			/* how to show checked items */
} L_Obj;


/*
 * a text object is a display-only box displaying an arbitrarily
 * long string.
 */
typedef struct {
    int class;		/* is this regular text, or something special? */
#define T_IS_TEXT	0		/* regular text */
#define T_IS_HTML	1		/* html */
    char **lines;	/* pointers to each line in the string */
    int nrlines;	/* number of lines in the string */
    int topy;		/* top of window */
    int off_x;		/* X offset, if scrolled left or right */
    int width;		/* width of widest line */
    short *bs;		/* T_IS_HTML: text backing store */
    short href;		/* T_IS_HTML: current href# */
    void *extra;	/* subclass-defined content */
} T_Obj;


/*
 * an editable list widget is an object containing a
 * list object, a string object, and a pair of button objects.
 */
typedef struct {
    LIA thelist;		/* the list we're manipulating */
    pfo insert_check;		/* user-defined insert check */
    pfo delete_check;		/* user-defined delete check */
    pfo update_check;		/* user-defined update check */
    void* add;			/* a field to add things to the list */
    void* addbutton;		/* ADD button */
    void* list;			/* a list that we can delete from */
    void* delete;		/* DELETE button */
    void* update;		/* UPDATE button, if it exists */
    /* if update != 0, we have an update button */
    enum { lw_A=0, lw_AB=1, lw_L=2, lw_U=3, lw_D=4 }
		    active;	/* which item is active */
} List_Widget;

/*
 * a file selector is an object containing a string and a pair of
 * list objects.
 */
typedef struct {
    LIA dirs;			/* directories in current directory */
    LIA files;			/* files in current directory */
    pfo chdir_check;		/* called before we chdir */
    pfo file_check;		/* called before we accept a file */
    pfsel file_match;		/* called to scan new files and dirs */
    char* curdir;		/* current directory */
    void* selection;		/* the selection string object */
    void* dirlist;		/* list containing directories */
    void* filelist;		/* list containing files */
    enum {fs_s=0, fs_d=1, fs_f=2 }
		active;		/* which item is active */
} File_Selector;


/*
 * the Obj structure is the base class for all the various objects
 * that you can find in a cfgmenu.  It contains common information
 * for each object, including the
 *	callback		-- callback function
 *	content			-- where data is written
 *	title			-\         title
 *	prefix			-- prefix [field] suffix
 *	suffix			-/
 *	x, y			-- the origin for this field, relative to
 *				-- the window.
 *	width, depth		-- the size of this field, excluding borders.
 *	next, prev		-- objects are kept in a doubly linked list
 *	flags			-- interesting things about this field.
 *	Class			-- subclass identifier
 *	item			-- subclass specific information
 */
typedef struct _nd_obj {
    pfo callback;		/* callback function */
    void *content;		/* pointer to the value of the object */
    char *help;			/* help information (format?) */
    char *title;		/* title of this field, if any */
    char *prefix, *suffix;	/* prefix and suffix for the field */
    int x, y;			/* origin */
    int dtx, dty,		/* data area origin ... */
	width, depth;		/* ... and size */
    int selx, sely,		/* mouse selection area origin ... */
	selwidth, seldepth;	/* ... and size */
    struct _nd_obj *next;	/* next object in list */
    struct _nd_obj *prev;	/* last object in list */
    long flags;			/* various attribute flags (need 32 bits) */
#define OBJ_WRITABLE	0x00010000	/* can we modify this object? */
#define OBJ_WRITTEN	0x00020000	/* has this object been modified? */
#define OBJ_HIDDEN	0x00040000	/* hide this object from sight */
#define OBJ_CURRENT	0x00080000	/* is this object being edited? */
#define OBJ_CLICKED	0x00100000	/* special for a clickable object */
#define OBJ_DIRTY	0x00200000	/* object has been modified */
#define OBJ_DRAW	0x01000000	/* no special drawing for widget */
    enum Class Class;		/* what type of object it is */
    struct _nd_obj *parent;	/* who we're related to, if we're part of an
				 * aggregate object
				 */
    union {			/* specifics for the object */
	S_Obj string;
	/* Check objects have no special attributes */
	B_Obj button;
	L_Obj list;
	T_Obj text;
	List_Widget listwidget;
	File_Selector fileselect;
	void* generic;
    } item;
    void *user_data;		/* pointer to user-specified data */
} Obj;

/* cast a void pointer into an Obj*, without fuss muss or ugly syntax
 */
#define OBJ(x)	((Obj*)x)

/* interesting things to do with objects
 */
#define ISCANCEL(x)	(OBJ(x)->Class == O_BUTTON && \
			    OBJ(x)->item.button.kind == CANCEL_BUTTON)
#define ISCONFIRM(x)	(OBJ(x)->Class == O_BUTTON && \
			    OBJ(x)->item.button.kind == CONFIRM_BUTTON)

extern int _nd_setflag(Obj *o, int bit) ;
extern int _nd_clearflag(Obj *o, int bit) ;

#define OBJ_READONLY(x)	(!(OBJ(x)->flags & OBJ_WRITABLE))
#define	RO_OBJ(x)	_nd_clearflag(x, OBJ_WRITABLE)
#define RW_OBJ(x)	_nd_setflag(x, OBJ_WRITABLE)
#define OBJ_TOUCHED(x)	(  OBJ(x)->flags & OBJ_WRITTEN)
#define TOUCH_OBJ(x)	_nd_setflag(x, OBJ_WRITTEN)
#define UNTOUCH_OBJ(x)	_nd_clearflag(x, OBJ_WRITTEN)
#define HIDE_OBJ(x)	(_nd_setflag(x, OBJ_HIDDEN) )
#define EXPOSE_OBJ(x)	(_nd_clearflag(x, OBJ_HIDDEN))
#define HIDDEN_OBJ(x)	(  OBJ(x)->flags & OBJ_HIDDEN )
#define MAKE_CURRENT(x)	_nd_setflag(x, OBJ_CURRENT)
#define IS_CURRENT(x)	(  OBJ(x)->flags & OBJ_CURRENT)
#define NOT_CURRENT(x)	_nd_clearflag(x, OBJ_CURRENT)
#define CLICKED(x)	_nd_setflag(x, OBJ_CLICKED)
#define REGULAR(x)	_nd_clearflag(x, OBJ_CLICKED)

extern Obj * _nd_newObj(pfo, enum Class, void*,
                        char*, char*,
			int, int, int, int, char*);

extern int _nd_callback(Obj *, void*);

/*
 * A _nd_display is an object containing the necessary information for
 * writing something to the screen.  For curses, it contains a window and
 * an X,Y origin IN THAT WINDOW for drawing items.
 */
typedef struct _nd_display {
    void* window;	/* the curses window */
    int x, y;		/* X, Y origin */
} Display;

extern void* newDisplay(void*, int, int);
extern void deleteDisplay(Display*);

#define DISPLAY(w)	((Display*)(w))

/* these are macros because they need to be tweaked for every display
 * system.  The internals to Display objects is NOT published, so we
 * shouldn't be bitten in the ass if we change to a Win32 or X interface
 */
#define Window(w)	((WINDOW*)(DISPLAY(w)->window))
#define WX(w)		(DISPLAY(w)->x)
#define WY(w)		(DISPLAY(w)->y)


/* we have a local input function so we can painlessly deal with tty devices
 * that don't have function keys.  Control-X <digit> maps to F<digit>, all other
 * input passes directly through.
 */
extern int ndgetch(Display* from);


#if DYNAMIC_BINDING
/*
 * we keep a table of registered objects, so that users can add their own
 * widgets without having to mess with the internals of ndialog.
 *
 * the basic types are hardwired into the table, and we provide room
 * for ND_TABLE_SIZE-(# of base objects) widgets.
 */

#define ND_TABLE_SIZE	50	/* live a little... */

typedef editCode (*nd_edit)(void*,void*,void*,editCode);
typedef void     (*nd_draw)(void*,void*);
typedef void     (*nd_free)(void*);
typedef va_list  *(*nd_bind)(void*, va_list*);
typedef int      (*nd_size)(void*, int, int*, int*);
typedef int      (*nd_getp)(void*);
typedef int      (*nd_setp)(void*, int);

struct _nd_object_table {
    int used;
    nd_edit edit;
    nd_draw draw;
    nd_free free;
    nd_bind bind;
    nd_size size;
    nd_getp getp;
    nd_setp setp;
};

#define SZOBJTAB	sizeof(struct _nd_object_table)

extern int nd_register_object(nd_edit, nd_draw, nd_free, nd_bind, nd_size);
extern int nd_register_objtab(int size, struct _nd_object_table *);

extern va_list *nd_bindToType(void*, va_list*);
extern va_list *nd_bindToString(void*, va_list*);
extern va_list *nd_bindToList(void*, va_list*);
extern va_list *nd_bindToText(void*, va_list*);

extern int getListCursor(Obj*);
extern int setListCursor(Obj*,int);

extern int getStringCursor(Obj*);
extern int setStringCursor(Obj*,int);

extern int setTextCursor(Obj*,int);

extern int nd_buttonSize(void*, int, int*, int*);
extern int nd_typeSize(void*, int, int*, int*);

extern struct _nd_object_table nd_object_table[];

#endif

#endif/*ND_OBJECTS_D*/
