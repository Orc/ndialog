#ifndef __OPTIONS_D
#define __OPTIONS_D

#include <stdio.h>

struct x_option {
    int  optval;	/* option value */
    char flag;		/* single character flag or null */
    char *name;		/* option name or null */
    char *has_argument;	/* does it need a following argument (and what that
			 * argument is, for help messages) 
			 */
    char *description;	/* what this options is for (for help messages) */
};

extern int x_optind;
extern int x_opterr;
extern char *x_optarg;

int x_getopt(int, char **, int, struct x_option *);

void showopts(FILE *, int, struct x_option *);

#endif/*__OPTIONS_D*/
