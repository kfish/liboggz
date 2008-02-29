## Process this file with automake to produce Makefile.in

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/include @OGG_CFLAGS@

OGGZDIR = ../../liboggz
OGGZ_LIBS = $(OGGZDIR)/liboggz.la @OGG_LIBS@

if OGGZ_CONFIG_READ
if OGGZ_CONFIG_WRITE
oggz_rw_programs = oggz-chop
endif

endif

# Programs to build
bin_PROGRAMS = $(oggz_rw_programs)
noinst_PROGRAMS = httpdate_test

TESTS_ENVIRONMENT = $(VALGRIND_ENVIRONMENT)

TESTS = httpdate_test

noinst_HEADERS = alloc_snprintf.h cgi.h cmd.h header.h httpdate.h memory.h oggz-chop.h tests.h timespec.h

oggz_chop_SOURCES = oggz-chop.c ../oggz_tools.c cmd.c cgi.c header.c httpdate.c main.c timespec.c
oggz_chop_LDADD = $(OGGZ_LIBS)

httpdate_test_SOURCES = httpdate.c httpdate_test.c

