/*
 * debugging malloc()/realloc()/calloc()/free() that attempts
 * to keep track of just what's been allocated today.
 *
 * Copyright (C) 1996-2017 David L Parsons.
 * The redistribution terms are provided in the COPYRIGHT file that must
 * be distributed with this source code.
 */
#ifndef AMALLOC_D
#define AMALLOC_D

#define USE_AMALLOC

#include <stdlib.h>

extern void *amalloc(int);
extern void *acalloc(int,int);
extern void *arealloc(void*,int);
extern void afree(void*);
extern void adump();

#define malloc	amalloc
#define	calloc	acalloc
#define realloc	arealloc
#define free	afree

#endif/*AMALLOC_D*/
