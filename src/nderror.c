/*
 * Copyright (C) 1996-2017 David L Parsons.
 * The redistribution terms are provided in the COPYRIGHT file that must
 * be distributed with this source code.
 */
#include <config.h>

#include <ndialog.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#if HAVE_ERRNO_H
#include <errno.h>
#endif

/*
 * Error() spits a system error message up on the screen, (q.v. perror(3))
 * but allows a printf-style argument list and separates the errno from
 * the message with a newline.
 */
void
Error(char *fmt, ...)
{
    void *chain;
    char *bfr;
    int bfrsize;
    va_list ptr;

    bfr = alloca(bfrsize = (strlen(fmt) + 1000) );

    va_start(ptr, fmt);
    vsnprintf(bfr, bfrsize-100, fmt, ptr);
    va_end(ptr);

    if (errno != 0) {
	strcat(bfr, "\n");
	strncat(bfr, strerror(errno), 99);
    }

    chain = ObjChain(0, newOKButton(0, "OK", 0, 0));

    MENU(chain, -1, -1, "Error", bfr, ERROR_FLAG);

    deleteObjChain(chain);
} /* Error */
