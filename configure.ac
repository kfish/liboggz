# Process this file with autoconf to produce a configure script.
AC_INIT
AC_CONFIG_SRCDIR([src/liboggz/oggz.c])

AC_PREREQ(2.53)
AC_CONFIG_AUX_DIR(autotools)
AC_CANONICAL_TARGET

AM_INIT_AUTOMAKE(liboggz, 0.5.43)
AM_CONFIG_HEADER(config.h)

SHARED_VERSION_INFO="0:5:0"
SHLIB_VERSION_ARG=""

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_LIBTOOL

AC_C_CONST
AC_C_BIGENDIAN


# Check for doxygen
AC_CHECK_PROG(HAVE_DOXYGEN, doxygen, true, false)
AM_CONDITIONAL(HAVE_DOXYGEN,$HAVE_DOXYGEN)
if test $HAVE_DOXYGEN = "false"; then
             AC_MSG_WARN([*** doxygen not found, docs will not be built])
fi

# Check for docbook
AC_CHECK_PROG(HAVE_DOCBOOK, docbook2man, true, false)
AM_CONDITIONAL(HAVE_DOCBOOK,$HAVE_DOCBOOK)

dnl Check for types

dnl Checks for libraries.
LIBS=""

# check for getopt in standard library
HAVE_GETOPT=no
AC_CHECK_LIB(getopt, getopt, HAVE_GETOPT="yes")
if test "x$HAVE_GETOPT" = xyes ; then
  GETOPT_LIBS="-lgetopt"
  AC_SUBST(GETOPT_LIBS)
fi

dnl Overall configuration success flag
oggz_config_ok=yes

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
XIPH_PATH_OGG(HAVE_OGG="yes", HAVE_OGG="no")
if test "x$HAVE_OGG" = xno ; then
  oggz_config_ok="no"
  AC_CHECK_LIB(ogg, ogg_stream_init, HAVE_OGG="maybe")
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

  Installation paths:

    liboggz: ..................... ${prefix}/lib
    C header files: .............. ${prefix}/include/oggz
    Documentation: ............... ${prefix}/share/doc/$PACKAGE

  Example programs will be built but not installed.
------------------------------------------------------------------------
])


fi
