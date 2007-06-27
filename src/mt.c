#include <stdio.h>
#include "curse.h"

main()
{
#if VERMIN
    unsigned int c;
    MEVENT qqq;
    WINDOW *bar;

    initscr();
    scrollok(stdscr, 1);
    mousemask(-1, (mmask_t*)0);

    bar = newwin(2,2,2,2);
    scrollok(bar, 1);

    raw();
    while ((c = wgetch(bar)) != 'q') {
	if (c == KEY_MOUSE) {
	    getmouse(&qqq);
	    printw("WE HAVE VERMIN: x=%d, y=%d, kcode=%04x\n", qqq.x, qqq.y, qqq.bstate);
	}
	else
	    printw("c is %04x\n", c);
	refresh();
    }
    endwin();
#else
    puts("VERMIN is not defined -- no mouse support that we know of");
#endif
}
