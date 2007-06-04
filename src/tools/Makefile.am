## Process this file with automake to produce Makefile.in

AM_CFLAGS = -Wall -pedantic -g

EXTRA_DIST = oggzdiff

bin_SCRIPTS = oggzdiff

INCLUDES = -I$(top_srcdir)/include @OGG_CFLAGS@

OGGZDIR = ../liboggz
OGGZ_LIBS = $(OGGZDIR)/liboggz.la @OGG_LIBS@

if OGGZ_CONFIG_READ
oggz_read_programs = oggzdump oggzinfo oggz-scan
oggz_read_noinst_programs =

if OGGZ_CONFIG_WRITE
oggz_rw_programs = oggzmerge oggzrip oggz-validate
oggz_rw_noinst_programs = oggz-basetime
endif

endif

noinst_HEADERS = oggz_tools.h skeleton.h

# Programs to build
bin_PROGRAMS = $(oggz_read_programs) $(oggz_rw_programs)
noinst_PROGRAMS = $(oggz_read_noinst_programs) $(oggz_rw_noinst_programs)

oggzinfo_SOURCES = oggzinfo.c oggz_tools.c skeleton.c
oggzinfo_LDADD = $(OGGZ_LIBS) -lm

oggzdump_SOURCES = oggzdump.c oggz_tools.c
oggzdump_LDADD = $(OGGZ_LIBS)

oggzmerge_SOURCES = oggzmerge.c oggz_tools.c
oggzmerge_LDADD = $(OGGZ_LIBS)

oggzrip_SOURCES = oggzrip.c oggz_tools.c
oggzrip_LDADD = $(OGGZ_LIBS)

oggz_validate_SOURCES = oggz-validate.c oggz_tools.c
oggz_validate_LDADD = $(OGGZ_LIBS)

oggz_basetime_SOURCES = oggz-basetime.c oggz_tools.c
oggz_basetime_LDADD = $(OGGZ_LIBS)

oggz_scan_SOURCES = oggz-scan.c oggz_tools.c
oggz_scan_LDADD = $(OGGZ_LIBS)
