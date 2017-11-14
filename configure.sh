#! /bin/sh

# local options:  ac_help is the help message that describes them
# and LOCAL_AC_OPTIONS is the script that interprets them.  LOCAL_AC_OPTIONS
# is a script that's processed with eval, so you need to be very careful to
# make certain that what you quote is what you want to quote.

ac_help='--with-bsd-curses	Use BSD curses if available.
--with-amalloc		Use a paranoid memory allocator
--with-getcap		(--with-bsd-curses) use getcap() to get
			function key definitions
--shared		Build shared libraries'

LOCAL_AC_OPTIONS='
set=`locals $*`;
if [ "$set" ]; then
    eval $set
    shift 1
else
    ac_error=T;
fi'

locals() {
    K=`echo $1 | $AC_UPPERCASE`
    case "$K" in
    --SHARED)
                echo TRY_SHARED=T
                ;;
    esac
}			


# load in the configuration file
#
TARGET=ndialog
. ./configure.inc

# and away we go
#
AC_INIT ndialog

AC_PROG_CC

test "$TRY_SHARED" && AC_COMPILER_PIC && AC_CC_SHLIBS

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

    if AC_LIBRARY start_color -lncurses -lcurses; then
	WITH_NCURSES=1
	return 0
    elif AC_LIBRARY initscr -lcurses "-lcurses -ltermcap"; then
	WITH_BSD_CURSES=1
	return 1	# have a curses lib, but it's not ncurses.
    fi
    $__fail 1
}


check_curses() {
    if AC_CHECK_HEADERS curses.h; then
	AC_SUB CURSES_HEADER curses.h
	# possibly.  
	if AC_LIBRARY initscr -lcurses '-lcurses -ltermcap'; then
	    WITH_BSD_CURSES=1
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
    if [ "$check_panel" ] && AC_LIBRARY new_panel -lpanel; then
	HAVE_PANEL=1
	AC_SUB PANEL_HEADER $panel_header
    fi
fi

test "$WITH_GETCAP" && AC_DEFINE WITH_GETCAP $WITH_GETCAP
test "$WITH_NCURSES" && AC_DEFINE WITH_NCURSES $WITH_NCURSES
test "$WITH_BSD_CURSES" && AC_DEFINE WITH_BSD_CURSES $WITH_BSD_CURSES
test $HAVE_PANEL && AC_DEFINE HAVE_PANEL $HAVE_PANEL

if [ ! "$WITH_BSD_CURSES" ]; then
    # check for particular ncurses functions so we can dummy them
    # if need be.
    AC_CHECK_FUNCS wattr_set
    AC_CHECK_FUNCS waddnstr
    AC_CHECK_FUNCS beep
    AC_CHECK_FUNCS curs_set
    AC_CHECK_FUNCS ripoffline
    AC_CHECK_FUNCS start_color
fi
AC_CHECK_FUNCS doupdate
AC_CHECK_FUNCS keypad
AC_CHECK_FUNCS getmouse

if [ "$WITH_AMALLOC" ]; then
    AC_SUB AMALLOC amalloc.o
    AC_INCLUDE 'amalloc.h'
else
    AC_SUB AMALLOC ''
    AC_DEFINE 'adump()' '1'
fi

AC_CHECK_HEADERS errno.h
MF_PATH_INCLUDE RANLIB ranlib true || AC_CONFIG RANLIB ':'

AC_OUTPUT Makefile src/Makefile src/curse.h dialog/Makefile

