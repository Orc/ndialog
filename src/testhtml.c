/*
 * htmlhelp: html help file interpreter.
 *
 * Bugs: This is a recursive descent parser, so we can't safely breakpoint
 * it and display only part of a file.
 */
#include <config.h>

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "html.h"
#include "bytecodes.h"

/*
 * display() is a q&d rendered page displayer
 */
void
display(Page *p)
{
    register int i;

    if (p->title && p->titlelen > 0)
	printf("TITLE: %s\n", p->title);

    for (i=0; i<p->pagelen; i++)
	switch (p->page[i]) {
	case bcfID:
		++i;
		if ((i < p->pagelen) && (p->page[i] == bcfID))
		    putchar(bcfID);
		break;
	case bctID:
		++i;
		if ((i < p->pagelen) && (p->page[i] == bctID))
		    putchar(bctID);
		else
		    while (i < p->pagelen && p->page[i] != bctID)
			++i;
		break;
	case DLE:
		++i;
		if (i < p->pagelen) {
		    if (p->page[i] == DLE)
			putchar(DLE);
		    else {
			int indent = ((unsigned char)p->page[i])-' ';

			if (indent > 0 && indent <= 96)
			    printf("%*.*s", indent, indent, "");
		    }
		}
		break;
	default:
		putchar(p->page[i]);
		break;
	}
    fflush(stdout);
} /* display */


void
main(int argc, char **argv)
{
    Page *p;
    int opt;
    int raw=0;
    int i;

    opterr = 1;
    while ((opt = getopt(argc, argv, "r")) != EOF)
	if (opt == 'r')
	    raw = 1;
	else {
	    fprintf(stderr, "usage: htmlhelp [-r] [file]\n");
	    exit(1);
	}

    if (argc > optind)
	if (freopen(argv[optind], "r", stdin) == 0) {
	    perror(argv[optind]);
	    exit(1);
	}

    p = render(stdin, 79);
    if (raw) {
	write(fileno(stdout), p->page, p->pagelen);

	for (i=0; i< p->nrhrefs; i++)
	    printf("%2d: %s\n", i, p->hrefs[i]);
    }
    else
	display(p);
    exit(0);
}
