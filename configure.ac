# Process this file with autoconf to produce a configure script.
AC_INIT
AC_CONFIG_SRCDIR([src/liboggz/oggz.c])

AC_PREREQ(2.53)

AC_CANONICAL_TARGET

AM_INIT_AUTOMAKE(liboggz, 0.9.1)
AM_CONFIG_HEADER(config.h)

SHARED_VERSION_INFO="2:0:1"
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
AC_CHECK_PROG(HAVE_DOCBOOKTOMAN, docbook-to-man, true, false)
AM_CONDITIONAL(HAVE_DOCBOOKTOMAN,$HAVE_DOCBOOKTOMAN)
AC_CHECK_PROG(HAVE_DOCBOOK2HTML, docbook2html, true, false)
AM_CONDITIONAL(HAVE_DOCBOOK2HTML,$HAVE_DOCBOOK2HTML)

# Check for valgrind
ac_enable_valgrind=no
AC_ARG_ENABLE(valgrind-testing,
     [  --enable-valgrind-testing     enable running of tests inside Valgrind ],
     [ ac_enable_valgrind=yes ], [ ac_enable_valgrind=no] )

if test "x${ac_enable_valgrind}" = xyes ; then
  if test "x${enable_shared}" = xyes ; then
    AC_MSG_WARN([
*** You have requested to test under valgrind, but have not disabled the
*** building of shared libraries. You usually need to do:
***   ./configure --disable-shared --enable-valgrind-testing
*** for Valgrind to produce useful output.])
  fi

  AC_CHECK_PROG(HAVE_VALGRIND, valgrind, yes, no)
  if test "x$HAVE_VALGRIND" = xyes ; then
    VALGRIND_ENVIRONMENT="valgrind -q --leak-check=yes --show-reachable=yes --num-callers=100"
    AC_SUBST(VALGRIND_ENVIRONMENT)
    TESTS_INFO="'make check' test suite will be run under:
  ${VALGRIND_ENVIRONMENT}"
  else
    TESTS_INFO="'make check' to run test suite (Valgrind not found)"
  fi
else
  TESTS_INFO="'make check' to run test suite (Valgrind testing not enabled)"
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
    oggz_read_programs="oggzdump oggzinfo"
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
  CFLAGS="$CFLAGS -Wall -g"
fi
dnl changequote([,])dnl

case "$target_os" in
  darwin* | rhapsody*)
    dnl Disable -Wall and -pedantic for Apple Darwin/Rhapsody.
    dnl System headers on these systems are broken.
    temp_CFLAGS=`echo $CFLAGS | sed "s/-Wall -pedantic//"`
    CFLAGS=$temp_CFLAGS
    ;;
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
src/Makefile
src/liboggz/Version_script
src/liboggz/Makefile
src/tools/Makefile
src/tools/oggzdiff
src/tests/Makefile
src/examples/Makefile
oggz.pc
])

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

    liboggz: ..................... ${prefix}/lib
    Tools: ....................... ${prefix}/bin
    C header files: .............. ${prefix}/include/oggz
    Documentation: ............... ${prefix}/share/doc/$PACKAGE

  ${TESTS_INFO}

  Example programs will be built but not installed.
------------------------------------------------------------------------
])


fi
