/*
 * html.h: global header for html.c
 *
 * Copyright (C) 1996-2017 David L Parsons.
 * The redistribution terms are provided in the COPYRIGHT file that must
 * be distributed with this source code.
 */
#ifndef HTMLHELP_D
#define HTMLHELP_D

#include <stdio.h>

/* font styles */
#define St_BOLD		0x01		/* (boldface) */
#define	St_ITALIC	0x02		/* (italic)   */
#define St_CAPS		0x04		/* (ALL CAPS) */
#define St_FIXED	0x08		/* (fixed-width font)  */

/*
 * render() returns a struct Page{}, which contains a rendered page,
 * plus information about that page
 */
typedef struct {
    char *title;	/* pointer to page title */
    int titlelen;	/* length of page title */
    unsigned char *page;/* pointer to rendered page */
    int pagelen;	/* number of bytes of data in the page */
    int pagealloc;	/* number of bytes ALLOCATED for the page */
    int startx;		/* start of current line */
    int xp;		/* X offset on current line */
    int lastwasspace;	/* last token written to current line was a space */
    int isbol;		/* we're at the beginning of the line */
    int style;
    char **hrefs;	/* array of hrefs in the page */
    int nrhrefs;	/* number of hrefs in the page */
} Page ;

extern Page * render(FILE*, int);	/* render a file */
extern void deletePage(Page*);		/* delete a Page */

/*
 * functions that write things to a rendered page
 */
extern void addbcf(char);		/* write a BCF-encoded command */
extern void addspace(char*);		/* add whitespace */
extern void addword(char*);		/* add a word */
extern void addlabel(char*);		/* add a href label */
extern int addhref(char*);		/* start a href tag */
extern void endhref(int);		/* end a href tag */
extern void start_title();		/* start a new title */
extern void breakline();		/* break this line */
extern void newline();			/* add a newline */

extern void set_bold();			/* font options functions */
extern void clear_bold();
extern void set_italic();
extern void clear_italic();
extern void set_tt();
extern void clear_tt();

enum Tokens {
	YYEOF=0,			/* EOF, in lex's little mind */
	wwWORD,				/* unmatched text */
	wwLT,				/* '<' */
	wwGT,				/* '>' */
	wwEQ,				/* = */
	wwSPACE,			/* whitespace */
	wwHTML,
	wwHEAD,
	wwTITLE,
	wwBODY,
	wwPARA,
	wwA,
	wwNAME,
	wwHREF,
	wwBREAK,
	wwID,
	wwBOLD,				/* <B> .. </B> */
	wwITAL,				/* <I> .. </I> */
	wwTT,				/* <TT> .. </TT> */
	wwPRE,
	wwBANGDASHDASH,			/* <!-- starts a comment */
	wwDASHDASH,			/* --> ends a comment */
	wwBQ,				/* <BLOCKQUOTE> .. </BLOCKQUOTE> */
	wwALIGN,
	wwCENTER,
	wwLEFT,
	wwRIGHT,
	wwHR,
	wwHEADER,
	wwDL,
	wwDT,
	wwDD,
	wwWIDTH
	/* we can only support 32 tags with this compiler, because I
	 * use a long integer as a bitmap for available tags.
	 * With WIDTH, 31 tags are taken.
	 */
} ;

#define BIT(x)		(1<<(x-1))
#define ALL_TAGS	(~0)
#define ALL_BODY_TAGS	(ALL_TAGS & ~(BIT(wwTITLE)|BIT(wwHTML)|BIT(wwHEAD)))
struct Format {
    int style;				/* text style; a bitmap of */
					/* font states.            */
    int header_type;			/* digit on H1...H9        */
    enum Tokens align;			/* alignment, if applicable */
    int doing;				/* are we doing something special */
#define D_VANILLA	1		/* nope. */
#define D_TITLE		2		/* doing a title */
#define D_PRE		3		/* doing a PRE */
#define D_HEAD		4		/* inside a <HEAD> .. </HEAD> sect. */
#define D_LIST		5		/* inside a <DL> .. </DL> section */
    int indent, width;			/* physical format size */
    int flags;				/* `doing'-specific flags */
#define DF_DT	0x01			/* doing a <DT> section */
#define DF_A	0x02			/* doing an <A ...> section */
    int href;				/* href link inside an a section */
    struct Format *parent;		/* link to parent Format */
    Page *page;				/* where we're writing our output */
};


#define Save(f)		((f) = state, state.parent = &(f), state.page=(f).page)
#define Restore(f)	(state = (f))


extern struct Format state;		/* global rendering state */
extern void parse_it(FILE *, int, int, unsigned long);

#define MAXLEN	2000

#endif/*HTMLHELP_D*/
