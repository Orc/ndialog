/*
 * Copyright (C) 2000-2017 David L Parsons.
 * The redistribution terms are provided in the COPYRIGHT file that must
 * be distributed with this source code.
 */
#ifndef __INDEXEDMENU_D
#define __INDEXEDMENU_D

#include <ndialog.h>

ndObject newIndexedMenu(int x, int y, int width, int depth, int indexwidth,
			LIA list, char* prompt, pfo callback, char* help);

int getIndexedMenuSelection(ndObject obj, int* ikey, int* mkey);
int setIndexedMenuSelection(ndObject obj, int ikey, int mkey);

#endif/*__INDEXEDMENU_D*/
