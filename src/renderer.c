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
 * renderer: code that actually renders a helpfile html page
 */

#include "html.h"
#include "bytecodes.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


#define PAGE		(state.page->page)
#define PAGELEN		(state.page->pagelen)
#define PAGEALLOC	(state.page->pagealloc)
#define LASTWASSPACE	(state.page->lastwasspace)
#define XP		(state.page->xp)
#define STARTX		(state.page->startx)
#define HREFS		(state.page->hrefs)
#define NRHREFS		(state.page->nrhrefs)


/* our current state */
struct Format state;

static void linestart();

/*
 * need grows the rendered page to fit the text we're trying to add in
 */
static void
need(int size)
{
    if (PAGELEN + size > PAGEALLOC) {
	PAGEALLOC *= 2;
	PAGE = realloc(PAGE, PAGEALLOC);
	/* and if this doesn't work, we politely crash without warning.
	 * This should be fixed with a longjmp to the end of the rendering
	 * code.
	 */
    }
} /* need */


/*
 * addchar() adds a single character to a rendered page
 */
static void
addchar(char c)
{
    need(1);
    PAGE[PAGELEN++] = c;
}


/*
 * addbcf() adds a bytecoded font control character to a rendered page
 */
void
addbcf(char c)
{
    linestart();
    need(2);
    PAGE[PAGELEN++] = bcfID;
    PAGE[PAGELEN++] = c;
}


/*
 * font style changers
 */
void set_bold()		{ state.page->style |= St_BOLD; addbcf(bcfSET_B); }
void clear_bold()	{ state.page->style &= ~St_BOLD; addbcf(bcfCLEAR_B); }
void set_italic()	{ state.page->style |= St_ITALIC; addbcf(bcfSET_I); }
void clear_italic()	{ state.page->style &= ~St_ITALIC; addbcf(bcfCLEAR_I); }
void set_tt()		{ state.page->style |= St_FIXED; addbcf(bcfSET_TT); }
void clear_tt()		{ state.page->style &= ~St_FIXED; addbcf(bcfCLEAR_TT); }


/*
 * addbct() is a local that actually adds a bct to a rendered page
 */
static void
addbct(char c, char *s)
{
    linestart();
    need(strlen(s)+3);

    PAGE[PAGELEN++] = bctID;
    PAGE[PAGELEN++] = c;

    while (*s)
	PAGE[PAGELEN++] = *s++;
    PAGE[PAGELEN++] = bctID;
}


/*
 * addlabel() adds a html label to a rendered page
 */
void
addlabel(char *s)
{
    addbct(bctLABEL, s);
}


/*
 * add_href_index() adds a tag to the page's href array and returns
 * the index for this tag
 */
int
add_href_index(char *tag)
{
    HREFS = realloc(HREFS, (1+NRHREFS) * sizeof(char**));
    HREFS[NRHREFS] = strdup(tag);
    return NRHREFS++;
} /* add_href_index */


/*
 * addhref() adds a start-of-href tag to a rendered page
 */
int
addhref(char *tag)
{
    char s[20];
    int refno = add_href_index(tag);

    sprintf(s, "%d", refno);
    addbct(bctSET_A, s);
    return refno;
} /* addhref */


/*
 * endhref() adds a end-of-href tag to a rendered page
 */
void
endhref(int refno)
{
    char s[20];

    sprintf(s, "%d", refno);
    addbct(bctCLEAR_A, s);
} /* endhref */


/*
 * start_title() initializes the title of the page
 */
void
start_title()
{
    if (state.page->title)
	free(state.page->title);
    state.page->title = malloc(1);
    state.page->title[0] = 0;
    state.page->titlelen = 0;
} /* start_title */


/*
 * setindent() writes an indent code to the start of the current line
 */
static void
setindent(unsigned int indent)
{
    if (indent > 96) indent = 96;
    PAGE[STARTX+1] = (char)(indent+32);
} /* setindent */


/*
 * addnewline() writes an end-of-line to the rendered page
 */
static void
addnewline()
{
    addchar('\n');			/* put out the end-of-line marker */
    XP = 0;
    LASTWASSPACE = 0;
    STARTX = PAGELEN;			/* mark the start of the next line */
    addchar(DLE);
    addchar(' ');
    state.page->isbol = 1;
} /* addnewline */


/*
 * flushline() finalizes a rendered line and sets up for the next line.
 */
static int
flushline()
{
    int indent = state.indent;

    /* deal with leading indent, which is not clickable */

    if (XP > 0) {
	switch (state.align) {
	case wwRIGHT:
		if (XP < state.width)
		    indent += state.width - XP;
		break;
	case wwCENTER:
		if (XP < state.width)
		    indent += (state.width - XP) / 2;
		break;
	default:
		/* everything else is left-alignment */
		break;
	}
	setindent(indent);
	addnewline();
	return 1;
    }
    return 0;
} /* flushline */


/*
 * breakline() breaks the current line, if there's anything to be broken
 */
void
breakline()
{
    flushline();
} /* breakline */


/*
 * newline() breaks the current line, even if there's nothing to be
 * broken on it.
 */
void
newline()
{
    if (!flushline())
	addnewline();
} /* newline */


/*
 * addstring() is a local that writes a string to the rendered page,
 * properly dealing with escape codes, and ignoring \n
 */
static void
addstring(unsigned char *s)
{
    while (*s) {
	if ( (*s == bcfID) || (*s == DLE) || (*s == bctID) )
	    addchar(*s);
	if (*s == '\n')
	    ++s;
	else
	    addchar(*s++);
    }
}


/*
 * linestart() writes all active tags at the start of each line, so that
 * whatever displays the rendered text doesn't have to worry its pretty
 * little head about context
 */
static void
linestart()
{
    struct Format *cp;

    if (state.page->isbol) {
	state.page->isbol = 0;

	for (cp = &state; cp; cp = cp->parent)
	    if ((cp->flags & DF_A) && (cp->href > 0)) {
		char s[20];
		sprintf(s, "%d", cp->href);
		addbct(bctSET_A, s);
	    }
	if (state.page->style & St_BOLD)
	    addbcf(bcfSET_B);
	if (state.page->style & St_ITALIC)
	    addbcf(bcfSET_I);
	if (state.page->style & St_FIXED)
	    addbcf(bcfSET_TT);
    }
} /* linestart */


/*
 * addword() adds a word to the rendered page, breaking the line as
 * appropriate.
 */
void
addword(char *word)
{
    int siz = strlen(word);
    char *p;

    switch (state.doing) {
    case D_PRE:
	addstring((unsigned char*)word);
	break;
    case D_TITLE:
	state.page->titlelen += siz;
	state.page->title = realloc(state.page->title, state.page->titlelen+2);
	strcat(state.page->title, word);
	break;
    default:
	if (XP + siz > state.width)
	    breakline();
	if (state.style & St_CAPS) {
	    for (p=word; *p; ++p)
		*p = toupper(*p);
	}
	linestart();

	addstring((unsigned char*)word);
	XP += siz;
	break;
    }
    LASTWASSPACE = 0;
} /* addword */


/*
 * addspace() adds space to the rendered page, breaking the line as appropriate
 */
void
addspace(char *space)
{
    switch (state.doing) {
    case D_PRE:
	    /* when doing a PRE segment, we need to catch \n's and properly
	     * expand them into \n, DLE, ' ' */
	    while (*space) {
		if (*space == '\n')
		    addnewline();
		else {
		    linestart();
		    addchar(*space);
		}
		++space;
	    }
	    break;
    case D_TITLE:
	    if (!LASTWASSPACE) {
		state.page->titlelen++;
		state.page->title = realloc(state.page->title,
					    state.page->titlelen+2);
		strcat(state.page->title, " ");
	    }
	    break;
    default:
	    if (!LASTWASSPACE) {
		if (XP >= state.width)
		    breakline();
		else if (XP > 0) {
		    linestart();
		    addchar(' ');
		    XP++;
		}
	    }
	    break;
    }
    LASTWASSPACE = 1;
} /* addspace */


/*
 * render() processes a html page and returns a buffer containing the
 * rendered page
 */
Page *
render(FILE *input, int screenwidth)
{
    Page *bfr;

    if ((bfr = malloc(sizeof *bfr)) == 0)
	return 0;

    bfr->pagealloc= 10240;			/* alloc 10k for the page */
    bfr->page     = malloc(bfr->pagealloc);
    bfr->pagelen  = 0;				/* nothing written yet */
    bfr->xp       = 0;				/* set up xp and start of */
    bfr->startx   = 0;				/* line */
    bfr->hrefs    = malloc(1);			/* prepare the href array */
    bfr->nrhrefs  = 0;
    bfr->title    = 0;				/* ... the title */
    bfr->titlelen = 0;
    bfr->isbol    = 1;				/* and mark beginning of line */

    memset(&state, 0, sizeof state);		/* reset state block */
    state.align = wwLEFT;
    state.width = screenwidth;
    state.doing = D_VANILLA;
    state.page = bfr;

    addchar(DLE);
    addchar(' ');
    parse_it(input, 0, 0, ALL_TAGS);
    addchar(0);					/* null-terminate the page */
    bfr->pagelen--;

    return bfr;
} /* render */


/*
 * deletePage() is an unpublished routine that wipes out a Page*
 */
void
deletePage(Page *page)
{
    int x;

    if (page) {
	free(page->page);
	if (page->title)
	    free(page->title);

	for (x=0; x<page->nrhrefs; x++)
	    free(page->hrefs[x]);
	if (page->hrefs)
	    free(page->hrefs);
	free(page);
    }
} /* deletePage */
