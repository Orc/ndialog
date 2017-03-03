/*
 * ndialog: new dialog library
 *
 * Copyright (C) 1996-2017 David L Parsons.
 * The redistribution terms are provided in the COPYRIGHT file that must
 * be distributed with this source code.
 */
#include <config.h>

#include "curse.h"
#include <ndialog.h>
#include <string.h>

#define NR_ND_COLORS	10
int nd_colors[NR_ND_COLORS+1] = { 0, };
int nr_nd_colors = NR_ND_COLORS;

#if HAVE_PANEL
static PANEL *root;
#endif

#if HAVE_RIPOFFLINE
static WINDOW *helpline;	/* a window for help information */
static int    helpcols;		/* # of columns in the help window */
static char   *helptext = 0;	/* the help information */
#endif

#if HAVE_RIPOFFLINE
/*
 * init_ripoff() initializes a ripoffline
 */
static int
init_ripoff(WINDOW *w, int i)
{
    helpline = w;
    helpcols = i;
    return 0;
} /* init_ripoff */
#endif


/*
 * init_dialog() starts curses and tweaks the console to our liking
 */
void
init_dialog()
{
    WINDOW *topwin;

#if HAVE_RIPOFFLINE
    ripoffline(-1, init_ripoff);
#endif

    if ((topwin = initscr()) == (WINDOW*)0)
	return;
    savetty();
    noecho();
    nonl();
    raw();

#if HAVE_START_COLOR
    start_color();
    if (has_colors()) {
#ifdef PEACOCK
	init_pair(1, COLOR_BLACK, COLOR_CYAN);	
	nd_colors[c__WINDOW] = COLOR_PAIR(1);		/* window */
	nd_colors[c__BG] = COLOR_PAIR(1);		/* background */
	init_pair(2, COLOR_CYAN, COLOR_BLACK);
	nd_colors[c__BUTTON] = COLOR_PAIR(2);		/* button */
	init_pair(3, COLOR_BLACK, COLOR_WHITE);
	nd_colors[c__PRESSED] = COLOR_PAIR(3);		/* selected button */
	nd_colors[c__SELECTED] = COLOR_PAIR(3);		/* selected item */
	init_pair(4, COLOR_YELLOW, COLOR_CYAN);
	nd_colors[c__WIDGET] = COLOR_PAIR(4);		/* widget */
	init_pair(5, COLOR_WHITE, COLOR_CYAN);
	nd_colors[c__TITLE] = COLOR_PAIR(5);		/* title */
	nd_colors[c__ACTIVE] = COLOR_PAIR(5);
	nd_colors[c__RELIEF] = COLOR_PAIR(5)|A_BOLD;	/* cheesy 3d effect */
	init_pair(6, COLOR_RED, COLOR_CYAN);
	nd_colors[c__HOTKEY] = COLOR_PAIR(6);		/* hot key */
	nd_colors[c__ERROR] = COLOR_PAIR(6)|A_BOLD;	/* error messages */
#else
	init_pair(1, COLOR_CYAN, COLOR_BLACK);
	nd_colors[c__WINDOW] = COLOR_PAIR(1);
	nd_colors[c__BG] = COLOR_PAIR(1);
	init_pair(2, COLOR_RED, COLOR_BLACK);
	nd_colors[c__SELECTED] = COLOR_PAIR(1)|A_REVERSE;
	nd_colors[c__BUTTON] = COLOR_PAIR(2);
	init_pair(3, COLOR_WHITE, COLOR_BLACK);
	nd_colors[c__PRESSED] = COLOR_PAIR(3);
	init_pair(4, COLOR_YELLOW, COLOR_BLACK);
	nd_colors[c__WIDGET] = COLOR_PAIR(4);
	init_pair(5, COLOR_WHITE, COLOR_BLACK);
	nd_colors[c__TITLE] = COLOR_PAIR(5);
	nd_colors[c__ACTIVE] = COLOR_PAIR(5);
	nd_colors[c__RELIEF] = COLOR_PAIR(5)|A_BOLD;
	init_pair(6, COLOR_RED, COLOR_BLACK);
	nd_colors[c__HOTKEY] = COLOR_PAIR(6);
	nd_colors[c__ERROR] = COLOR_PAIR(6)|A_BOLD;
#endif
    }
    else {
	nd_colors[c__WINDOW] = nd_colors[c__BUTTON] =
	nd_colors[c__WIDGET] = nd_colors[c__TITLE] =
	nd_colors[c__BG] = nd_colors[c__RELIEF] = 0;
	nd_colors[c__ACTIVE] = A_BOLD;
	nd_colors[c__HOTKEY] = nd_colors[c__SELECTED] =
	nd_colors[c__PRESSED] = A_STANDOUT;
    }
#else /* !HAVE_START_COLOR */
    nd_colors[c__ACTIVE] =
    nd_colors[c__WINDOW] = nd_colors[c__BUTTON] =
    nd_colors[c__WIDGET] = nd_colors[c__TITLE] =
    nd_colors[c__BG] = nd_colors[c__RELIEF] = 0;
    nd_colors[c__SELECTED] = nd_colors[c__HOTKEY] = 
    nd_colors[c__PRESSED] = CURRENT_COLOR;
#endif

#if HAVE_PANEL
    root = new_panel(topwin);
#endif
#if WITH_NCURSES
    wbkgdset(topwin, COLOR_PAIR(1));
    werase(topwin);
    wnoutrefresh(topwin);
#if HAVE_RIPOFFLINE
    wbkgdset(helpline, COLOR_PAIR(1));
    werase(helpline);
    wnoutrefresh(helpline);
#endif
#else
    werase(topwin);
    wrefresh(topwin);
#endif
} /* init_dialog */


/*
 * end_dialog() shuts off ncurses
 */
void
end_dialog()
{
#if HAVE_PANEL
    del_panel(root);
#endif
    mvcur(0,0,COLS-1,0);
    echo();
    nl();
    noraw();
    resetty();
    endwin();
} /* end_dialog */


/*
 * get_helpline() returns a pointer to the current helpline
 */
char *
get_helpline()
{
#if HAVE_RIPOFFLINE
    return helptext;
#else
    return 0;
#endif
} /* get_helpline */


/*
 * use_helpline() sets the helpline
 */
void
use_helpline(char *helpmemrspock)
{
#if HAVE_RIPOFFLINE
    int helplen = strlen(helpmemrspock);

    helptext = helpmemrspock;
    wclear(helpline);

    if (helplen < helpcols)
	wmove(helpline,0, (helpcols-helplen)/2);
    waddnstr(helpline, helpmemrspock, helpcols-1);
    wrefresh(helpline);
#endif
} /* use_helpline */


/*
 * restore_helpline() appears to be use_helpline() under a different name
 */
void
restore_helpline(char *helpmeplease)
{
    use_helpline(helpmeplease);
} /* restore_helpline */
