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
static const char rcsid[] = "$Id$";
/*
 * new dialog library:  glue library to link with code written against
 * libdialog.
 */
#include "nd_objects.h"
#include "curse.h"
#include "dialog.h"
#include <malloc.h>
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
dialog_mesgbox(DIALOG_CHAR *title, DIALOG_CHAR *mesg, int h, int w)
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
dialog_msgbox(DIALOG_CHAR *title, DIALOG_CHAR *msg, int h, int w, int wait)
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
dialog_inputbox(DIALOG_CHAR *title, DIALOG_CHAR *prompt, int height, int width, DIALOG_CHAR* result)
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
