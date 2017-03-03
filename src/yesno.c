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
 * yesno: the YES/NO dialog message box
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
