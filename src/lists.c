/*
 *   Copyright (c) 1998 David Parsons. All rights reserved.
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
 * new dialog library:  libdialog compatability list/menu/checkboxes.
 */
#include "nd_objects.h"
#include "curse.h"
#include "dialog.h"
#include <errno.h>
#include <string.h>
#include <stdlib.h>


/*
 * menucallback() is a local function that tells MENU() to abandon ship NOW.
 */
static int
menucallback(void *obj, void* w)
{
    /* we just want to exit the form */
    return -1;
} /* menucallback */

/*
 * ndialog_list() is a new core function that does menus, checklists,
 * and radio lists.
 *
 * arguments:
 *	title		-- window title
 *	prompt		-- descriptive text inside the window
 *	width		-- the width of the window, or -1 if you want it to be
 *			   computed automatically.
 *	depth		-- the depth of the window, or -1.
 *	list_height	-- how many items visible in the list
 *	nrlist		-- how many items in the list
 *	listarray	-- the list, in dialog format:
 *			   <id><desc> for menus, <id><desc><switch> otherwise
 *	ch,sc		-- current selection (dialog compatable, but only
 *			   ch is ever set)
 *	mode		-- what we want to display:  ND_A_MENU, ND_A_LIST,
 *			   ND_A_RADIOLIST
 *	oklabel		-- how the OK button is labelled (if null, there
 *			   will be no ok button)  You can specify the location
 *			   of this button on the buttonbar by prefixing it
 *			   with <number>: -- the buttons will be sorted left
 *			   to right in ascending number order.
 *	cancellabel	-- how the Cancel button is labelled (if null, there
 *			   will be no cancel button)
 *
 * ndialog_list returns:
 *		-1	-- something went wrong.  Look at errno for details.
 *			   (This is NOT how libdialog works; when a libdialog
 *			    message box has something go wrong, it crashes
 *			    the program.)
 */
int
ndialog_list(char *title, char *prompt,
	     int height, int width, int list_height, int nrlist,
	     char **listarray,
				 /* format depends on what we are:
				  * for menus: ID, then ITEM
				  * for lists: MODE, then ID, then ITEM
				  */
	     char *result, int *ch, int *sc, 
	     enum dialog_list_mode mode,
	     char *oklabel, char *cancellabel)
{
    ListItem *list;
    void *chain;
    void *listobj;

    void *ok = 0, *cancel = 0;

    int rc;
    int retval = -1;
    int x;
    int nr_selected=0;
    int tw, itemwidth = 0, idwidth = 0;
    int menuwidth=0, framewidth=0;
    int promptdepth = prompt ? strdepth(prompt) : 0;
    int promptwidth = prompt ? strwidth(prompt) : 0;
    char *p;

    if (oklabel) {
	if ((p = strchr(oklabel, ':')) != 0)
	    ok = newOKButton(atoi(oklabel), p+1, 0, 0);
	else
	    ok = newOKButton(1, oklabel, 0, 0);
    }

    if (cancellabel) {
	if ((p = strchr(cancellabel, ':')) != 0)
	    cancel = newCancelButton(atoi(cancellabel), p+1, 0, 0);
	else
	    cancel = newCancelButton(2, cancellabel, 0, 0);
    }

    if ((list = malloc(nrlist * sizeof list[0])) == (ListItem*)0)
	return -1;

    /* allocate and build a ListItem array, and, while we're at it,
     * figure out how wide the menu should be.
     */
    for (idwidth = itemwidth = x = 0; x<nrlist; x++) {
	if (mode == ND_A_MENU) {
	    list[x].id  = listarray[x+x];
	    list[x].item = listarray[1+x+x];
	    list[x].selected = (x == (*ch));
	}
	else {
	    list[x].id = listarray[x+x+x];
	    list[x].item = listarray[x+x+x+1];
	    list[x].selected = (strcasecmp(listarray[x+x+x+2],"on") == 0);
	}
	if (list[x].selected)
	    ++nr_selected;
	if ((tw=strlen(list[x].id)) > idwidth)
	    idwidth = tw;
	if ((tw=strlen(list[x].item)) > itemwidth)
	    itemwidth = tw;
	list[x].help = 0;
    }
    if ((mode == ND_A_RADIOLIST) && (nr_selected != 1)) {
	/* oops.  Radio lists can only have one item selected */
	errno = EINVAL;
	goto byebye;
    }
    menuwidth = idwidth + 2 + itemwidth + 2;
    if (mode != ND_A_MENU)
	menuwidth += 4;	/* for `[x] ' */
    framewidth = (menuwidth > promptwidth) ? menuwidth : promptwidth;

    /* make sure that the frame is wide enough for the buttons */
    tw = 0;
    if (cancellabel)
	tw = strlen(cancellabel)+2;
    if (mode != ND_A_MENU && oklabel)
	tw += strlen(oklabel)+2;
    if (framewidth < tw)
	framewidth = tw;

    /* check that the height and width are okay */

    /* height.  `2' because that's the depth of a buttonbar */
    if (height > 0 && height < list_height+promptdepth+2) {
	errno = EINVAL;
	goto byebye;
    }
    /* width */
    if (width > 0 && width < framewidth) {
	errno = EINVAL;
	goto byebye;
    }

    /* now we can allocate a chain and fire up MENU()
     */
    if (mode == ND_A_MENU)
	listobj = newMenu(-1,0, -1, list_height, nrlist, list, 0, "",
				SHOW_IDS, menucallback, 0);
    else if (mode == ND_A_RADIOLIST)
	listobj = newRadioList(-1,0,-1,list_height,nrlist, list, 0, "",
				SHOW_IDS,0,0);
    else
	listobj = newList(-1,0,-1,list_height, nrlist, list, 0, "",
				SHOW_IDS, 0, 0);
    setObjCursor(listobj, *ch);
    chain = ObjChain(0, listobj);
    if (oklabel)
	chain = ObjChain(chain, ok);
    if (cancellabel)
	chain = ObjChain(chain, cancel);

    rc = MENU(chain, width, height, title, prompt, 0);

    *sc = 0;
    *ch = getObjCursor(listobj);

    deleteObjChain(chain);

    result[0] = 0;
    if (rc == MENU_OK) {
	retval = 0;
	for (x=0; x<nrlist;x++)
	    if (list[x].selected) {
		strcat(result, list[x].id);
		strcat(result, "\n");
	    }
    }
    else if (rc == MENU_ESCAPE || rc < MENU_ERROR)
	retval = -1;
    else
	retval = 1;

byebye:
    free(list);
    return retval;
} /* ndialog_list */


/*
 * dialog_menu() displays a menu and lets you pick one thing from it.
 * buttons.
 *
 * DIFFERENCES FROM DIALOG:  There is no OK button, just a cancel
 * button.
 */
int
dialog_menu(char *title, char *prompt,
            int height, int width,
	    int menu_height, int nrmenu,
	    char **menu, /* duples; KEY, then ITEM */
	    char *result,
	    int *ch, int *sc)
{
    return ndialog_list(title,prompt,height,width,
			menu_height,nrmenu,(char**)menu,
			result,ch,sc,
			ND_A_MENU,0,"1:CANCEL");
} /* dialog_menu */


/*
 * dialog_checklist() displays a checklist menu.
 */
int
dialog_checklist(char *title, char *prompt,
		    int height, int width, int menu_height, int nrmenu,
		    char **menu, /* triples: ID, then ITEM, then ON */
		    char *result)
{
    int ch=0, sc=0;

    return ndialog_list(title,prompt,height,width,
			menu_height,nrmenu,(char**)menu,
			result,&ch,&sc,
			ND_A_LIST,"1:  OK  ","2:CANCEL");
} /* dialog_checklist */


/*
 * dialog_radiolist() displays a radio button list.
 */
int
dialog_radiolist(char *title, char *prompt,
		    int height, int width, int menu_height, int nrmenu,
		    char **menu, /* triples: ID, then ITEM, then ON */
		    char *result)
{
    int ch=0, sc=0;

    return ndialog_list(title,prompt,height,width,
			menu_height,nrmenu,(char**)menu,
			result,&ch,&sc,
			ND_A_RADIOLIST,"1:  OK  ","2:CANCEL");
} /* dialog_checklist */
