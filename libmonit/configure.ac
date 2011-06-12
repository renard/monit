# Copyright (C) 2000-2011 Tildeslash Ltd. All rights reserved.

AC_PREREQ([2.53])

AC_INIT([monit], [1.0], [monit-dev@nongnu.org])
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([src])

# ---------------------------------------------------------------------------
# Programs 
# ---------------------------------------------------------------------------

AC_PROG_CC

# ---------------------------------------------------------------------------
# Libtool 
# ---------------------------------------------------------------------------

AC_PROG_LIBTOOL

# ---------------------------------------------------------------------------
# Build options
# ---------------------------------------------------------------------------

AC_ARG_ENABLE(optimized,
	AS_HELP_STRING([--enable-optimized], 
        	[Build software optimized]),
    [
        CFLAGS=`echo $CFLAGS|sed 's/-O.//'`
        if test "x$enableval" = "xyes" ; then
   	    	CFLAGS=`echo $CFLAGS|sed 's/-g//'`
		CFLAGS="$CFLAGS -O3"
		OPTIMIZED=1
	else
		OPTIMIZED=0
        fi
    ],
    [
	OPTIMIZED=0
    ]
)

AC_ARG_ENABLE(profiling,
        AS_HELP_STRING([--enable-profiling],
                [Build with debug and profiling options]),
    [
        if test "x$enableval" = "xyes" ; then
                CFLAGS="$CFLAGS -g -pg"
                profile="true"
        fi
    ],
    [
        profile="false"
    ]
)

AC_ARG_ENABLE([openssl],
        AS_HELP_STRING([--enable-openssl(=<path>)], 
                [Link Monit with openssl. An optional path argument may
                be given to specify the top-level directory to search for
                openssl to link with]),
     [
        openssl="true"
        if test "x$enableval" = "xyes"; then
		AC_CHECK_LIB([ssl], [SSL_library_init], [], [AC_MSG_ERROR([libssl not found])])
		AC_CHECK_LIB([crypto], [SHA1_Init], [], [AC_MSG_ERROR([libcrypto not found])])
	else
                AC_MSG_CHECKING([for openssl in $enableval])
                LDFLAGS="-L$enableval/lib -lssl -lcrypto $LDFLAGS "
                CPPFLAGS="-I$enableval/include $CPPFLAGS"
                if test -r "$enableval/lib/libssl.a" -a -r "$enableval/lib/libcrypto.a"; then
                        AC_MSG_RESULT([ok])
                else
                       AC_MSG_ERROR([openssl not found in $enableval])
                fi
        fi
     ],[openssl="false"])

# ---------------------------------------------------------------------------
# Libraries 
# ---------------------------------------------------------------------------

AC_CHECK_LIB([pthread], [pthread_create], [], [AC_MSG_ERROR([POSIX thread library is required])])

# ---------------------------------------------------------------------------
# Header files 
# ---------------------------------------------------------------------------

AC_HEADER_STDC

# ------------------------------------------------------------------------
# Functions 
# ------------------------------------------------------------------------

# Require a working setjmp
AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <setjmp.h>]],
        [[jmp_buf env; setjmp(env);]])], 
        [], [AC_MSG_FAILURE([setjmp is required])])


# ------------------------------------------------------------------------
# Architecture/OS
# ------------------------------------------------------------------------

if test `uname` = "Linux"  
then
   CFLAGS="$CFLAGS -D _REENTRANT"
   AC_DEFINE([LINUX], 1, [Define to 1 if the system is Linux])
elif test `uname` = "FreeBSD"
then
   CFLAGS="$CFLAGS -D _REENTRANT"
   AC_DEFINE([FREEBSD], 1, [Define to 1 if the system is FreeBSD])
elif test `uname` = "OpenBSD"
then
   CFLAGS="$CFLAGS -D _REENTRANT"
   AC_DEFINE([OPENBSD], 1, [Define to 1 if the system is OpenBSD])
elif test `uname` = "Darwin"
then
   CFLAGS="$CFLAGS -DREENTRANT"
   AC_DEFINE([DARWIN], 1, [Define to 1 if the system is OSX])
elif test `uname` = "SunOS"
then
   LIBS="$(DEPS_LIBS) -lsocket -lnsl"
   CFLAGS="$CFLAGS -D _REENTRANT -D_POSIX_PTHREAD_SEMANTICS -m64"
   LDFLAGS="$LDFLAGS -m64"
   if test `uname -m` != "i86pc"
   then
       CFLAGS="$CFLAGS -mtune=v9"
       LDFLAGS="$LDFLAGS -mtune=v9"
   fi
   AC_DEFINE([SOLARIS], 1, [Define to 1 if the system is SOLARIS])
else
   AC_MSG_ERROR([Architecture not supported: `uname`.])
fi

# ---------------------------------------------------------------------------
# Compiler 
# ---------------------------------------------------------------------------

# Compiler characteristics
AC_C_CONST
AC_C_BIGENDIAN

# If the compiler is gcc, tune warnings and make the char type unsigned 
# and enable C99 support
if test "x$GCC" = "xyes"; then
	CFLAGS="$CFLAGS -Wall -Wunused -Wno-unused-label -funsigned-char";
        # Add C99 support
        CFLAGS="$CFLAGS -D_GNU_SOURCE -std=c99"
        # does this compiler support -Wno-pointer-sign ?
        svd_CFLAGS="$CFLAGS"
        CFLAGS="-Wno-pointer-sign $CFLAGS"
        AC_COMPILE_IFELSE([AC_LANG_PROGRAM([], [return 0;])], [], [CFLAGS="$svd_CFLAGS"])        
        # does this compiler support -Wno-address ?
        svd_CFLAGS="$CFLAGS"
        CFLAGS="-Wno-address $CFLAGS"
        AC_COMPILE_IFELSE([AC_LANG_PROGRAM([], [return 0;])], [], [CFLAGS="$svd_CFLAGS"])
fi


# ------------------------------------------------------------------------
# Atomic support
# ------------------------------------------------------------------------
AC_MSG_CHECKING(for Atomic support)
if test `uname` = "Darwin" -o `uname` = "FreeBSD" 
then
        AC_MSG_RESULT(yes)
elif test `uname` = "SunOS"
then
        AC_CHECK_HEADER([atomic.h], 
        [AC_MSG_RESULT(yes)],
        [AC_MSG_ERROR([atomic.h is required for atomic support])])
else
        AC_RUN_IFELSE([AC_LANG_PROGRAM([],[
             #if ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 1)))
             return 0;
             #else
             return 1;
             #endif
	])],
	[AC_MSG_RESULT(yes)],
	[AC_MSG_FAILURE([GCC version > 4.1 is required for atomic support])])
fi


# ------------------------------------------------------------------------
# IPv6 Support
# ------------------------------------------------------------------------

AC_MSG_CHECKING(for IPv6 support)
AC_CACHE_VAL(ac_cv_ipv6,
	AC_TRY_RUN([
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	/* Make sure the definitions for AF_INET6 and struct sockaddr_in6
	 * are defined, and that we can actually create an IPv6 TCP socket.*/
	main()
	{
		int fd;
		struct sockaddr_in6 foo;
		fd = socket(AF_INET6, SOCK_STREAM, 0);
		exit(fd >= 0 ? 0 : 1);
	}],
	ac_cv_ipv6=yes,
	ac_cv_ipv6=no,
	ac_cv_ipv6=no))
AC_MSG_RESULT($ac_cv_ipv6)
if test $ac_cv_ipv6 = yes ; then
	AC_DEFINE([IPV6], 1, [Define to 1 if the system supports IPv6])
fi

# ---------------------------------------------------------------------------
# Functions 
# ---------------------------------------------------------------------------


# ---------------------------------------------------------------------------
# Outputs
# ---------------------------------------------------------------------------

AM_CONFIG_HEADER(src/xconfig.h)
AC_OUTPUT(Makefile)

cat <<EOT
+------------------------------------------------------------+
| License:                                                   |
| This is Open Source Software and use is subject to the GNU |
| AFFERO GENERAL PUBLIC LICENSE version 3, available in this |
| distribution in the file COPYING.                          |
|                                                            |
| By continuing this installation process, you are bound by  | 
| the terms of this license agreement. If you do not agree   |
| with the terms of this license, you must abort the         |
| installation process at this point.                        |
+------------------------------------------------------------+
| Libmonit is configured as follows:                         |
|                                                            |
EOT

if test "x1" = "x$OPTIMIZED"; then
echo "|   Optimized:                                    ENABLED    |"
else
echo "|   Optimized:                                    DISABLED   |"
fi
if test "xfalse" = "x$profile"; then
echo "|   Profiling:                                    DISABLED   |"
else
echo "|   Profiling:                                    ENABLED    |"
fi
if test "xtrue" = "x$openssl"; then
echo "|   Openssl:                                      ENABLED    |"
fi
echo "+------------------------------------------------------------+"


