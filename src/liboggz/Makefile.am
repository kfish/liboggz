## Process this file with automake to produce Makefile.in

AM_CFLAGS = -Wall -pedantic

INCLUDES = $(INCLTDL) -I$(top_srcdir)/include @OGG_CFLAGS@

EXTRA_DIST = Version_script.in

# Libraries to build
lib_LTLIBRARIES = liboggz.la

liboggz_la_SOURCES = \
	oggz.c \
	oggz_private.h oggz_byteorder.h oggz_compat.h oggz_macros.h \
	oggz_io.c \
	oggz_read.c oggz_write.c \
	oggz_auto.c oggz_auto.h \
	oggz_table.c \
	oggz_vector.c oggz_vector.h

liboggz_la_LDFLAGS = -version-info @SHARED_VERSION_INFO@ @SHLIB_VERSION_ARG@
