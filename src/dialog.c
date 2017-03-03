/*
 * new dialog library:  glue library to link with code written against
 * libdialog.
 *
 * Copyright (C) 1996-2017 David L Parsons.
 * The redistribution terms are provided in the COPYRIGHT file that must
 * be distributed with this source code.
 */
#include <config.h>

#include "nd_objects.h"
#include "curse.h"
#include "dialog.h"
#include <stdlib.h>
#include <errno.h>
#include <string.h>


/*
 * dialog_notify spits up a message and returns without fuss or bother
 */
void
dialog_notify(char* message)
{
    dialog_mesgbox("Message", message, -1, -1);
} /* dialog_notify */


/*
 * dialog_mesgbox() puts up a message and waits for the user to
 * press [return] before continuing.
 *
 * DIFFERENCES FROM DIALOG:  You cannot scroll the message.
 */
int
dialog_mesgbox(char *title, char *mesg, int h, int w)
{
    void *chain = ObjChain(0, newOKButton(0, "  OK  ", 0, 0));
    int rc;

    rc = MENU(chain, w, h, title, mesg, 0);

    deleteObjChain(chain);

    return (rc == MENU_ESCAPE || rc == MENU_ERROR) ? -1 : 0;
} /* dialog_mesgbox */


/*
 * dialog_msgbox() puts up a message and optionally waits for the
 * user to press [return] before continuing.
 */
int
dialog_msgbox(char *title, char *msg, int h, int w, int wait)
{
    int rc;
    if (wait)
	return dialog_mesgbox(title, msg, h, w);
    rc = MENU((void*)0, w, h, title, msg, 0);

    return (rc == MENU_ESCAPE || rc == MENU_ERROR) ? -1 : 0;
} /* dialog_msgbox */


/*
 * dialog_clear_norefresh() clears the screen, then does a wnoutrefresh
 */
void
dialog_clear_norefresh()
{
    wclear(stdscr);
    touchwin(stdscr);
#if WITH_NCURSES
    wnoutrefresh(stdscr);
#endif
} /* dialog_clear_norefresh */


/*
 * dialog_clear() clears and refreshes the screen
 */
void
dialog_clear()
{
    wclear(stdscr);
    refresh();
} /* dialog_clear */


/*
 * dialog_update() refreshes the screen
 */
void
dialog_update()
{
    refresh();
} /* dialog_update */


/*
 * dialog_gauge() spits up a nice progress gauge, with some commentary
 */
void
dialog_gauge(char *title, char *prompt, int y, int x,
                                        int height, int width, int perc)
{
    void *chain;

    if (width < 0)
	return;

    if ( (chain = ObjChain(0, newGauge(0, 0, width, &perc, 0, ""))) != 0) {
	MENU(chain, -1, -1, title, prompt, 0);
	deleteObjChain(chain);
    }
    else
	Error("dialog_gauge(%s,%s,%d,%d,%d,%d,%d) -- chain is NULL",
		title ? title : "[null]", prompt ? prompt : "[null]",
		y, x, height, width, perc);
} /* dialog_gauge */


/*
 * dialog_inputbox() spits up a box that asks for string input
 */
int
dialog_inputbox(char *title, char *prompt, int height, int width, char* result)
{
    void *chain;
    int strwidth = (width < 0) ? (COLS-4) : width-4;
    int  rc;

    chain = ObjChain(newString(0, 0, strwidth, strwidth, result, 0, "", 0, 0),
		     newOKButton(0, "OK", 0, 0));
    chain = ObjChain(chain, newCancelButton(1, "CANCEL", 0,0));

    rc = MENU(chain, width, height,title,prompt,0);

    deleteObjChain(chain);

    return (rc ==MENU_OK) ? 0 : -1;
} /*  dialog_inputbox */
