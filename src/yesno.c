/*
 * yesno: the YES/NO dialog message box
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
 * dialog_yesno() puts up a message and gives the user the option of pressing
 * YES or NO.
 */
int
dialog_yesno(char *title, char *prompt, int height, int width)
{
    return ndialog_yesno(title, prompt, height, width, "1:YES", "2:NO");
} /* dialog_yesno */


/*
 * ndialog_yesno() puts up a message and gives the user a OKButton or a
 * CancelButton to get out.  You may specify the order in which the
 * buttons appear by prefixing them with the button number and a `:';
 * so 2:OK, 1:CANCEL has the CANCEL button on the LEFT side of the
 * messagebox.
 */
int
ndialog_yesno(char *title, char *prompt,
              int height, int width,
	      char *okbutton, char *cancelbutton)
{
    int rc;
    void *chain;
    char *p;

    if ((p=strchr(okbutton, ':')) != 0)
	chain = newOKButton(atoi(okbutton), 1+p, 0, 0);
    else
	chain = newOKButton(1, okbutton, 0, 0);

    if ((p=strchr(cancelbutton, ':')) != 0)
	chain = ObjChain(chain, newCancelButton(atoi(cancelbutton), 1+p, 0, 0));
    else
	chain = ObjChain(chain, newCancelButton(2, cancelbutton, 0, 0));

    rc = MENU(chain, width, height, title, prompt, 0);
    deleteObjChain(chain);

    if (rc == MENU_OK)
	return 0;
    else if (rc == MENU_CANCEL)
	return 1;
    else
	return -1;
} /* dialog_yesno */
