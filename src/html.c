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
 * html: html help file scanner/parser.
 *
 * Bugs: This is a recursive descent parser, so we can't safely breakpoint
 * it and display only part of a file.
 */
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "html.h"
#include "bytecodes.h"

static char text[MAXLEN];


/*
 * lookup a word to see if it's something special
 */
int
lookup(char *text)
{
    int negate = (*text == '/') ? 1 : 0;

    if (negate) ++text;

    switch (*text) {
    case '!':
	    if (text[1] == '-' && text[2] == '-' && text[3] == 0)
		if (!negate)
		    return wwBANGDASHDASH;
	    break;
    case '-':
	    if (text[1] == '-' && text[2] == 0)
		if (!negate)
		    return wwDASHDASH;
	    break;
    case 'A': case 'a':
	    if (text[1] == 0)
		return negate ? -wwA : wwA;
	    else if (!negate && strcasecmp(1+text, "lign") == 0)
		return wwALIGN;
	    break;
    case 'B': case 'b':
	    if (text[1] == 0)
		return negate ? -wwBOLD : wwBOLD;
	    else if ((text[1] == 'R' || text[1] == 'r') && text[2] == 0) {
		if (!negate)
		    return wwBREAK;
	    }
	    else if (strcasecmp(1+text, "ody") == 0)
		return negate ? -wwBODY : wwBODY;
	    else if (strcasecmp(1+text, "lockquote") == 0)
		return negate ? -wwBQ : wwBQ;
	    break;
    case 'C': case 'c':
	    if (strcasecmp(1+text, "enter") == 0)
		return negate ? -wwCENTER : wwCENTER;
	    break;
    case 'D': case 'd':
	    if (text[2] != 0)
		break;
	    if (text[1] == 'l' || text[1] == 'L')
		return negate ? -wwDL : wwDL;
	    else if (text[1] == 't' || text[1] == 'T')
		return negate ? -wwDT : wwDT;
	    else if (text[1] == 'd' || text[1] == 'D')
		return negate ? -wwDD : wwDD;
	    break;
    case 'H': case 'h':
	    if (isdigit(text[1]) && text[2] == 0) {
		state.header_type = text[1]-'0';
		return negate ? -wwHEADER : wwHEADER;
	    }
	    else if ((text[1] == 'r' || text[1] == 'R') && text[2] == 0)
		return negate ? -wwHR : wwHR;
	    else if (strcasecmp(1+text, "ref") == 0) {
		if (!negate)
		    return wwHREF;
	    }
	    else if (strcasecmp(1+text, "tml") == 0)
		return negate ? -wwHTML : wwHTML;
	    else if (strcasecmp(1+text, "ead") == 0)
		return negate ? -wwHEAD : wwHEAD;
	    break;
    case 'I': case 'i':
	    if (text[1] == 0)
		return negate ? -wwITAL : wwITAL;
	    else if ((text[1] == 'd' || text[1] == 'D') && text[2] == 0) {
		if (!negate)
		    return wwID;
	    }
	    break;
    case 'L':
	    if (!negate && strcasecmp(text+1, "elt") == 0)
		return wwLEFT;
	    break;
    case 'N': case 'n':
	    if (!negate && strcasecmp(1+text, "ame") == 0)
		return wwNAME;
	    break;
    case 'P': case 'p':
	    if (text[1] == 0)
		return negate ? -wwPARA : wwPARA;
	    else if (strcasecmp(1+text, "re") == 0)
		return negate ? -wwPRE : wwPRE;
	    break;
    case 'R':
	    if (!negate && strcasecmp(text+1, "ight") == 0)
		return wwRIGHT;
	    break;
    case 'T': case 't':
	    if (text[2] == 0 && (text[1] == 'T' || text[1] == 't'))
		return negate ? -wwTT : wwTT;
	    if (strcasecmp(1+text, "itle") == 0)
		return negate ? -wwTITLE : wwTITLE;
	    break;
    case 'W': case 'w':
	    if (!negate && strcasecmp(1+text, "idth") == 0)
		return wwWIDTH;
	    break;
    }
    return wwWORD;
} /* lookup */



static int pushback = 0;

/*
 * unscan() pushes a token back onto the input stream.  You can push
 * back as many as you want, but only the last one will be saved.
 */
void
unscan(int text, FILE *f)
{
    pushback = text;
} /* unscan */


/*
 * grab a token off our input stream
 */
int
scan(FILE *f)
{
    register c;
    register st = wwWORD;
    int x = 0;
    int did_escape = 0;
    static int brace_level=0;

    if (pushback) {
	st = pushback;
	pushback = 0;
	goto endofscan;
    }
    memset(text, 0, sizeof text);

    while ((c = getc(f)) != EOF) {
	if (isspace(c)) {
	    if (x == 0) {
		do {
		    text[x++] = c;
		} while ((c=getc(f)) != EOF && isspace(c));
		text[x] = 0;
		ungetc(c,f);
		st = wwSPACE; goto endofscan;
	    }
	    else {
		ungetc(c, f);
		break;
	    }
	}
	if (x < sizeof text)
	    text[x++] = c;
	if (c == '<' || c == '>' || c == '=') {
	    /* handle < & > */
	    if (x == 1) {
		switch (c) {
		case '<':	st = wwLT; brace_level++; break;
		case '>':	st = wwGT;
				if (brace_level > 0)
				    brace_level--;
				break;
		case '=':	st = wwEQ;	break;
		}
		break;
	    }
	    else {
		--x;
		ungetc(c, f);
		break;
	    }
	}
	else if (c == '&') {
	    char little[20];
	    int lx = 0;

	    if (x != 1) {
		--x;
		ungetc(c, f);
		break;
	    }
	    did_escape = 1;
	    while ((c=getc(f)) != EOF && c != ';' && !isspace(c)) {
		if (lx < (sizeof little) - 1)
		    little[lx++] = c;
	    }
	    if (c != ';')
		ungetc(c, f);
	    little[lx] = 0;

	    if (x < sizeof text) {
		if (strcasecmp(little, "lt") == 0)
		    text[x-1] = '<';
		else if (strcasecmp(little, "gt") == 0)
		    text[x-1] = '>';
		else if (strcasecmp(little, "amp") == 0)
		    text[x-1] = '&';
		else if (strcasecmp(little, "emdash") == 0) {
		    text[x-1] = '-';
		    text[x++] = '-';
		}
		else if (little[0] == '#') {
		    for (c = 0, lx = 1; little[lx]; lx++)
			c = (c*10) + (little[lx] - '0');
		    text[x-1] = c;
		}
	    }
	}
	else if (c == '"' && brace_level > 0) {
	    /* snarf up strings */
	    --x;
	    if (x > 0) {
		ungetc(c,f);
		break;
	    }
	    while ((c = getc(f)) != EOF && c != '"')
		if (x < sizeof text)
		    text[x++] = c;
	}
    }
    if (c == EOF && x == 0)
	return YYEOF;
endofscan:
    if (x < sizeof text)
	text[x] = 0;
    if (st == wwWORD && !did_escape)
	st = lookup(text);

    return st;
} /* scan */


/*
 * scannw() grabs a token off our input stream, ignoring whitespace
 */
int
scannw(FILE *input)
{
    int tok;

    while ((tok = scan(input)) == wwSPACE)
	;
    return tok;
} /* scannw */


/*
 * block() changes indent and width for a BLOCKQUOTE section
 */
void
block()
{
    if (state.width > 8 ) {
	state.indent += 4;
	state.width -= 8;
    }
} /* block */


void eattag(FILE *);

/*
 * a_header() processes an wwA tag, returning a reference to any wwHREF=
 * found inside it.
 */
int
a_header(FILE *input)
{
    int tok;
    int tagid = EOF;

    while ((tok = scannw(input)) != wwGT && tok != YYEOF) {
	if (tok == wwNAME) {
	    if ((tok = scannw(input)) == wwEQ) {
		/* save name for later access */
		if ((tok = scannw(input)) == wwGT)
		    unscan(tok, input);
		else
		    addlabel(text);
	    }
	    else unscan(tok, input);
	}
	else if (tok == wwHREF) {
	    if ((tok = scannw(input)) == wwEQ) {
		if ((tok = scannw(input)) == wwGT)
		    unscan(tok, input);
		else if (tagid == EOF)
		    state.href = tagid = addhref(text);
	    }
	    else unscan(tok, input);
	}
    }
    return tagid;
} /* a_header */


/*
 * block_header() processes any blocking tag (P, BLOCKQUOTE, Hx, wwDL) and
 * deals with any wwID or wwALIGN's found inside it.
 */
void
block_header(FILE *input, int allow_align)
{
    int tok;

    while ((tok=scannw(input)) != wwGT && tok != YYEOF) {
	if (tok == wwID) {
	    if ((tok = scannw(input)) == wwEQ) {
		if ((tok = scannw(input)) != wwGT)
		    addlabel(text);
		else
		    unscan(tok, input);
	    }
	    else
		unscan(tok, input);
	}
	else if ((tok == wwALIGN) && allow_align) {
	    tok = scannw(input);
	    if (tok == wwEQ) {
		switch (tok = scannw(input)) {
		case wwLEFT:	state.align = wwLEFT;	break;
		case wwRIGHT:	state.align = wwRIGHT;	break;
		case wwCENTER:	state.align = wwCENTER;	break;
		default:	unscan(tok, input);
		}
	    }
	    else
		unscan(tok, input);
	}
    }
} /* block_header */


/* process a <wwHTML> .. </wwHTML> block */
void
do_html(FILE *input)
{
    block_header(input, 0);
    parse_it(input, -wwHTML, 0, BIT(wwHEAD)|BIT(wwBODY));
} /* do_html */


/* process a <wwHEAD> .. </wwHEAD> block */
void
do_head(FILE *input)
{
    block_header(input, 0);
    parse_it(input, -wwHEAD, 0, BIT(wwTITLE));
} /* do_head */


/* process a <wwBODY> .. </wwBODY> block */
void
do_body(FILE *input)
{
    block_header(input, 0);
    parse_it(input, -wwBODY, 0, ALL_BODY_TAGS);
} /* do_body */


/* process a <Hx> .. </Hx> block */
void
do_header(FILE *input, int header_type)
{
    struct Format sv;

    Save(sv);

    state.align = wwCENTER;

    if (header_type < 2)
	state.flags |= St_CAPS;

    block_header(input, 1);
    breakline();
    if (header_type < 4)
	addbcf(bcfSET_B);
    parse_it(input, -wwHEADER, header_type, ALL_BODY_TAGS);
    if (header_type < 4)
	addbcf(bcfCLEAR_B);
    breakline();

    Restore(sv);
} /* do_header */


/* process a <wwTITLE> .. </wwTITLE> block */
void
do_title(FILE *input)
{
    struct Format sv;
    Save(sv);

    state.doing = D_TITLE;

    start_title();
    block_header(input, 0);
    parse_it(input, -wwTITLE, 0, BIT(wwTITLE));
    Restore(sv);
} /* do_title */


/* process a <wwA...> .. </wwA> block */
void
do_a(FILE *input)
{
    struct Format sv;
    int tagid;

    Save(sv);

    state.flags = DF_A;

    tagid = a_header(input);
    parse_it(input, -wwA, 0, ALL_BODY_TAGS);

    if (tagid != EOF)
	endhref(tagid);

    Restore(sv);
} /* do_a */


/* process a <wwPRE> .. </wwPRE> block */
void
do_pre(FILE *input)
{
    struct Format sv;
    Save(sv);

    breakline();
    state.doing = D_PRE;
    set_tt();
    block_header(input, 0);
    parse_it(input, -wwPRE, 0, BIT(wwITAL)|BIT(wwTT)|BIT(wwBOLD)|BIT(wwHR));
    clear_tt();
    Restore(sv);
} /* do_pre */


/* process a <BLOCKQUOTE> ... </BLOCKQUOTE> block */
void
do_bq(FILE *input)
{
    struct Format sv;

    Save(sv);
    breakline();
    block();
    block_header(input, 1);
    parse_it(input, -wwBQ, 0, ALL_BODY_TAGS);
    breakline();
    newline();
    Restore(sv);
} /* do_bq */


/* process a <P> ... </P> block */
void
do_paragraph(FILE *input, int tok)
{
    struct Format sv;

    breakline();
    Save(sv);
    if (tok == wwCENTER)
	state.align = wwCENTER;
    block_header(input, 1);
    parse_it(input, -tok, 0, ALL_BODY_TAGS);
    breakline();
    newline();
    Restore(sv);
} /* do_paragraph */


/* do_list handles definition lists, in a terrifyingly ugly fashion */
void
do_list(FILE *input)
{
    struct Format sv;

    breakline();
    Save(sv);
    state.flags = 0;
    eattag(input);
    parse_it(input, -wwDL, 0, ALL_BODY_TAGS);
    breakline();
    newline();
    Restore(sv);
} /* do_list */


/* do_bullet handles a <wwDT> tag and the following text */
void
do_bullet(FILE *input)
{
    breakline();	/* push out any cached text */
				/* then set the indentation */
    state.indent = (state.parent)->indent;
    state.width = (state.parent)->width;
    state.flags = DF_DT;	/* and flag ourself */
} /* do_bullet */


/* do_text handles a <wwDD> tag and the following text */
void
do_text(FILE *input)
{
    breakline();
    state.indent = (state.parent)->indent + 10;
    state.width = (state.parent)->width - 10;
} /* do_text */


/* do_hr handles a <HT> tag: this is renderer-specific */
void
do_hr(FILE *input)
{
    long width=100;
    char hr[201];
    int tok;
    struct Format sv;

    while ((tok = scannw(input)) != YYEOF && tok != wwGT) {
	if (tok == wwWIDTH) {
	    tok = scannw(input);
	    if (tok == wwEQ) {
		tok = scannw(input);
		if (tok == wwWORD && strchr(text, '%') != 0)
		    width = atoi(text);
	    }
	}
	if (tok == wwGT)
	    break;
    }

    width = (state.width * width) / 100;
    if (width > 200)
	width = 200;
    memset(hr, '-', width);
    hr[width] = 0;
    Save(sv);
    breakline();
    state.align = wwCENTER;
    addword(hr);
    breakline();
    Restore(sv);
} /* do_hr */


/*
 * eattag() gobbles up the rest of a tag that we don't care about
 */
void
eattag(FILE *input)
{
    int tok;

    while ((tok = scan(input)) != wwGT && tok != YYEOF)
	;
}


/*
 * parse_it() handles a section of html code,
 */
void
parse_it(FILE *input, int endtag, int level, unsigned long allowed_tags)
{
    int tok;

    allowed_tags |= BIT(wwBANGDASHDASH)|BIT(wwGT);

    while ((tok=scan(input)) != YYEOF) {
	if (tok == wwLT) {
	    tok = scan(input);

	    if (tok == endtag && (endtag != -wwHEADER || level == state.header_type)) {
		eattag(input);
		return;
	    }

	    if (tok < 0) {
		if ((BIT(-tok) & allowed_tags) == 0) {
		    eattag(input);
		    continue;
		}
	    }
	    else if (tok > 0) {
		if ((BIT(tok) & allowed_tags) == 0) {
		    eattag(input);
		    continue;
		}
	    }

	    switch (tok) {
	    case wwBANGDASHDASH:
		while ((tok=scan(input)) != YYEOF)
		    if (tok == wwDASHDASH)
			if ((tok = scan(input)) == wwGT || tok == YYEOF)
			    break;
		break;
	    case wwHTML:
		do_html(input);
		break;
	    case wwHEAD:
		do_head(input);
		break;
	    case wwDL:
		do_list(input);
		break;
	    case wwDT:
		do_bullet(input);
		eattag(input);
		break;
	    case wwDD:
		do_text(input);
		eattag(input);
		break;
	    case wwBODY:
		do_body(input);
		break;
	    case wwCENTER:
	    case wwPARA:
		do_paragraph(input, tok);
		break;
	    case wwHEADER:
		do_header(input, state.header_type);
		break;
	    case wwBREAK:
		newline();
		eattag(input);
		break;
	    case wwTITLE:
		do_title(input);
		break;
	    case wwA:
		do_a(input);
		break;
	    case wwPRE:
		do_pre(input);
		break;
	    case wwHR:
		do_hr(input);
		break;
	    case wwBOLD:
		set_bold();
		eattag(input);
		break;
	    case -wwBOLD:
		clear_bold();
		eattag(input);
		break;
	    case wwBQ:
		do_bq(input);
		break;
	    case wwITAL:
		set_italic();
		eattag(input);
		break;
	    case -wwITAL:
		clear_italic();
		eattag(input);
		break;
	    case wwTT:
		set_tt();
		eattag(input);
		break;
	    case -wwTT:
		clear_tt();
		eattag(input);
		break;
	    case wwGT:
		/* end of tag; could it be a <> tag? */
		unscan(tok, input);
		eattag(input);
		break;
	    default:
		eattag(input);
		break;
	    }
	}
	else {
	    if (tok == wwSPACE)
		addspace(text);
	    else
		addword(text);
	}
    }
} /* parse_it */
