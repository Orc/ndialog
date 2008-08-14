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
 * published interface for the ndialog library
 */
#ifndef NDIALOG_D
#define NDIALOG_D 1

#ifndef DIALOG_CHAR
#define DIALOG_CHAR	char
#endif

/*
 * a ListItem contains a list entry for a Menu, List or RadioList object
 */
typedef struct {
    char *id;		/* libdialog compatability id */
    char *item;		/* the description on the list line */
    char *help;		/* the help context for this item */
    int  selected;	/* on/off flag for this item */
} ListItem;

/*
 * an LIA is an object containing a ListItem array.  It should be used
 * internally in the appropriate display objects, but that won't be for
 * a while.
 */
typedef void* LIA;

LIA newLIA(ListItem *, int);
void deleteLIA(LIA);
int addToLIA(LIA, char*, char*, char*);
int delFromLIA(LIA, int);
int LIAcount(LIA);
ListItem *LIAlist(LIA);


enum Class { O_ERROR=0, O_STRING=1, O_CHECK=2,
             O_BUTTON=3, O_LIST=4, O_TEXT=5,
	     O_GAUGE=6, W_LIST=15, W_FILESEL=16 };

typedef void* ndObject;
typedef void* ndDisplay;

#ifndef ND_OBJECTS_D
typedef int (*pfo)(ndObject, ndDisplay);
#endif

#define ESCAPE	27

/* Obj constructors */

ndObject newString(int, int, int, int, char*, char*, char*, pfo, char*);
		/* x, y, width, size, bfr, prompt, prefix, callback, help*/
ndObject newPWString(int, int, int, int, char*, char*, char*, pfo, char*);
		/* x, y, width, size, bfr, prompt, prefix, callback, help*/

ndObject newCheck(int,int,char *,char*,char*,pfo,char*);
		/* x, y, prompt, prefix, bfr, callback, help */
ndObject newButton(int,char *,pfo,char*);
		/* x, prompt, callback, help */
ndObject newCancelButton(int,char*,pfo,char*);
		/* x, prompt, callback, help */
ndObject newOKButton(int,char*,pfo,char*);
		/* x, prompt, callback, help */
ndObject newText(int,int,int,int,int,char*,char*,char*,pfo,char*);
		/* x, y, width, height, size, prompt, prefix, bfr,
		 * callback, help
		 */
ndObject newHelp(int,int,int,int,char*,pfo,char*);
		/* x, y, width, height, document, callback, help */
ndObject newList(int,int,int,int,int,ListItem *,char*,char*,int,pfo,char*);
		/* x, y, width, height, nritems, items,
		 * prompt, prefix, displayas, callback, help */
#define CHECK_SELECTED		0x00
#define HIGHLIGHT_SELECTED	0x01
#define MENU_SELECTION		0x02
#define LO_SELECTION_MASK	0x03
#define SHOW_IDS		0x04	/* show ListItem.key & ListItem.item */
#define NO_HOTKEYS		0x08
#define DONT_CLIP_ITEMS		0x10	/* don't clip overly long selections */
#define DEL_LIST		0x20	/* DELETE selects items */
#define ALWAYS_HIGHLIT		0x40	/* always highlight current selection */
#define CR_LIST			0x80	/* RETURN selects items (default) */

ndObject newRadioList(int,int,int,int,int,ListItem *,char*,char*,int,pfo,char*);
		/* x, y, width, height, nritems, items,
		 * prompt, prefix, displayas, callback, help */
ndObject newMenu(int,int,int,int,int,ListItem *,char*,char*,int,pfo,char*);
		/* x, y, width, height, nritems, items,
		 * prompt, prefix, flags, callback, help */

ndObject newGauge(int, int, int, int*, char*, char*);
		/* x, y, width, percent, prompt, prefix*/

ndObject copyObj(ndObject);		/* copy constructor */
void deleteObj(ndObject);		/* destructor */


/*
 * Obj utility functions
 */

void  setObjTitle(ndObject,char*);	/* change the object title */
char* objTitle(ndObject);		/* what is the object title? */
void  setObjData(ndObject,...);		/* change the data field */
void  setObjHelp(ndObject,char*);	/* set the help information */
void  *objDataPtr(ndObject);		/* get the object data pointer */
char* objHelp(ndObject);		/* what is the help information? */
int   objSize(ndObject,int*,int*);	/* what is the object size? */
int   objSizeof(ndObject,int,int*,int*);/* what are the object sizes? */
void  setUserData(ndObject,void*);	/* set the user data area */
void* getUserData(ndObject);		/* get the user data area */
int   objAt(ndObject,int*,int*);	/* where is the object located? */
int   objDataAt(ndObject,int*,int*,int*,int*);	/* where is the data located? */
enum Class objType(ndObject);		/* what type is this object (int) ? */
char* objId(ndObject);			/* what sort of object am I (text)? */
int   isOKbutton(ndObject);		/* is this object an OK button? */
int   isCANCELbutton(ndObject);		/* is this object a CANCEL button? */
int   setButtonDataArea(ndObject,int,int);	/* set the display area for a button */
int   currentSelection(ndObject);	/* what's the current list selection? */
ListItem *getObjList(ndObject);		/* get the list out of a list object */
int getObjListSize(ndObject);		/* get the number of items in a list */
int   getObjCursor(ndObject);		/* get the current position in an
					 * applicable object
					 */
int   setObjCursor(ndObject,int);	/* set the current position in an
					 * applicable object
					 */

void  setWritable(ndObject);		/* make the object read/write */
void  setReadonly(ndObject);		/* make the object readonly */
int   writable(ndObject);		/* can we write to this object */
void  hideObj(ndObject);		/* don't display this object */
void  unhideObj(ndObject);		/* display this object */
void  touchObj(ndObject);		/* mark this object as needing to
					 * be redisplayed
					 */
void  untouchObj(ndObject);		/* don't redisplay this object */
int   touched(ndObject);		/* does this object need to be 
					 * redisplayed?
					 */
int   isDirty(ndObject);		/* was this object modified? */

char* getHelpTopic(ndObject);		/* get the title from a helpfile */
char* currentHtmlTag(ndObject);		/* get the current selection from a */
					/* helpfile */
void setHelpRoot(char*);		/* set the root directory for
                                         * help files */
void* getHelpCursor(ndObject);		/* get the current location in
					 * a helpfile */
int setHelpCursor(ndObject,void*);	/* set the current location in
					 * a helpfile */


void drawObj(ndObject, void*);
	    /* object, display */

typedef enum { eNOP,    eERROR,   eCANCEL,eTAB,
               eBACKTAB,eRETURN,  eEVENT, eESCAPE,
	       eREFRESH,eEXITFORM,eRESIZE } editCode;
editCode editObj(ndObject,void*,void*,editCode);
	    /* object, Display, event, flags */


/* functions to chain objects together, sort them, and delete them
 */
ndObject ObjChain(ndObject, ndObject);	/* add an object to an object chain */
	    /* chain, new item */
ndObject extractFromObjChain(ndObject, ndObject);
ndObject sortObjChain(ndObject);	/* sort a chain by x/y coordinates */
	    /* chain */
void deleteObjChain(ndObject);		/* delete all the objects in a chain */
	    /* chain */


/*  libdialog compatability glue.
 */
int ndialog_yesno(char*,char*,int,int,char*,char*);

enum dialog_list_mode {ND_A_MENU,ND_A_LIST,ND_A_RADIOLIST};
int ndialog_list(char*,char*,int,int,int,int,char**,
                 char*,int*,int*,enum dialog_list_mode,char*,char*);


/* default error display function.
 */
void Error(char *,...);

/* MENU() is the main display driver.
 */
int MENU(ndObject,int,int,char*,char*,int);	/* process an object chain */
	/* chain,width,depth,title,prompt,flags) */

/* MENU() flags
 */
#define FANCY_MENU	0x01		/* fancy borders */
#define ALIGN_LEFT	0x02		/* align prompt text left */
#define ALIGN_RIGHT	0x04		/* align prompt text right */
#define ERROR_FLAG	0x08		/* special colors for error msgs */
#define AT_BUTTON	0x10		/* put the cursor on the first button */

/* MENU() return codes
 */
#define MENU_ERROR	-1		/* something went wrong, look at errno
					 * for details */
#define MENU_OK		0		/* OK button pressed, or a object edit
					 * function returned eEXITFORM */
#define	MENU_CANCEL	1		/* CANCEL button pressed */
#define	MENU_ESCAPE	2		/* ESC pressed */


/* Functions that do exactly the same as libdialog functions, so
 * we'll put prototypes for them here.
 */
void init_dialog();		/* ladies and gentlemen, start your engines */
void end_dialog();		/* game over, go home */
char *get_helpline();		/* get a pointer to the current helpline */
void use_helpline(char*);	/* set the helpline */
void restore_helpline(char*);	/* a rose by any other name would smell as
				 * sweet
				 */

/* Wrappers for running external programs
 */
void to_dialog();		/* switch into dialog terminal settings */
void from_dialog();		/* switch away from dialog terminal settings */


/*
 * Widgets
 */
ndObject newListWidget(int,int,int,int,LIA,char*,pfo,pfo,pfo,char*);
		/* x, y, width, height, list, prompt,
		 * insert callback, delete callback, dummy, help
		 */
ndObject newEditableList(int,int,int,int,LIA,char*,pfo,pfo,pfo,char*);
		/* x, y, width, height, list, prompt,
		 * insert callback, delete callback, update callback, help
		 */
ndObject ListWidgetObj(ndObject obj);

typedef int (*pfsel)(void*);
ndObject newFileSelector(int,int,int,int,char*,char*,char*,int,pfsel,pfo,pfo,char*);
		/* x, y, list_width, list_height, prompt,
		 * start_dir, result, result size, file select function,
		 * chdir_callback, fsel_callback, help
		 */

/*
 * color definitions (can be reset if you really want)
 */
extern int nd_colors[];
#define c__WINDOW	0
#define	WINDOW_COLOR	nd_colors[c__WINDOW]
#define c__SELECTED	1
#define SELECTED_COLOR	nd_colors[c__SELECTED]
#define c__BUTTON	2
#define BUTTON_COLOR	nd_colors[c__BUTTON]
#define c__PRESSED	3
#define PRESSED_COLOR	nd_colors[c__PRESSED]
#define c__WIDGET	4
#define WIDGET_COLOR	nd_colors[c__WIDGET]
#define c__TITLE	5
#define TITLE_COLOR	nd_colors[c__TITLE]
#define c__BG		6
#define BG_COLOR	nd_colors[c__BG]
#define c__RELIEF	7
#define	RELIEF_COLOR	nd_colors[c__RELIEF]
#define c__HOTKEY	8
#define HOTKEY_COLOR	nd_colors[c__HOTKEY]
#define c__ERROR	9
#define	ERROR_COLOR	nd_colors[c__ERROR]
#define c__ACTIVE	10
#define ACTIVE_COLOR	nd_colors[c__ACTIVE]

#endif /*NDIALOG_D*/
