/*
 * test the Obj Class
 */
#include <assert.h>
#include <stdio.h>

#include "nd_objects.h"

void
printNextObjChain(void *obj)
{
    Obj *p = OBJ(obj);

    if (p == 0)
	return;
    do {
	assert(p != 0);
	printf("(%d,%d) %s title [%s]\n", p->x, p->y, objId(p), objTitle(p));
	p = p->next;
    } while (p != OBJ(obj));
} /* printNextObjChain */


void
printPrevObjChain(void *obj)
{
    Obj *p = OBJ(obj);

    if (p == 0)
	return;
    do {
	assert(p != 0);
	printf("(%d,%d) %s title [%s]\n", p->x, p->y, objId(p), objTitle(p));
	p = p->prev;
    } while (p != OBJ(obj));
} /* printPrevObjChain */

void
main(int pointy, char **arguments)
{
    Obj *a, *c;
    Obj *t, *chain;

    chain = ObjChain(a = newButton(1,"A",0,0),
		     newButton(2,"B",0,0));
    chain = ObjChain(chain, c = newCheck(0,0,"C",0,0,0,0));
    chain = ObjChain(chain, newCheck(0,4,"D",0,0,0,0));

    t = ObjChain(chain, chain);

    if (t == (Obj*)0)
	perror("duplicate insert");
    else
	puts("ERROR! duplicate inserts work");

    puts("BACKWARD");
    printPrevObjChain(chain);

    puts("FORWARD");
    printNextObjChain(chain);

    t = copyObj(a);
    setObjTitle(t, "AA");

    chain = extractFromObjChain(chain, a);

    if (chain) {
	puts("FORWARD AFTER EXTRACT");
	printNextObjChain(chain);

	ObjChain(c, a);
	ObjChain(c, t);
	puts("FORWARD AFTER REINSERT");
	printNextObjChain(chain);

	chain = sortObjChain(chain);
	if (chain) {
	    puts("FORWARD AFTER SORT");
	    printNextObjChain(chain);
	}
	else
	    perror("sortObjChain");
    }
    else
	perror("extractFromObjChain");

    deleteObjChain(chain);
}
