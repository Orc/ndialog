/*
 *   Copyright (c) 1996,2001 David Parsons. All rights reserved.
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
 *  3. My name may not be used to endorse or promote products derived
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

static const char rcsid[] = "$Id: s.options.c 1.8 00/02/01 22:42:44-00:00 orc $";

/*
 * x_getopt() another version of long argument lists
 */
#include <stdio.h>
#include <string.h>
#include "options.h"

int x_optind = 1;
int x_opterr = 0;
char *x_optarg;

int
x_getopt(int argc, char **argv, int optcount, struct x_option *opts)
{
    static char* shortoptarg=0;	/* currently parsing a short options arg? */
    static int donewithargs;	/* found -- or arg starting w/o - */
    static int x_argc = 0;	/* arglist currently being processed */
    static char **x_argv = 0;

    int x;
    char *longopts;

    if (x_argv != argv) {
	x_optind = 1;
	x_argv = argv;
	x_argc = argc;
	shortoptarg = 0;
	donewithargs = 0;
    }

#if DEBUG
    printf("\nshortoptarg:  %s\n", shortoptarg ? shortoptarg : "{null}");
    printf("x_optind:     %d\n", x_optind);
    printf("x_argc:       %d\n", x_argc);
    printf("donewithargs: %d\n", donewithargs);
#endif

    if (shortoptarg) {
	char flag = *(shortoptarg++);

	if (*shortoptarg == 0)
	    shortoptarg = 0;

	for (x=0; x<optcount; x++)
	    if (opts[x].flag && (opts[x].flag == flag)) {
		if (opts[x].has_argument) {
		    if (shortoptarg) {
			x_optarg = shortoptarg;
			shortoptarg = 0;
		    }
		    else if (x_optind < x_argc)
			x_optarg = x_argv[x_optind++];
		    else {
			if (x_opterr)
			    fprintf(stderr,
				    "%s: option `-%c' requires an argument\n",
				    x_argv[0], flag);
			return 0;
		    }
		}
		return opts[x].optval;
	    }
	if (x_opterr)
	    fprintf(stderr, "%s: unknown option -- %c\n", x_argv[0], flag);
	return 0;
    } /* if shortoptarg */

    if (x_optind >= x_argc || donewithargs)
	return EOF;

    /* we only get here if we're just starting a new argument */
    if (x_argv[x_optind][0] != '-') {
	donewithargs = 1;
	return EOF;
    }

    if (x_argv[x_optind][1] != '-') {
	shortoptarg = &x_argv[x_optind++][1];
	return x_getopt(argc, argv, optcount, opts);
    }

    /* not a new short opts */
    longopts = &x_argv[x_optind++][2];

    /* solitary -- means that there are no more options */
    if (*longopts == 0) {
	donewithargs=1;
	return EOF;
    }

    for (x=0; x<optcount; x++)
	if (opts[x].name && strcmp(opts[x].name, longopts) == 0) {
	    if (opts[x].has_argument) {
		if (x_optind < x_argc)
		    x_optarg = x_argv[x_optind++];
		else {
		    if (x_opterr)
			fprintf(stderr,
				"%s: option `--%s' requires an argument\n",
				x_argv[0], longopts);
		    return 0;
		}
	    }
	    return opts[x].optval;
	}

    /* it certainly can't be an argument at this point */
    if (x_opterr)
	fprintf(stderr, "%s: unknown option `--%s'\n", x_argv[0], longopts); 
    return 0;
} /* x_getopt */


/* 
 * showopts() -- dump a summary of options
 */
void
showopts(FILE *dest, int optcount, struct x_option *opts)
{
    int n;
    char *p;
    int optindent;
    int argindent;
    int optsize;
    int argsize;

    optindent = argindent = 0;
    for (n = 0; n < optcount; n++) {
	if (opts[n].optval == 0)
	    continue;
	if (opts[n].name && (strlen(opts[n].name) > optindent))
	    optindent = strlen(opts[n].name);
	if (opts[n].has_argument && (strlen(opts[n].has_argument) > argindent))
	    argindent = strlen(opts[n].has_argument);
    }
    argindent += 2;	/* leading space */
    optindent += 2;	/* leading -- */


    for (n = 0; n < optcount; n++) {
	if (opts[n].optval == 0)
	    continue;
	argsize = optsize = 0;
	if (opts[n].flag)
	    fprintf(dest, " -%c", opts[n].flag);
	else
	    fprintf(dest, "   ");
	if (opts[n].name) {
	    fprintf (dest, opts[n].flag  ? ", " : "  ");
	    fprintf (dest, "--%s", opts[n].name);
	    optsize = 2+strlen(opts[n].name);
	}
	else
	    fprintf (dest, "  ");

	if (opts[n].has_argument) {
	    fprintf(dest, " %s", opts[n].has_argument);
	    argsize = 1+strlen(opts[n].has_argument);
	}
	fprintf(dest, "%*s", (argindent+optindent)-(argsize+optsize), "");
	for (p=opts[n].description; *p; p++) {
	    fputc(*p, dest);
	    if (*p == '\n' && p[1])
		fprintf(dest, "     %*s", (argindent+optindent), "");
	}
	if (p[-1] != '\n')
	    fputc('\n', dest);
    }
} /* showopts */

#ifdef TEST_THIS_OUT
/*char *arglist[] = {"ARG 0", "-cdexf", "foo", "--file", "bar", "--create", "--delete", "--help", 0 };*/
char *arglist[] = {"ARG 0", "-f", "-", 0 };

struct x_option options[] = {
    { 1, 'c', "create", 0, "CREATE SOMETHING" },
    { 2, 'd', "delete", 0, "DELETE SOMETHING" },
    { 3, 'e', NULL,     0, "SHORT OPTION ONLY" },
    { 4, 'f', "file",   "ARGUMENT", "OPTION WITH ARGUMENT" },
    { 5, 0  , "help",   0, "LONG OPTIONS ONLY" }
};

main()
{
    int opt;

    x_opterr = 1;

    while ((opt=x_getopt(3, arglist, 5, options)) != EOF)
	if (opt > 0 && opt < 5) {
	    printf("option %d: %s\n", opt, options[opt-1].description);
	}
	else
	    printf("BAD BAD BAD\n");
    puts("EOF");
    printf ("optind = %d, argc = 3\n", x_optind);
}
#endif
