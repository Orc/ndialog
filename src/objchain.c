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
 * objchain(): routines that manage object chains.
 */
#include <config.h>

#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>

#include "nd_objects.h"
#include "ndwin.h"

/*-----------------------------------------------*
 *                                               *
 *  O B J E C T   C H A I N   F U N C T I O N S  *
 *                                               *
 *-----------------------------------------------*/

/*
 * ObjChain() adds an object to an object chain.
 */
void *
ObjChain(void *chain, void *item)
{
    Obj *tail;

    if (OBJ(item)->next || OBJ(item)->prev) {
	/* can't simultaneously insert an object into two object chains */
	errno = EINVAL;
	return 0;
    }
    if (OBJ(chain) == 0) {
	/* chain is null; make item into a chain head */
	OBJ(item)->prev = OBJ(item)->next = OBJ(item);
	return item;
    }
    if (OBJ(chain)->prev == (Obj*)0 || OBJ(chain)->next == (Obj*)0) {
	if (OBJ(chain)->next || OBJ(chain)->prev) {
	    errno = EFAULT;
	    return 0;
	}
	/* tie the object and the chain together, returning the chain */
	OBJ(chain)->prev = OBJ(chain)->next = item;
	OBJ(item)->next = OBJ(item)->prev = chain;

	return chain;
    }

    tail = OBJ(chain)->prev;

    OBJ(item)->next = OBJ(chain);
    OBJ(item)->prev = tail;
    tail->next = OBJ(item);
    OBJ(chain)->prev = OBJ(item);
    return chain;
} /* ObjChain */


/*
 * extract FromObjChain() pulls an object out of an object chain.
 *
 * returns a pointer to the head of the chain if the object can be
 * extracted, or 0 if it cannot (not in the chain or it's the only
 * object in the chain.)
 *
 * If it returns 0, it sets errno to:
 *	EINVAL: arguments bad.
 *	EFAULT: broken/corrup chain.
 *	ENOENT: can't find the object in the chain.
 *	EPERM:  the object is the only thing in the chain.
 */
void *
extractFromObjChain(void *chain, void *obj)
{
    Obj *cur;

    if (chain == 0 || obj == 0) {
	errno = EINVAL;	/* invalid argument */
	return 0;
    }

    cur = OBJ(chain);

    /* special case for when the object is the head of the chain
     */
    if (obj == chain) {
	cur = OBJ(chain)->next;
	if (cur == OBJ(obj) || cur == 0) {
	    errno = cur ? EPERM : EFAULT;
	    return 0;
	}

	cur->prev = OBJ(obj)->prev;
	cur->prev->next = cur;

	OBJ(obj)->next = OBJ(obj)->prev = 0;

	return cur;
    }

    /* the object may be somewhere inside the chain
     */
    do {
	if (cur == OBJ(obj))
	    break;
	cur = cur->next;
	if (cur == 0) {
	    errno = EFAULT;
	    return 0;
	}
    } while (cur != OBJ(chain));

    if (cur == OBJ(obj)) {
	if (cur->prev == 0) {
	    errno = EFAULT;
	    return 0;
	}
	cur->prev->next = cur->next;
	cur->next->prev = cur->prev;

	OBJ(obj)->next = OBJ(obj)->prev = 0;

	return chain;
    }

    errno = ENOENT;
    return 0;
} /* extractFromObjChain */


/*
 * deleteObjChain() wipes out an object chain and all the objects it contains
 */
void
deleteObjChain(void *o)
{
    Obj *top, *cur, *next;

    top = OBJ(o);

    if (top != (Obj*)0) {
	cur = top;
	do {
	    next = cur->next;
	    deleteObj(cur);
	    if (next == 0)
		break;
	    cur = next;
	} while (cur != top);
    }
} /* deleteObjChain */


/*-----------------------------------------------*
 *                                               *
 *           C H A I N   S O R T I N G           *
 *                                               *
 *-----------------------------------------------*/

/*
 * coreSortObjChain() sorts an object chain by some user-specified criteria,
 * returning a pointer to the start of the chain.
 *
 * We could do this in some elegant fashion, but that would make my
 * head hurt.  Instead, we build an array of Obj*'s, run qsort on
 * that array, reassemble the chain, and return a pointer to the new
 * head of the chain.
 */
void*
coreSortObjChain(void *chain, int (*sortf)(const void*, const void*))
{
    Obj **list;
    Obj *run;
    int idx;
    int count;

    if (chain == 0) {
	errno = EINVAL;
	return 0;
    }


    /* count the number of elements in the chain */
    count =0;
    run = OBJ(chain);
    do {
	++count;
	run = run->next;
	if (run == 0) {
	    errno = EFAULT;
	    return 0;
	}
    } while (run != OBJ(chain));

    /* build an array of Obj*'s */
    if ((list = (Obj**)malloc(count * sizeof list[0])) == (Obj**)0)
	return 0;

    for (run=OBJ(chain),idx=0; idx<count; idx++) {
	list[idx] = run;
	run = run->next;
    }

    /* sort the array */
    qsort(list, count, sizeof list[0], sortf);

    /* reassemble the chain */
    for (idx=0; idx<count; idx++) {
	list[idx]->next = (idx == count-1) ? list[0] : list[idx+1];
	list[idx]->prev = (idx == 0) ? list[count-1] : list[idx-1];
    }
    chain = list[0];
    free(list);
    return chain;
} /* coreSortObjChain */


/*
 * sortbyxytype() is a local function to help sort object chains by
 * x,y,and object type
 */
static int
sortbyxytype(const void* a, const void* b)
{
    Obj *ta = *(Obj**)a;
    Obj *tb = *(Obj**)b;

    if (ta->Class == O_BUTTON && tb->Class != O_BUTTON)
	return 1;
    if (ta->Class != O_BUTTON && tb->Class == O_BUTTON)
	return -1;

    if (ta->y == tb->y)
	return (ta->x) - (tb->x);

    return (ta->y) - (tb->y);
} /* sortbyxytype */


/*
 * sortObjChain() sorts an object chain by x,y,and object type
 */
void *
sortObjChain(void *chain)
{
    return coreSortObjChain(chain, sortbyxytype);
} /* sortObjChain */
