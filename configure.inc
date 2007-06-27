#
#   Copyright (c) 1999 David Parsons. All rights reserved.
#   
#   Redistribution and use in source and binary forms, with or without
#   modification, are permitted provided that the following conditions
#   are met:
#  1. Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in
#     the documentation and/or other materials provided with the
#     distribution.
#  3. My name may not be used to endorse or promote products derived
#     from this software without specific prior written permission.
#     
#  THIS SOFTWARE IS PROVIDED BY DAVID PARSONS ``AS IS'' AND ANY
#  EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
#  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
#  PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL DAVID
#  PARSONS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
#  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
#  TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
#  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
#  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
#  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
#  IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
#  THE POSSIBILITY OF SUCH DAMAGE.
#


#
# this preamble code is executed when this file is sourced and it picks
# interesting things off the command line.
#
ac_default_path="/sbin:/usr/sbin:/bin:/usr/bin:/usr/local/bin:/usr/X11R6/bin"

ac_standard="--src=DIR		where the source lives (.)
--prefix=DIR		where to install the final product (/usr/local)
--execdir=DIR		where to put executables (prefix/bin)
--libdir=DIR		where to put libraries (prefix/lib)
--mandir=DIR		where to put manpages (prefix/man)
--confdir=DIR		where to put configuration information (/etc)"

__fail=exit

ac_configure_command="$*"

while [ $# -gt 0 ]; do
    unset matched

    case X"$1" in
    X--src|X--srcdir)
	AC_SRCDIR=$2
	shift 2;;

    X--src=*|X--srcdir==*)
	AC_SRCDIR=`echo "$1" | sed -e 's/^[^=]*=//'`
	shift 1 ;;

    X--prefix)
	AC_PREFIX=$2
	shift 2;;

    X--prefix=*)
	AC_PREFIX=`echo "$1"| sed -e 's/^[^=]*=//'`
	shift 1;;

    X--confdir)
	AC_CONFDIR=$2
	shift 2;;

    X--confdir=*)
	AC_CONFDIR=`echo "$1" | sed -e 's/^[^=]*=//'`
	shift 1;;

    X--lib|X--libdir)
	AC_LIBDIR=$2
	shift 2;;

    X--lib=*|X--libdir=*)
	AC_LIBDIR=`echo "$1" | sed -e 's/^[^=]*=//'`
	shift 1;;

    X--exec|X--execdir)
	AC_EXECDIR=$2
	shift 2;;

    X--exec=*|X--execdir=*)
	AC_EXECDIR=`echo "$1" | sed -e 's/^[^=]*=//'`
	shift 1;;

    X--man|X--mandir)
	AC_MANDIR=$2
	shift 2;;

    X--man=*|X--mandir=*)
	AC_MANDIR=`echo "$1" | sed -e 's/^[^=]*=//'`
	shift 1;;

    X--with*=*)
	_var=`echo "$1"| sed -n 's/^--with-\([A-Za-z][-A-Za-z0-9_]*\)=.*$/\1/p'`
	if [ "$_var" ]; then
	    _val=`echo "$1" | sed -e 's/^--with-[^=]*=\(.*\)$/\1/'`
	    _v=`echo $_var | tr '[a-z]' '[A-Z]' | tr '-' '_'`
	    eval WITH_${_v}=$_val
	else
	    echo "Bad option $1.   Use --help to show options"
	    exit 1
	fi
	shift 1 ;;

    X--with*)
	_var=`echo "$1" | sed -n 's/^--with-\([A-Za-z][A-Za-z0-9_-]*\)$/\1/p'`
	if [ "$_var" ]; then
	    _v=`echo $_var | tr '[a-z]' '[A-Z]' | tr '-' '_'`
	    eval WITH_${_v}=1
	else
	    echo "Bad option $1.   Use --help to show options"
	    exit 1
	fi
	shift 1 ;;

    X--help)
	echo "$ac_standard"
	test "$ac_help" && echo "$ac_help"
	exit 0;;

    *)	if [ "$LOCAL_AC_OPTIONS" ]; then
	    eval $LOCAL_AC_OPTIONS
	else
	    ac_error=T
	fi
	if [ "$ac_error" ]; then
	    echo "Bad option $1.   Use --help to show options"
	    exit 1
	fi ;;
    esac
done


#
# log something to the terminal and to a logfile.
#
LOG () {
    echo "$@" | tee -a config.log
}

#
# log something to the terminal without a newline, and to a logfile with
# a newline
#
LOGN () {
    echo -n "$@"
    echo "$@" >> config.log
}

#
# log something to the terminal
#
TLOG () {
    echo "$@"
}

#
# log something to the terminal, no newline
#
TLOGN () {
    echo -n "$@"
}


#
# AC_CONTINUE tells configure not to bomb if something fails, but to
# continue blithely along
#
AC_CONTINUE () {
    __fail="return"
}

#
# Emulate gnu autoconf's AC_CHECK_HEADERS() function
#
AC_CHECK_HEADERS () {
    AC_PROG_CC

    cat > /tmp/ngc$$.c << EOF
#include <$1>

main()
{
}
EOF

    LOGN "checking for header $1"

    if $AC_CC -o /tmp/ngc$$ /tmp/ngc$$.c 2>>config.log; then
	echo '#define HAVE_'`echo $1 | tr 'a-z' 'A-Z' | tr './' '_'`' 1' >> config.h
	TLOG " (found)"
	rc=0
    else
	TLOG " (not found)"
	rc=1
    fi
    rm -f /tmp/ngc$$.c /tmp/ngc$$
    return $rc
}


#
# emulate GNU autoconf's AC_CHECK_FUNCS function
#
AC_CHECK_FUNCS () {
    AC_PROG_CC

    cat >> /tmp/ngc$$.c << EOF
main()
{
    $1();
}
EOF

    LOGN "checking for the $1 function"

    if $AC_CC -o /tmp/ngc$$ /tmp/ngc$$.c $LIBS 2>>config.log; then
	echo "#define `echo ${2:-HAVE_$1} | tr 'a-z' 'A-Z'` 1" >> config.h
	TLOG " (found)"
	rc=0
    else
	echo "offending command was:"                  >> config.log
	cat /tmp/ngc$$.c                               >> config.log
	echo "$AC_CC -o /tmp/ngc$$ /tmp/ngc$$.c $LIBS" >> config.log
	TLOG " (not found)"
	rc=1
    fi
    rm -f /tmp/ngc$$.c /tmp/ngc$$
    return $rc
}


#
# check that the C compiler works
#
AC_PROG_CC () {
    test "$AC_CC" && return 0

    cat > /tmp/ngc$$.c << \EOF
#include <stdio.h>
main()
{
    puts("hello, sailor");
}
EOF

    TLOGN "Checking the C compiler"

    AC_CC=${CC:-`acLookFor cc`}

    if [ ! "$AC_CC" ]; then
	echo " (no C compiler found)"
	$__fail 1
    fi
    echo "Checking out the C compiler" >> config.log

    $AC_CC -o /tmp/ngc$$ /tmp/ngc$$.c 2>> config.log
    status=$?

    TLOGN " ($AC_CC)"
    if [ $status -eq 0 ]; then
	TLOG " ok"

	# check that the CFLAGS and LDFLAGS aren't bogus

	unset AC_CFLAGS AC_LDFLAGS

	if [ "$CFLAGS" ]; then
	    test "$CFLAGS" && echo "validating CFLAGS=${CFLAGS}" >> config.log
	    if $AC_CC $CFLAGS -o /tmp/ngc$$.o /tmp/ngc$$.c 2>> config.log; then
		AC_CFLAGS=${CFLAGS:-"-g"}
		test "$CFLAGS" && echo "CFLAGS=\"${CFLAGS}\" are okay"
	    elif [ "$CFLAGS" ]; then
		echo "ignoring bogus CFLAGS=\"${CFLAGS}\""
	    fi
	else
	    AC_CFLAGS=-g
	fi
	if [ "$LDFLAGS" ]; then
	    test "$LDFLAGS" && echo "validating LDFLAGS=${LDFLAGS}" >>config.log
	    if $AC_CC $LDFLAGS -o /tmp/ngc$$ /tmp/ngc$$.o 2>>/config.log; then
		AC_LDFLAGS=${LDFLAGS:-"-g"}
		test "$LDFLAGS" && echo "LDFLAGS=\"${LDFLAGS}\" are okay"
	    elif [ "$LDFLAGS" ]; then
		echo "ignoring bogus LDFLAGS=\"${LDFLAGS}\""
	    fi
	else
	    AC_LDFLAGS=${CFLAGS:-"-g"}
	fi
	echo "s,@CFLAGS@,$AC_CFLAGS,g" >> config.sub
	echo "s,@LDFLAGS@,$AC_LDFLAGS,g" >> config.sub
    else
	echo "s,@CFLAGS@,,g" >> config.sub
	echo "s,@LDFLAGS@,,g" >> config.sub
	TLOG " does not compile code properly"
	$__fail 1
    fi

    echo "s,@CC@,$AC_CC,g" >> config.sub

    rm -f /tmp/ngc$$ /tmp/ngc$$.c /tmp/ngc$$.o

    return $status
}


#
# acLookFor actually looks for a program, without setting anything.
#
acLookFor () {
    path=${AC_PATH:-$ac_default_path}
    case "X$1" in
    X-[rx]) __mode=$1
	    shift
	    ;;
    *)	    __mode=-x
	    ;;
    esac
    oldifs="$IFS"
    for program in $*; do
	IFS=":"
	for x in $path; do
	    if [ $__mode $x/$program ]; then
		echo $x/$program
		break 2
	    fi
	done
    done
    IFS="$oldifs"
    unset __mode
}


#
# check that a program exists and set its path
#
MF_PATH_INCLUDE () {
    SYM=$1; shift

    case X$1 in
    X-[rx]) __mode=$1
	    shift
	    ;;
    *)      unset __mode
	    ;;
    esac

    TLOGN "looking for $1"

    DEST=`acLookFor $__mode $*`

    if [ "$DEST" ]; then
	TLOG " ($DEST)"
	echo "$1 is $DEST" >> config.log
	echo "HAVE_$SYM = 1" >> config.mak
	echo "#define PATH_$SYM \"$DEST\"" >> config.h
	echo "s,@`echo $SYM | tr 'a-z' 'A-Z'`@,$DEST,g" >> config.sub
	eval CF_$SYM=$DEST
	return 0
    else
	#echo "s,@`echo $SYM | tr 'a-z' 'A-Z'`@,,g" >> config.sub
	echo "$1 is not found" >> config.log
	TLOG " (not found)"
	return 1
    fi
}


#
# AC_INIT starts the ball rolling
#
AC_INIT () {
    AC_CONFIGURE_FOR=__AC_`echo $1 | sed -e 's/\..$//' | tr 'a-z' 'A-Z' | tr ' ' '_'`_D
    LOG "Configuring for [$1]"
    ac_os=`uname -s`
    cat > config.h << EOF
/*
 * configuration for $1${2:+" ($2)"}, generated `date`
 * by ${LOGNAME:-`whoami`}@`hostname`
 */
#ifndef $AC_CONFIGURE_FOR
#define $AC_CONFIGURE_FOR 1


EOF
    rm -f config.mak

AC_PREFIX=${AC_PREFIX:-/usr/local}
AC_EXECDIR=${AC_EXECDIR:-$AC_PREFIX/bin}
AC_LIBDIR=${AC_LIBDIR:-$AC_PREFIX/lib}
AC_MANDIR=${AC_MANDIR:-$AC_PREFIX/man}
AC_CONFDIR=${AC_CONFDIR:-/etc}
AC_SRCDIR=${AC_SRCDIR:-`pwd`}

    cat > config.sub << EOF
s,@srcdir@,$AC_SRCDIR,g
s,@prefix@,$AC_PREFIX,g
s,@exedir@,$AC_EXECDIR,g
s,@libdir@,$AC_LIBDIR,g
s,@confdir@,$AC_CONFDIR,g
s,@mandir@,$AC_MANDIR,g
EOF

    AC_PROG_CPP
    AC_PROG_INSTALL
}


#
# AC_PROG_YACC checks to see if YACC exists, and if it's bison or
# not.
#
AC_PROG_YACC () {

    TLOGN "looking for yacc "

    DEST=`acLookFor yacc`
    if [ "$DEST" ]; then
	echo "HAVE_YACC = 1" >> config.mak
	echo "#define PATH_YACC \"$DEST\"" >> config.h
	echo "s,@YACC@,$DEST,g" >> config.sub
	TLOG "($DEST)"
	echo "yacc is $DEST" >> config.log
    else
	DEST=`acLookFor bison`
	if [ "$DEST" ]; then
	    echo "HAVE_BISON = 1" >> config.mak
	    echo "#define PATH_YACC \"$DEST\"" >> config.h
	    echo "s,@YACC@,$DEST -y,g" >> config.sub
	    echo "yacc is $DEST -y" >> config.log
	    TLOG "($DEST -y)"
	else
	    echo "s,@YACC@,,g" >> config.sub
	    echo "neither yacc or bison found" >> config.log
	    TLOG " (not found)"
	    return 1
	fi
    fi
    return 0
}


#
# AC_PROG_LN_S checks to see if ln exists, and, if so, if ln -s works
#
AC_PROG_LN_S () {
    test "$AC_FIND_PROG" || AC_PROG_FIND

    test "$AC_FIND_PROG" || return 1
    
    TLOGN "looking for \"ln -s\""
    DEST=`acLookFor ln`

    if [ "$DEST" ]; then
	rm -f /tmp/b$$
	$DEST -s /tmp/a$$ /tmp/b$$
	if [ "`$AC_FIND_PROG /tmp/b$$ -type l -print`" ]; then
	    TLOG " ($DEST)"
	    echo "$DEST exists, and ln -s works" >> config.log
	    echo "s,@LN_S@,$DEST -s,g" >> config.sub
	    rm -f /tmp/b$$
	else
	    echo "s,@LN_S@,,g" >> config.sub
	    TLOG " ($DEST exists, but -s does not seem to work)"
	    echo "$DEST exists, but ln -s doesn't seem to work" >> config.log
	    rm -f /tmp/b$$
	    return 1
	fi
    else
	echo "s,@LN_S@,,g" >> config.sub
	echo "ln not found" >> config.log
	TLOG " (not found)"
	return 1
    fi
}


#
# AC_PROG_FIND looks for the find program and sets the FIND environment
# variable
#
AC_PROG_FIND () {
    if test -z "$AC_FIND_PROG"; then
	MF_PATH_INCLUDE FIND find
	rc=$?
	AC_FIND_PROG=$DEST
	return $rc
    fi
    return 0
}


#
# AC_PROG_SED looks for the sed program and sets the SED environment
# variable
#
AC_PROG_SED () {
    if test -z "$AC_SED_PROG"; then
	MF_PATH_INCLUDE SED sed
	rc=$?
	AC_SED_PROG=$DEST
	return $rc
    fi
    return 0
}


#
# AC_HEADER_SYS_WAIT looks for sys/wait.h
#
AC_HEADER_SYS_WAIT () {
    AC_CHECK_HEADERS sys/wait.h || return 1
}

#
# AC_TYPE_PID_T checks to see if the pid_t type exists
#
AC_TYPE_PID_T () {
    cat > /tmp/pd$$.c << EOF
#include <sys/types.h>
main() { pid_t me; }
EOF

    LOGN "checking for pid_t"

    if $AC_CC -c /tmp/pd$$.c -o /tmp/pd$$.o 2>> config.log; then
	TLOG " (found)"
	rc=0
    else
	echo "typedef int pid_t;" >> config.h
	TLOG " (not found)"
	rc=1
    fi
    rm -f /tmp/pd$$.o /tmp/pd$$.c
    return $rc
}


#
# AC_C_CONST checks to see if the compiler supports the const keyword
#
AC_C_CONST () {
    cat > /tmp/pd$$.c << EOF
const char me=1;
EOF
    LOGN "checking for \"const\" keyword"

    if $AC_CC -c /tmp/pd$$.c -o /tmp/pd$$.o 2>> config.log; then
	TLOG " (yes)"
	rc=0
    else
	echo "#define const /**/" >> config.h
	TLOG " (no)"
	rc=1
    fi
    rm -f /tmp/pd$$.o /tmp/pd$$.c
    return $rc
}


#
# AC_OUTPUT generates makefiles from makefile.in's
#
AC_OUTPUT () {
    echo "s,@LIBS@,$AC_LIBS,g" >> config.sub
    if [ -r config.sub ]; then
	test "$AC_SED_PROG" || AC_PROG_SED
	test "$AC_SED_PROG" || return 1

	echo                                   >> config.h
	echo "#endif/* ${AC_CONFIGURE_FOR} */" >> config.h

	rm -f config.cmd
	cat - > config.cmd << EOF
#! /bin/sh
${CC:+CC=\"$CC\"} ${CFLAGS:+CFLAGS=\"$CFLAGS\"} $0 $ac_configure_command
EOF
	chmod +x config.cmd

	for makefile in $*;do
	    if test -r ${makefile}.in; then
		LOG "generating $makefile"
		$AC_SED_PROG -f config.sub < ${makefile}.in > $makefile
	    else
		echo "WARNING: ${makefile}.in does not exist!" | tee -a config.log
	    fi
	done
    else
	echo 
    fi
}


#
# AC_PROG_INSTALL finds the install program and guesses whether it's a 
# Berkeley or GNU install program
#
AC_PROG_INSTALL () {

    DEST=`acLookFor install`

    LOGN "Checking for install"
    unset IS_BSD
    if [ "$DEST" ]; then
	# BSD install or GNU install?  Let's find out...
	touch /tmp/a$$

	$DEST /tmp/a$$ /tmp/b$$

	if test -r /tmp/a$$; then
	    LOG " ($DEST)"
	else
	    IS_BSD=1
	    LOG " ($DEST) bsd install"
	fi
	rm -f /tmp/a$$ /tmp/b$$
    else
	DEST=`acLookFor ginstall`
	if [ "$DEST" ]; then
	    LOG " ($DEST)"
	else
	    DEST="false"
	    LOG " (not found)"
	fi
    fi

    PROG_INSTALL="$DEST${IS_BSD:+" -c"}"

	cat >> config.sub << EOF
s,@INSTALL@,$PROG_INSTALL,g
s,@INSTALL_PROGRAM@,$PROG_INSTALL -s -m 755,g
s,@INSTALL_DATA@,$PROG_INSTALL -m 444,g
EOF
}

#
# acCheckCPP is a local that runs a C preprocessor with a given set of
# compiler options
#
acCheckCPP () {
	cat > /tmp/ngc$$.c << EOF
#define FOO BAR

FOO
EOF

    if $1 $2 /tmp/ngc$$.c 2>config.log > /tmp/ngc$$.o; then
	if grep -v '#define' /tmp/ngc$$.o | grep -qs BAR; then
	    echo "CPP=[$1], CPPFLAGS=[$2]" >> config.log
	    echo "s,@CPP@,$1,g" >> config.sub
	    echo "s/@CPPFLAGS@/$2/g" >> config.sub
	    rm /tmp/ngc$$.c /tmp/ngc$$.o
	    return 0
	fi
    fi
    rm /tmp/ngc$$.c /tmp/ngc$$.o
    return 1
}


#
# AC_PROG_CPP checks for cpp, then checks to see which CPPFLAGS are needed
# to run it as a filter.
#
AC_PROG_CPP () {
    DEST=${AC_CPP_PROG:-`AC_PATH=/lib:/usr/lib:$ac_default_path acLookFor cpp`}

    unset fail
    LOGN "Looking for cpp"
    if [ "$DEST" ]; then
	TLOGN " ($DEST)"
	acCheckCPP $DEST "$CPPFLAGS" || \
		 acCheckCPP $DEST -traditional-cpp -E || \
		 acCheckCPP $DEST -E || \
		 acCheckCPP $DEST -traditional-cpp -pipe || \
	         acCheckCPP $DEST -pipe || fail=1

	if [ "$fail" ]; then
	    TLOG " (can't run cpp as a pipeline)"
	    $__fail 1
	else
	    TLOG " ok"
	    return 0
	fi
    fi
    TLOG " (not found)"
    $__fail 1
}

#
# AC_FAIL spits out an error message, then __fail's 
AC_FAIL() {
    echo "$*"
    $__fail 1
}

#
# AC_SUB writes a substitution into config.sub
AC_SUB() {
    echo "s,@$1@,$2," >> config.sub
}

#
# AC_MAK writes a define into config.mak
AC_MAK() {
    echo "HAVE_$1 = 1" >> config.mak
}

#
# AC_CONFIG adds a configuration setting to all the config files
AC_CONFIG() {
    echo "#define PATH_$1 \"$2\"" >> config.h
    echo "HAVE_$1 = 1" >> config.mak
    echo "s;@$1@;$2;g" >> config.sub
}

#
# AC_DEFINE adds a #define to config.h
AC_DEFINE() {
    echo "#define $1 ${2:-1}" >> config.h
}
