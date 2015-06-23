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
#include "cstring.h"


/*
 * we want to keep a stack of hrefs, so we can use escape to back out through
 * nested references.  In the future, we want to retain context, so we can
 * back out to the exact point we were at before.
 */
typedef struct {
    char *file;
    void *cursor;
} Page;

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


static char *
helpfile(char *doc, char *prev)
{
    char *ret, *q;
    
    if ( *doc == '/' ) {
	ret = calloc ( 1, (root ? strlen(root) : 0) + 2 + strlen(doc) );

	if ( root ) strcpy(ret, root);

	strcat(ret, doc);
	return ret;
    }
    else if ( prev ) {
	ret = malloc(strlen(prev) + strlen(doc) + 2 );
	strcpy(ret, prev);
	if ( *doc == '#' ) {
	    if  ( q = strrchr(ret, '#') )
		*q = 0;
	}
	else if ( q = strrchr(ret, '/') )
	    *++q = 0;
	else if ( q = strchr(ret, '#') )
	    *q = 0;

	strcat(ret, doc);
	return ret;
    }
    else
	return strdup(doc);
}


/*
 * _nd_help() displays helpfiles
 */
void
_nd_help(char *document)
{
    STRING(Page) pages;
    Page *cur, *up = 0;
    int rc;
    void *help, *chain;
    char *topic;		/* help topic title, for putting on the
				 * help box titlebar*/

    if (document == 0)
	return;

    CREATE(pages);

    cur = &EXPAND(pages);
    
    cur->cursor = 0;
    cur->file = helpfile(document, root);

    do {
	help = newHelp(0, 0, (COLS*3)/4, LINES-10,
		       cur->file, (pfo)ndhcallback, 0);

	/*setObjTitle(help, cur->file);*/

	if (cur->cursor)
	    setHelpCursor(help, cur->cursor);

	chain = ObjChain(help, newCancelButton(0,"Done", 0, 0));

	rc = MENU(chain, -1, -1, getHelpTopic(help), 0, 0);

	if (rc == MENU_OK) {
	    if ( topic = currentHtmlTag(help) ) {
		cur->cursor = getHelpCursor(help);

		up = cur;
		cur = &EXPAND(pages);
		cur->cursor = 0;
		cur->file = helpfile(topic, up ? up->file : root);
	    }
	}
	else if (rc == MENU_ESCAPE) {
	    free(cur->file);
	    S(pages)--;
	    up = (S(pages) > 1) ? &T(pages)[S(pages)-2]:0;
	    cur = &T(pages)[S(pages)-1];
	}
	else if (rc == MENU_CANCEL) {
	    int i;
	    for (i = 0; i < S(pages); i++)
		free(T(pages)[i].file);
	}
	deleteObjChain(chain);
    } while ( S(pages) > 0 );
    DELETE(pages);
#if HAVE_DOUPDATE
    doupdate();
#else
    refresh();
#endif
} /* _nd_help */

