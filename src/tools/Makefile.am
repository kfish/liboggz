## Process this file with automake to produce Makefile.in

AM_CFLAGS = -Wall -pedantic -g

EXTRA_DIST = oggzdiff

bin_SCRIPTS = oggzdiff

INCLUDES = -I$(top_srcdir)/include @OGG_CFLAGS@

OGGZDIR = ../liboggz
OGGZ_LIBS = $(OGGZDIR)/liboggz.la @OGG_LIBS@

if OGGZ_CONFIG_READ
oggz_read_programs = oggzdump
oggz_read_noinst_programs = oggzinfo oggzed

if OGGZ_CONFIG_WRITE
oggz_rw_programs = oggzmerge oggzrip
endif

endif

# Programs to build
bin_PROGRAMS = $(oggz_read_programs) $(oggz_rw_programs)
noinst_PROGRAMS = $(oggz_read_noinst_programs)

oggzinfo_SOURCES = oggzinfo.c
oggzinfo_LDADD = $(OGGZ_LIBS) -lm

oggzed_SOURCES = oggzed.c
oggzed_LDADD = $(OGGZ_LIBS)

oggzdump_SOURCES = oggzdump.c
oggzdump_LDADD = $(OGGZ_LIBS)

oggzmerge_SOURCES = oggzmerge.c
oggzmerge_LDADD = $(OGGZ_LIBS)

oggzrip_SOURCES = oggzrip.c
oggzrip_LDADD = $(OGGZ_LIBS)
