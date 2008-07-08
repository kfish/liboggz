## Process this file with automake to produce Makefile.in

SUBDIRS = oggz-chop

EXTRA_DIST = oggz-diff

bin_SCRIPTS = oggz-diff

INCLUDES = -I$(top_builddir) -I$(top_builddir)/include \
           -I$(top_srcdir)/include \
           @OGG_CFLAGS@

OGGZDIR = ../liboggz
OGGZ_LIBS = $(OGGZDIR)/liboggz.la @OGG_LIBS@

oggz_any_programs = oggz

if OGGZ_CONFIG_READ
oggz_read_programs = oggz-dump oggz-info oggz-scan
oggz_read_noinst_programs =

if OGGZ_CONFIG_WRITE
oggz_rw_programs = oggz-merge oggz-rip oggz-validate oggz-comment oggz-sort
oggz_rw_noinst_programs = oggz-basetime
endif

endif

noinst_HEADERS = oggz_tools.h skeleton.h

# Programs to build
bin_PROGRAMS = $(oggz_any_programs) $(oggz_read_programs) $(oggz_rw_programs)
noinst_PROGRAMS = $(oggz_read_noinst_programs) $(oggz_rw_noinst_programs)

oggz_SOURCES = oggz.c
oggz_LDADD =

oggz_info_SOURCES = oggz-info.c oggz_tools.c skeleton.c
oggz_info_LDADD = $(OGGZ_LIBS) -lm

oggz_comment_SOURCES = oggz-comment.c oggz_tools.c
oggz_comment_LDADD = $(OGGZ_LIBS)

oggz_dump_SOURCES = oggz-dump.c oggz_tools.c
oggz_dump_LDADD = $(OGGZ_LIBS)

oggz_merge_SOURCES = oggz-merge.c oggz_tools.c
oggz_merge_LDADD = $(OGGZ_LIBS)

oggz_rip_SOURCES = oggz-rip.c oggz_tools.c
oggz_rip_LDADD = $(OGGZ_LIBS)

oggz_validate_SOURCES = oggz-validate.c oggz_tools.c
oggz_validate_LDADD = $(OGGZ_LIBS)

oggz_basetime_SOURCES = oggz-basetime.c oggz_tools.c
oggz_basetime_LDADD = $(OGGZ_LIBS)

oggz_scan_SOURCES = oggz-scan.c oggz_tools.c
oggz_scan_LDADD = $(OGGZ_LIBS)

oggz_sort_SOURCES = oggz-sort.c oggz_tools.c
oggz_sort_LDADD = $(OGGZ_LIBS)

