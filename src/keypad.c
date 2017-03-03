/*
 * keypad() for ndialog on curses that don't have arrow and/or function
 * key support.
 *
 * Copyright (C) 1996-2017 David L Parsons.
 * The redistribution terms are provided in the COPYRIGHT file that must
 * be distributed with this source code.
 */
#include <config.h>

#include "curse.h"

#if HAVE_KEYPAD

int
ndwgetch(WINDOW *w)
{
#if !HAVE_PANEL
    ndredraw();
#endif
    return wgetch(w);
}

#else

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <malloc.h>

static struct {
    char key[2];
    char **cap;
    int name;
} keywewant[] = {
    { "k0", &K0, KEY_F0 },
    { "k1", &K1, KEY_F1 },
    { "k2", &K2, KEY_F2 },
    { "k3", &K3, KEY_F3 },
    { "k4", &K4, KEY_F4 },
    { "k5", &K5, KEY_F5 },
    { "k6", &K6, KEY_F6 },
    { "k7", &K7, KEY_F7 },
    { "k8", &K8, KEY_F8 },
    { "k9", &K9, KEY_F9 },
    { "kd", &KD, KEY_DOWN },
    { "kl", &KL, KEY_LEFT },
    { "kr", &KR, KEY_RIGHT },
    { "ku", &KU, KEY_UP },
#if WITH_GETCAP	// getcap() confuses the hell out of bsd curses; don't use it!
    { "kN", 0,   KEY_NPAGE },
    { "kP", 0,   KEY_PPAGE },
#endif
};
#define NR_K	(sizeof keywewant/sizeof keywewant[0])

struct keymac {
    enum { FIN=0, TERM=1, MORE=2 } term;
    char c;
    union {
	struct keymac *next;
	int key;
    } u;
} ;

static int    initialized = 0;
static struct keymac *function_keys = 0;
static int    keylen_max = 0;
static int    keylen_min = 9999;
#define LONGBITS	(8*sizeof(long))
static long   key_hash[256/LONGBITS];

/*
 * addmac() adds a function key to the macros table
 */
static struct keymac *
addmac(int d, struct keymac *p, char *v, int key)
{
    struct keymac *r;
    int sz = 0;
    unsigned char c;

    if (v == 0 || *v == 0) return p;

    if (p) {
	for (r = p; r->term; r++) {
	    if (r->c == *v) {
		if (r->term == MORE)
		    r->u.next = addmac(1+d, r->u.next, 1+v, key);
		return p;
	    }
	    sz++;
	}
    }
    else {
	r = p = calloc(3, sizeof p[0]);
	sz = 0;
    }
    
    if (d == 1) {
	c = *v;
	key_hash[c/LONGBITS] |= 1<<(c%LONGBITS);
    }

    r->c = *v;
    if (v[1]) {
	r->term = MORE;
	r->u.next = addmac(1+d, 0, 1+v, key);
    }
    else {
	r->term = TERM;
	r->u.key = key;
	if (d < keylen_min) keylen_min = d;
	if (d > keylen_max) keylen_max = d;
    }
    ++r;
    r->term = 0;
    return realloc(p, (sz+3)*sizeof p[0]);
}


int
keypad(void * win, int on)
{
    int i;

    if (initialized || !on) return 1;
    initialized = 1;

    setbuf(stdin,0); fflush(stdin);
    if (KS) { fputs(KS,stdout); fflush(stdout); }

    for (i=0; i < NR_K; i++) {
	function_keys = addmac(1,function_keys,
				 keywewant[i].cap ? *(keywewant[i].cap)
						  : getcap(keywewant[i].key),
				 keywewant[i].name);
    }
    return 1;
}


int
ndwgetch(WINDOW *w)
{
    int c, ct, rc, c2;
    fd_set fds;
    struct timeval tick;
    struct keymac *p;

#if !HAVE_PANEL
    ndredraw();
#endif
again:
    c = wgetch(w);

    if ( key_hash[c/(8*sizeof key_hash[0])] & 1<<(c%(8*sizeof key_hash[0])) ) {
	/* possible function key */

	ct = 1;
	for (p = function_keys; p->term && p->c != c; p++)
	    ;
	switch (p->term) {
	case FIN:	goto again;
	case TERM:	return p->u.key;
	default:	tick.tv_sec = 0;
			tick.tv_usec = 50000;
			break;
	}
	FD_ZERO(&fds);
	while (1) {
	    FD_SET(0, &fds);

	    rc = select(1, &fds, 0, 0, &tick);

	    if (rc == 0 || !FD_ISSET(0, &fds))
		break;

	    ct++;
	    c2 = wgetch(w);

	    for (p = p->u.next; p->term && p->c != c2; p++)
		;
	    switch (p->term) {
	    case FIN:	goto again;
	    case TERM:	return p->u.key;
	    default:	tick.tv_sec = 0;
			tick.tv_usec = 25000;
			break;
	    }
	}
    }
    return c;
}
#endif
