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
 * The online help system, accessed (probably) through F1 (or ?, on appropriate
 * objects
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "curse.h"
#include "nd_objects.h"
#include "ndialog.h"


/*
 * we want to keep a stack of hrefs, so we can use escape to back out through
 * nested references.  In the future, we want to retain context, so we can
 * back out to the exact point we were at before.
 */
typedef struct {
    char *file;
    void *cursor;
} HelpStackItem;

static char *root = 0;		/* root for helpfiles, set by setHelpRoot(). */

/*
 * setHelpRoot() sets the directory root for help files
 */
void
setHelpRoot(char *newroot)
{
    if (root)
	free(root);
    root = strdup(newroot);
} /* setHelpRoot */


/*
 * ndhcallback() is the callback for hrefs.  It returns -1 if we're
 * sitting on a href, 0 otherwise
 */
static int
ndhcallback(void *o)
{
    return currentHtmlTag(o) ? -1 : 0;
} /* ndhcallback */


/*
 * _nd_help() displays helpfiles
 */
void
_nd_help(char *document)
{
    HelpStackItem *stack;	/* stack is a stack of previously visited */
    int nrhelp = 0;		/* help topics, so we can rewind with ESC */
    int rc;
    void *help, *chain;
    char *p;
    char *topic;		/* help topic title, for putting on the
				 * help box titlebar*/

    if (document == 0)
	return;

    stack = malloc(sizeof stack[0]);
    if (stack == 0) {
	Error("malloc");
	return;
    }
    stack[0].cursor = 0;
    if (document[0] != '/') {
	stack[0].file = malloc((root?strlen(root):0) +2+ strlen(document));
	sprintf(stack[0].file, "%s/%s", root ? root : "", document);
    }
    else
	stack[0].file = strdup(document);

    do {
	help = newHelp(0, 0, (COLS*3)/4, LINES-10,
		       stack[nrhelp].file, (pfo)ndhcallback, 0);

	if (stack[nrhelp].cursor)
	    setHelpCursor(help, stack[nrhelp].cursor);

	chain = ObjChain(help, newCancelButton(0,"Done", 0, 0));

	rc = MENU(chain, -1, -1, getHelpTopic(help), 0, 0);

	if (rc == MENU_OK) {
	    if ((topic = currentHtmlTag(help)) != (char*)0) {
		stack[nrhelp].cursor = getHelpCursor(help);
		++nrhelp;
		stack = realloc(stack, (1+nrhelp)*sizeof stack[0]);

		stack[nrhelp].cursor = 0;
		if (topic[0] == '#') {
		    stack[nrhelp].file = malloc(strlen(stack[nrhelp-1].file) + 2 + strlen(topic));

		    strcpy(stack[nrhelp].file, stack[nrhelp-1].file);
		    if ((p = strchr(stack[nrhelp].file, '#')) != 0)
			*p = 0;
		    strcat(stack[nrhelp].file, topic);
		}
		else if (topic[0] != '/' && (p = strrchr(stack[nrhelp-1].file, '/')) != 0) {
		    ++p;
		    stack[nrhelp].file = malloc(strlen(stack[nrhelp-1].file)
						       + 2 + strlen(topic));
		    memcpy(stack[nrhelp].file,
			   stack[nrhelp-1].file,
			   (int)(p - stack[nrhelp-1].file));
		    stack[nrhelp].file[p-stack[nrhelp-1].file] = 0;
		    strcat(stack[nrhelp].file, topic);
		}
		else
		    stack[nrhelp].file = strdup(topic);
	    }
	}
	else if (rc == MENU_ESCAPE) {
	    if (nrhelp >= 0) {
		free(stack[nrhelp].cursor);
		free(stack[nrhelp].file);
		--nrhelp;
	    }
	}
	else if (rc == MENU_CANCEL) {
	    while (nrhelp >= 0) {
		free(stack[nrhelp].cursor);
		free(stack[nrhelp].file);
		--nrhelp;
	    }
	}
	deleteObjChain(chain);
    } while (nrhelp >= 0);
    free(stack);
#if HAVE_DOUPDATE
    doupdate();
#else
    refresh();
#endif
} /* _nd_help */
