## Process this file with automake to produce Makefile.in

AM_CFLAGS = -Wall -pedantic

INCLUDES = -I$(top_srcdir)/include

OGGZDIR = ../liboggz
OGGZ_LIBS = $(OGGZDIR)/liboggz.la @OGG_LIBS@

# Test programs

test: check

if OGGZ_CONFIG_WRITE
write_tests = write-bad-guard write-unmarked-guard write-recursive \
	write-bad-bytes write-bad-bos write-dup-bos write-bad-eos \
	write-bad-granulepos write-bad-packetno
endif

TESTS = $(write_tests)

noinst_PROGRAMS = $(TESTS)
noinst_HEADERS = oggz_tests.h

write_bad_guard_SOURCES = write-bad-guard.c
write_bad_guard_LDADD = $(OGGZ_LIBS)

write_unmarked_guard_SOURCES = write-unmarked-guard.c
write_unmarked_guard_LDADD = $(OGGZ_LIBS)

write_recursive_SOURCES = write-recursive.c
write_recursive_LDADD = $(OGGZ_LIBS)

write_bad_bytes_SOURCES = write-bad-bytes.c
write_bad_bytes_LDADD = $(OGGZ_LIBS)

write_bad_bos_SOURCES = write-bad-bos.c
write_bad_bos_LDADD = $(OGGZ_LIBS)

write_dup_bos_SOURCES = write-dup-bos.c
write_dup_bos_LDADD = $(OGGZ_LIBS)

write_bad_eos_SOURCES = write-bad-eos.c
write_bad_eos_LDADD = $(OGGZ_LIBS)

write_bad_granulepos_SOURCES = write-bad-granulepos.c
write_bad_granulepos_LDADD = $(OGGZ_LIBS)

write_bad_packetno_SOURCES = write-bad-packetno.c
write_bad_packetno_LDADD = $(OGGZ_LIBS)
