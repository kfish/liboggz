# Process this file with autoconf to produce a configure script.
AC_INIT
AC_CONFIG_SRCDIR([src/liboggz/oggz.c])

AC_PREREQ(2.53)

AC_CANONICAL_TARGET

AM_INIT_AUTOMAKE(liboggz, 0.9.5.1)
AM_CONFIG_HEADER(config.h)

SHARED_VERSION_INFO="3:0:2"
SHLIB_VERSION_ARG=""

# Checks for programs.
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_LIBTOOL

AC_C_CONST
AC_C_BIGENDIAN

dnl Add parameters for aclocal
AC_SUBST(ACLOCAL_AMFLAGS, "-I m4")

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h inttypes.h stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_OFF_T
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_REALLOC
AC_CHECK_FUNCS([memmove random])

# Check for pkg-config
AC_CHECK_PROG(HAVE_PKG_CONFIG, pkg-config, yes)

# Check for doxygen
AC_CHECK_PROG(HAVE_DOXYGEN, doxygen, true, false)
AM_CONDITIONAL(HAVE_DOXYGEN,$HAVE_DOXYGEN)
if test $HAVE_DOXYGEN = "false"; then
             AC_MSG_WARN([*** doxygen not found, docs will not be built])
fi

# Check for docbook
AC_CHECK_PROGS(DOCBOOKTOMAN, docbook-to-man docbook2man, false)
AM_CONDITIONAL(HAVE_DOCBOOKTOMAN, test "x$DOCBOOKTOMAN" != "xfalse")
AC_CHECK_PROG(HAVE_DOCBOOK2HTML, docbook2html, true, false)
AM_CONDITIONAL(HAVE_DOCBOOK2HTML,$HAVE_DOCBOOK2HTML)

# Check for valgrind
VALGRIND_ENVIRONMENT=""
ac_enable_valgrind=no
AC_ARG_ENABLE(valgrind-testing,
     [  --enable-valgrind-testing     enable running of tests inside Valgrind ],
     [ ac_enable_valgrind=yes ], [ ac_enable_valgrind=no] )

if test "x${ac_enable_valgrind}" = xyes ; then
  if test "x${enable_shared}" = xyes ; then
    VALGRIND_ENVIRONMENT="libtool --mode=execute "
  fi

  AC_CHECK_PROG(HAVE_VALGRIND, valgrind, yes, no)
  if test "x$HAVE_VALGRIND" = xyes ; then
    VALGRIND_ENVIRONMENT="$VALGRIND_ENVIRONMENT valgrind -q --leak-check=yes --show-reachable=yes --num-callers=100"
    AC_SUBST(VALGRIND_ENVIRONMENT)
    TESTS_INFO="Test suite will be run under:
    ${VALGRIND_ENVIRONMENT}"
  else
    TESTS_INFO="(Valgrind not found)"
  fi
else
  TESTS_INFO="(Valgrind testing not enabled)"
fi

dnl Checks for libraries.
LIBS=""

# check for getopt in a separate library
HAVE_GETOPT=no
AC_CHECK_LIB(getopt, getopt, HAVE_GETOPT="yes")
if test "x$HAVE_GETOPT" = xyes ; then
  GETOPT_LIBS="-lgetopt"
  AC_SUBST(GETOPT_LIBS)
fi

# check for getopt_long in standard library
HAVE_GETOPT_LONG=no
AC_CHECK_FUNC(getopt_long, HAVE_GETOPT_LONG="yes")
if test "x$HAVE_GETOPT_LONG" = xyes ; then
  AC_DEFINE(HAVE_GETOPT_LONG, [], [Define to 1 if you have the 'getopt_long' function])
fi

dnl Overall configuration success flag
oggz_config_ok=yes

oggz_read_programs=""
oggz_rw_programs=""

dnl
dnl  Configuration option for building of experimental code.
dnl

ac_enable_experimental=no
AC_ARG_ENABLE(experimental,
     [  --enable-experimental   enable building of experimental code ],
     [ ac_enable_experimental=yes ])

if test "x${ac_enable_experimental}" = xyes ; then
    AC_DEFINE(OGGZ_CONFIG_EXPERIMENTAL, [], [Define to build experimental code])
fi

dnl
dnl  Configuration option for building of reading support.
dnl

ac_enable_read=yes
AC_ARG_ENABLE(read,
     [  --disable-read          disable building of reading support ],
     [ ac_enable_read=no ], [ ac_enable_read=yes] )

if test "x${ac_enable_read}" = xyes ; then
    AC_DEFINE(OGGZ_CONFIG_READ, [1], [Build reading support])
    oggz_read_programs="oggzdump oggzinfo oggz-scan"
else
    AC_DEFINE(OGGZ_CONFIG_READ, [0], [Do not build reading support]) 
fi
AM_CONDITIONAL(OGGZ_CONFIG_READ, test "x${ac_enable_read}" = xyes)

dnl
dnl  Configuration option for building of writing support.
dnl
  
ac_enable_write=yes
AC_ARG_ENABLE(write,
     [  --disable-write         disable building of writing support ],
     [ ac_enable_write=no ], [ ac_enable_write=yes] )

if test "x${ac_enable_write}" = xyes ; then
    AC_DEFINE(OGGZ_CONFIG_WRITE, [1], [Build writing support])
    if test "x${ac_enable_read}" = xyes ; then
      oggz_rw_programs="oggzrip oggzmerge oggz-validate"
    fi
else
    AC_DEFINE(OGGZ_CONFIG_WRITE, [0], [Do not build writing support])
fi
AM_CONDITIONAL(OGGZ_CONFIG_WRITE, test "x${ac_enable_write}" = xyes)

dnl
dnl  Check read/write option sanity
dnl
if test "x${ac_enable_read}" = xno && test "x${ac_enable_write}" = xno ; then
  AC_MSG_ERROR([
***
*** You have attempted to explicitly disable both writing and reading.
***
])
fi

dnl Large file support
dnl Adapted from: libsndfile by Erik de Castro Lopo
dnl
dnl Find an appropriate type for oggz_off_t
dnl On systems supporting files larger than 2 Gig, oggz_off_t must be a
dnl 64 bit value. Unfortunately there is more than one way of ensuring
dnl this so need to do some pretty rigourous testing here.

unset ac_cv_sizeof_off_t

AC_CHECK_SIZEOF(off_t, 1)

case "$host_os" in
    mingw*)
        TYPEOF_OGGZ_OFF_T="__int64"
        OGGZ_OFF_MAX="0x7FFFFFFFFFFFFFFFLL"
        SIZEOF_OGGZ_OFF_T=8
        ;;
    *)
        if test "x$ac_cv_sizeof_off_t" = "x8" ; then
            # If sizeof (off_t) is 8, no further checking is needed.
            TYPEOF_OGGZ_OFF_T="off_t"
            OGGZ_OFF_MAX="0x7FFFFFFFFFFFFFFFLL"
            SIZEOF_OGGZ_OFF_T=8
        else
            # Check for common 64 bit file offset types.
            AC_CHECK_SIZEOF(loff_t,1)       # Fake default value.
            AC_CHECK_SIZEOF(off64_t,1)      # Fake default value.

            TYPEOF_OGGZ_OFF_T="unknown"
            if test "x$ac_cv_sizeof_loff_t" = "x8" ; then
                TYPEOF_OGGZ_OFF_T="loff_t"
                SIZEOF_OGGZ_OFF_T=8
            elif test "x$ac_cv_sizeof_off64_t" = "x8" ; then
                TYPEOF_OGGZ_OFF_T="off64_t"
                SIZEOF_OGGZ_OFF_T=8
            fi

            # Save the old sizeof (off_t) value  and then unset it to see if it
            # changes when Large File Support is enabled.

            pre_largefile_sizeof_off_t=$ac_cv_sizeof_off_t
            unset ac_cv_sizeof_off_t

            AC_SYS_EXTRA_LARGEFILE

            if test "x$ac_cv_sys_largefile_CFLAGS" = "xno" ; then
                ac_cv_sys_largefile_CFLAGS=""
            fi
            if test "x$ac_cv_sys_largefile_LDFLAGS" = "xno" ; then
                ac_cv_sys_largefile_LDFLAGS=""
            fi
            if test "x$ac_cv_sys_largefile_LIBS" = "xno" ; then
                ac_cv_sys_largefile_LIBS=""
            fi

            AC_CHECK_SIZEOF(off_t,1)        # Fake default value.

            if test "x$ac_cv_sizeof_off_t" = "x8" ; then
                    OGGZ_OFF_MAX="0x7FFFFFFFFFFFFFFFLL"
            elif test "x$ac_cv_sizeof_off_t" = "x$pre_largefile_sizeof_off_t" ; then
                AC_MSG_WARN([[This machine does not seem to support 64 bit file offsets.]])
                TYPEOF_OGGZ_OFF_T="off_t"
                SIZEOF_OGGZ_OFF_T=$ac_cv_sizeof_off_t
            elif test "x$TYPEOF_OGGZ_OFF_T" = "xunknown" ; then
                echo
                echo "*** The configure process has determined that this system is capable"
                echo "*** of Large File Support but has not been able to find a type which"
                echo "*** is an unambiguous 64 bit file offset."
                echo "*** Please contact the author to help resolve this problem."
                echo
                AC_MSG_ERROR([[Bad file offset type.]])
            fi
        fi
    ;;
    esac

if test $SIZEOF_OGGZ_OFF_T = 4 ; then
    OGGZ_OFF_MAX="0x7FFFFFFF"
    PRI_OGGZ_OFF_T="l"
elif test $SIZEOF_OGGZ_OFF_T = 8 ; then
    PRI_OGGZ_OFF_T="q"
fi

dnl The following configured variables are written into the public header
dnl <oggz/oggz_off_t_generated.h>:
dnl   oggz_off_t (typedef)
dnl   PRI_OGGZ_OFF_T (printf format)

dnl AC_DEFINE_UNQUOTED([TYPEOF_OGGZ_OFF_T],${TYPEOF_OGGZ_OFF_T}, [Set to long if unknown.])
AC_SUBST(TYPEOF_OGGZ_OFF_T)

AC_SUBST(PRI_OGGZ_OFF_T)

AC_DEFINE_UNQUOTED([SIZEOF_OGGZ_OFF_T],${SIZEOF_OGGZ_OFF_T}, [Set to sizeof (long) if unknown.])
AC_SUBST(SIZEOF_OGGZ_OFF_T)

AC_DEFINE_UNQUOTED([OGGZ_OFF_MAX],${OGGZ_OFF_MAX}, [Set to maximum allowed value of sf_count_t type.])
AC_SUBST(OGGZ_OFF_MAX)

AC_CHECK_TYPES(ssize_t)
AC_CHECK_SIZEOF(ssize_t,4)

dnl was defined to set oggz_off_t type (removed by laser13)
dnl AC_DEFINE_UNQUOTED([oggz_off_t], ${TYPEOF_OGGZ_OFF_T}, [Configured type of oggz_off_t])


dnl
dnl Detect Ogg
dnl

HAVE_OGG=no
if test "x$HAVE_PKG_CONFIG" = "xyes" ; then
  PKG_CHECK_MODULES(OGG, ogg >= 1.0, HAVE_OGG=yes, HAVE_OGG=no)
fi

if test "x$HAVE_OGG" = "xno" ; then
  XIPH_PATH_OGG(HAVE_OGG="yes", HAVE_OGG="no")
  if test "x$HAVE_OGG" = xno ; then
    oggz_config_ok="no"
    AC_CHECK_LIB(ogg, ogg_stream_init, HAVE_OGG="maybe")
  fi
fi

AM_CONDITIONAL(HAVE_OGG, [test "x$HAVE_OGG" = "xyes"])

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

dnl Use -Wall if we have gcc.
dnl changequote(,)dnl
if test "x$ac_cv_prog_gcc" = xyes ; then
  CFLAGS="$CFLAGS -Wall -g -std=c99 -Wdeclaration-after-statement"
fi
dnl changequote([,])dnl

case "$target_os" in
  linux* | solaris*)
    SHLIB_VERSION_ARG="-Wl,--version-script=Version_script"
    ;;
  *)
    ;;
esac

# Checks for library functions.

dnl
dnl  Configuration tests complete -- provide summary of results.
dnl

if test $oggz_config_ok = no ; then

AC_MSG_RESULT([
**************************************************************

*** $PACKAGE $VERSION: Automatic configuration FAILED.

*** The file config.log has full details.

*** The following required libraries are missing or
*** misconfigured on your system:
])

if test "x$HAVE_OGG" != xyes ; then
    AC_MSG_RESULT(
[*** libogg, available from  http://www.xiph.org/ogg/])
fi
if test "x$HAVE_OGG" = xmaybe ; then
    AC_MSG_RESULT(
[      Development files missing: The libogg library binary seems to be
      installed, but building of a test program failed.
])
fi

AC_MSG_RESULT(
[*** If you install the required libraries from source, you
*** need to inform the dynamic linker of their location. If
*** you install them in a system-wide directory such as
*** /usr/local (the default), you must ensure that
*** /usr/local/lib is listed in /etc/ld.so.conf, then run
*** ldconfig to update the dynamic linking system.
*** Alternatively, you can set your LD_LIBRARY_PATH environment
*** variable to include the library installation directory.
])


AC_MSG_RESULT(
[**************************************************************
])


AC_MSG_ERROR([
***
*** After fixing the above problems, you must run ./configure again.
***
]) 

else

AC_SUBST(SHLIB_VERSION_ARG)
AC_SUBST(SHARED_VERSION_INFO)

AC_OUTPUT([
Makefile
doc/Makefile
doc/Doxyfile
include/Makefile
include/oggz/Makefile
include/oggz/oggz_off_t_generated.h
src/Makefile
src/liboggz/Version_script
src/liboggz/Makefile
src/tools/Makefile
src/tools/oggzdiff
src/tests/Makefile
src/examples/Makefile
oggz.pc
oggz-uninstalled.pc
])

AS_AC_EXPAND(LIBDIR, ${libdir})
AS_AC_EXPAND(INCLUDEDIR, ${includedir})
AS_AC_EXPAND(BINDIR, ${bindir})
AS_AC_EXPAND(DOCDIR, ${datadir}/doc)

AC_MSG_RESULT([
------------------------------------------------------------------------
  $PACKAGE $VERSION:  Automatic configuration OK.

  General configuration:

    Experimental code: ........... ${ac_enable_experimental}
    Reading support: ............. ${ac_enable_read}
    Writing support: ............. ${ac_enable_write}

  Tools:

    ${oggz_read_programs} ${oggz_rw_programs}

  Installation paths:

    liboggz: ..................... ${LIBDIR}
    Tools: ....................... ${BINDIR}
    C header files: .............. ${INCLUDEDIR}/oggz
    Documentation: ............... ${DOCDIR}/$PACKAGE

  Building:

    Type 'make' to compile $PACKAGE.

    Type 'make install' to install $PACKAGE.

    Type 'make check' to test $PACKAGE using the unittests
    contained in the src/tests directory.
    ${TESTS_INFO}

  Example programs will be built but not installed.
------------------------------------------------------------------------
])


fi
