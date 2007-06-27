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
 * libdialog() compatability functions.
 */
#ifndef DIALOG_COMPAT
#define DIALOG_COMPAT

#ifndef DIALOG_CHAR
#define DIALOG_CHAR unsigned char
#endif

void draw_box(void *win, int y, int x, int height, int width,
                                int box, int border);
int line_edit(void* dialog, int y, int x, int flen, int width,
                                int attrs, int first, DIALOG_CHAR *result);

int strwidth(const char *str);				/* from ndialog */
int strdepth(const char *str);				/* from ndialog */
#define	strheight	strdepth

void dialog_create_rc(DIALOG_CHAR *filename);

int dialog_yesno(DIALOG_CHAR *title, DIALOG_CHAR *prompt,
				    int height, int width);
							/* IMPLEMENTED */
#define dialog_yesno_extended ndialog_yesno

int dialog_prgbox(DIALOG_CHAR *title, const DIALOG_CHAR *line,
				    int height, int width,
				    int pause, int use_shell);

int dialog_msgbox(DIALOG_CHAR *title, DIALOG_CHAR *prompt,
				    int height, int width,
				    int pause);		/* IMPLEMENTED */

int dialog_textbox(DIALOG_CHAR *title, DIALOG_CHAR *file,
				    int height, int width);
int dialog_menu(DIALOG_CHAR *title, DIALOG_CHAR *prompt,
				    int height, int width,
				    int menu_height, int item_no,
				    DIALOG_CHAR **items,
				    DIALOG_CHAR *result,
				    int *ch, int *sc);	/* IMPLEMENTED */
int dialog_checklist(DIALOG_CHAR *title, DIALOG_CHAR *prompt,
				    int height, int width,
				    int nritems, int item_no,
				    DIALOG_CHAR **items,
				    DIALOG_CHAR *result);
							/* IMPLEMENTED */

int dialog_radiolist(DIALOG_CHAR *title, DIALOG_CHAR *prompt,
				    int height, int width,
				    int menu_height, int item_no,
				    DIALOG_CHAR **items,
				    DIALOG_CHAR *result);
							/* IMPLEMENTED */

int dialog_inputbox(DIALOG_CHAR *title, DIALOG_CHAR *prompt,
                                        int height, int width,
					DIALOG_CHAR *result);
							/* IMPLEMENTED */

void dialog_clear_norefresh(void);			/* IMPLEMENTED */
void dialog_clear(void);				/* IMPLEMENTED */
void dialog_update(void);				/* IMPLEMENTED */
void init_dialog(void);					/* IMPLEMENTED */
void end_dialog(void);					/* IMPLEMENTED */

char *dialog_fselect(char *dir, char *fmask);
int  dialog_dselect(char *dir, char *fmask);
void dialog_notify(char *msg);				/* IMPLEMENTED */
int  dialog_mesgbox(DIALOG_CHAR *title, DIALOG_CHAR *prompt,
                                          int height, int width);
							/* IMPLEMENTED */
void use_helpfile(char *helpfile);			/* IMPLEMENTED */
void use_helpline(char *helpline);			/* IMPLEMENTED */
char *get_helpline(void);				/* IMPLEMENTED */
void restore_helpline(char *helpline);			/* IMPLEMENTED */
void dialog_gauge(char *title, char *prompt, int y, int x,
		  int height, int width, int perc);	/* IMPLEMENTED */

#endif/*DIALOG_COMPAT*/
