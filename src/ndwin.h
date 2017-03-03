/*
 * ndwin: display functions (this should not be published)
 *
 * Copyright (C) 1996-2017 David L Parsons.
 * The redistribution terms are provided in the COPYRIGHT file that must
 * be distributed with this source code.
 */
#ifndef NDWIN_D
#define NDWIN_D

#include "curse.h"
#include "nd_objects.h"

void drawButton(void*, void*);
void drawCheck(void*, void*);
void drawString(void*, void*);
void drawMenu(void*, void*);
void drawList(void*, void*);
void drawText(void*, void*);
void drawGauge(void*, void*);
#if !DYNAMIC_BINDING
void drawListWidget(void*, void*);
void drawFileSelector(void*, void*);
#endif

void fancywin(WINDOW*, int, int, char*, int, int);
void simplewin(WINDOW*, int, int, char *, int, int);
void drawbox(WINDOW*, int, int, int, int, int, int, int);

editCode editButton(void*, void*, MEVENT*, editCode);
editCode editString(void*, void*, MEVENT*, editCode);
editCode editMenu(void*, void*, MEVENT*, editCode);
editCode editCheck(void*, void*, MEVENT*, editCode);
editCode editList(void*, void*, MEVENT*, editCode);
editCode editText(void*, void*, MEVENT*, editCode);
#if !DYNAMIC_BINDING
editCode editListWidget(void*, void*, MEVENT*, editCode);
editCode editFileSelector(void*, void*, MEVENT*, editCode);
#endif

#define DREW_TITLE	0x01
#define DREW_PREFIX	0x02
#define DREW_SUFFIX	0x04
#define DREW_A_BOX	0x08

extern void _nd_adjustXY(int, void*, int*, int*);
extern int _nd_drawObjCommon(void*, void*);
extern void _nd_help(char*);
extern int _nd_inside(Obj*, MEVENT*);

#define ADJUSTXY(o,xp,yp)	(xp += o->x, yp += o->y)

#endif/*NDWIN_D*/
