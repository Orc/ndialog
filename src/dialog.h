/*
 * libdialog() compatability functions.
 *
 * Copyright (C) 1996-2017 David L Parsons.
 * The redistribution terms are provided in the COPYRIGHT file that must
 * be distributed with this source code.
 */
#ifndef DIALOG_COMPAT
#define DIALOG_COMPAT

#ifndef DIALOG_CHAR
#define DIALOG_CHAR char
#endif

void draw_box(void *win, int y, int x, int height, int width,
                                int box, int border);
int line_edit(void* dialog, int y, int x, int flen, int width,
                                int attrs, int first, char *result);

int strwidth(const char *str);				/* from ndialog */
int strdepth(const char *str);				/* from ndialog */
#define	strheight	strdepth

void dialog_create_rc(char *filename);

int dialog_yesno(char *title, char *prompt,
				    int height, int width);
							/* IMPLEMENTED */
#define dialog_yesno_extended ndialog_yesno

int dialog_prgbox(char *title, const char *line,
				    int height, int width,
				    int pause, int use_shell);

int dialog_msgbox(char *title, char *prompt,
				    int height, int width,
				    int pause);		/* IMPLEMENTED */

int dialog_textbox(char *title, char *file,
				    int height, int width);
int dialog_menu(char *title, char *prompt,
				    int height, int width,
				    int menu_height, int item_no,
				    char **items,
				    char *result,
				    int *ch, int *sc);	/* IMPLEMENTED */
int dialog_checklist(char *title, char *prompt,
				    int height, int width,
				    int nritems, int item_no,
				    char **items,
				    char *result);
							/* IMPLEMENTED */

int dialog_radiolist(char *title, char *prompt,
				    int height, int width,
				    int menu_height, int item_no,
				    char **items,
				    char *result);
							/* IMPLEMENTED */

int dialog_inputbox(char *title, char *prompt,
                                        int height, int width,
					char *result);
							/* IMPLEMENTED */

void dialog_clear_norefresh(void);			/* IMPLEMENTED */
void dialog_clear(void);				/* IMPLEMENTED */
void dialog_update(void);				/* IMPLEMENTED */
void init_dialog(void);					/* IMPLEMENTED */
void end_dialog(void);					/* IMPLEMENTED */

char *dialog_fselect(char *dir, char *fmask);
int  dialog_dselect(char *dir, char *fmask);
void dialog_notify(char *msg);				/* IMPLEMENTED */
int  dialog_mesgbox(char *title, char *prompt,
                                          int height, int width);
							/* IMPLEMENTED */
void use_helpfile(char *helpfile);			/* IMPLEMENTED */
void use_helpline(char *helpline);			/* IMPLEMENTED */
char *get_helpline(void);				/* IMPLEMENTED */
void restore_helpline(char *helpline);			/* IMPLEMENTED */
void dialog_gauge(char *title, char *prompt, int y, int x,
		  int height, int width, int perc);	/* IMPLEMENTED */

#endif/*DIALOG_COMPAT*/
