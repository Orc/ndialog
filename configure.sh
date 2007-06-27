#! /bin/sh

# local options:  ac_help is the help message that describes them
# and LOCAL_AC_OPTIONS is the script that interprets them.  LOCAL_AC_OPTIONS
# is a script that's processed with eval, so you need to be very careful to
# make certain that what you quote is what you want to quote.

ac_help='--with-bsd-curses	Use BSD curses if available.
--with-getcap		(--with-bsd-curses) use getcap() to get
			function key definitions'


# load in the configuration file
#
TARGET=ndialog
. ./configure.inc

# and away we go
#
AC_INIT ndialog

AC_PROG_CC

check_ncurses() {
    if AC_CHECK_HEADERS ncurses/curses.h; then
	AC_SUB CURSES_HEADER ncurses/curses.h
	# if the ncurses directory is there, we've probably got a winnah.
    elif AC_CHECK_HEADERS curses.h; then
	AC_SUB CURSES_HEADER curses.h
	# possibly.  
    else
	# no ncurses OR curses header.  Goodbye, cruel world.
	$__fail 1
    fi

    if LIBS="-lncurses" AC_CHECK_FUNCS keypad WITH_NCURSES; then
	AC_LIBS="$AC_LIBS -lncurses"
	return 0
    elif LIBS="-lcurses" AC_CHECK_FUNCS keypad WITH_NCURSES; then
	AC_LIBS="$AC_LIBS -lcurses"
	return 0
    elif LIBS="-lcurses" AC_CHECK_FUNCS initscr WITH_BSD_CURSES; then
	AC_LIBS="$AC_LIBS -lcurses"
	return 1	# have a curses lib, but it's not ncurses.
    elif LIBS="-lcurses -ltermcap" AC_CHECK_FUNCS initscr WITH_BSD_CURSES; then
	AC_LIBS="$AC_LIBS -lcurses -ltermcap"
	return 1	# have a curses lib, but it's not ncurses.
    fi
    $__fail 1
}


check_curses() {
    if AC_CHECK_HEADERS curses.h; then
	AC_SUB CURSES_HEADER curses.h
	# possibly.  
	if LIBS="-lcurses" AC_CHECK_FUNCS initscr WITH_BSD_CURSES; then
	    AC_LIBS="$AC_LIBS -lcurses"
	    return 0
	elif LIBS="-lcurses -ltermcap" AC_CHECK_FUNCS initscr WITH_BSD_CURSES; then
	    AC_LIBS="$AC_LIBS -lcurses -ltermcap"
	    return 0
	fi
    fi
    $__fail 1
}


if [ "${WITH_BSD_CURSES:-0}" -eq 1 ]; then
    check_curses
elif check_ncurses; then
    # if ncurses, then check for the panel library
    if AC_CHECK_HEADERS ncurses/panel.h; then
	panel_header=ncurses/panel.h
        check_panel=T
    elif AC_CHECK_HEADERS panel.h; then
	panel_header=panel.h
	check_panel=T
    fi
    if [ "$check_panel" ]; then
	if LIBS="-lpanel $AC_LIBS" AC_CHECK_FUNCS new_panel HAVE_PANEL; then
	    AC_LIBS="-lpanel $AC_LIBS"
	    AC_SUB PANEL_HEADER $panel_header
	elif LIBS="$AC_LIBS -lpanel" AC_CHECK_FUNCS new_panel HAVE_PANEL; then
	    AC_LIBS="$AC_LIBS -lpanel"
	    AC_SUB PANEL_HEADER $panel_header
	fi
    fi
fi
AC_CHECK_HEADERS errno.h
test "$WITH_GETCAP" && AC_DEFINE WITH_GETCAP $WITH_GETCAP

if [ ! "$WITH_BSD_CURSES" ]; then
    # check for particular ncurses functions so we can dummy them
    # if need be.
    LIBS="$AC_LIBS" AC_CHECK_FUNCS wattr_set
    LIBS="$AC_LIBS" AC_CHECK_FUNCS waddnstr
    LIBS="$AC_LIBS" AC_CHECK_FUNCS beep
    LIBS="$AC_LIBS" AC_CHECK_FUNCS curs_set
    LIBS="$AC_LIBS" AC_CHECK_FUNCS ripoffline
    LIBS="$AC_LIBS" AC_CHECK_FUNCS doupdate
    LIBS="$AC_LIBS" AC_CHECK_FUNCS keypad
    LIBS="$AC_LIBS" AC_CHECK_FUNCS start_color
fi

MF_PATH_INCLUDE RANLIB ranlib true || AC_CONFIG RANLIB ':'

AC_OUTPUT src/Makefile src/curse.h

