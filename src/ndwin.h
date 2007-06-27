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
 *   (orc@pell.chi.il.us)
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
 * ndwin: display functions (this should not be published)
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
