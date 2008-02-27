## Process this file with automake to produce Makefile.in

INCLUDES = -I$(top_srcdir)/include @OGG_CFLAGS@

OGGZDIR = ../../liboggz
OGGZ_LIBS = $(OGGZDIR)/liboggz.la @OGG_LIBS@

if OGGZ_CONFIG_READ
if OGGZ_CONFIG_WRITE
oggz_rw_programs = oggz-chop
endif

endif

# Programs to build
bin_PROGRAMS = $(oggz_rw_programs)

oggz_chop_SOURCES = oggz-chop.c ../oggz_tools.c
oggz_chop_LDADD = $(OGGZ_LIBS)
