## Process this file with automake to produce Makefile.in

AM_CFLAGS = -Wall -pedantic -g

EXTRA_DIST = oggzdiff

bin_SCRIPTS = oggzdiff

INCLUDES = -I$(top_srcdir)/include

OGGZDIR = ../liboggz
OGGZ_LIBS = $(OGGZDIR)/liboggz.la @OGG_LIBS@

if OGGZ_CONFIG_READ
oggz_read_programs = oggzinfo oggzdump oggzed
endif

# Programs to build
bin_PROGRAMS = $(oggz_read_programs)

oggzinfo_SOURCES = oggzinfo.c
oggzinfo_LDADD = $(OGGZ_LIBS)

oggzed_SOURCES = oggzed.c
oggzed_LDADD = $(OGGZ_LIBS)

oggzdump_SOURCES = oggzdump.c
oggzdump_LDADD = $(OGGZ_LIBS)

