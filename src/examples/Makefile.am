## Process this file with automake to produce Makefile.in

AM_CFLAGS = -Wall -pedantic -g

INCLUDES = -I$(top_srcdir)/include @OGG_CFLAGS@

OGGZDIR = ../liboggz
OGGZ_LIBS = $(OGGZDIR)/liboggz.la @OGG_LIBS@

if OGGZ_CONFIG_READ
if OGGZ_CONFIG_WRITE
oggz_rw_programs = identity
endif
endif

if OGGZ_CONFIG_READ
oggz_read_programs = read-file read-pages read-io
endif

if OGGZ_CONFIG_WRITE
oggz_write_programs = write-feed write-hungry
endif

# Programs to build
noinst_PROGRAMS = $(oggz_rw_programs) $(oggz_read_programs) \
	$(oggz_write_programs)

identity_SOURCES = identity.c
identity_LDADD = $(OGGZ_LIBS)

read_file_SOURCES = read-file.c
read_file_LDADD = $(OGGZ_LIBS)

read_pages_SOURCES = read-pages.c
read_pages_LDADD = $(OGGZ_LIBS)

read_io_SOURCES = read-io.c
read_io_LDADD = $(OGGZ_LIBS)

write_feed_SOURCES = write-feed.c
write_feed_LDADD = $(OGGZ_LIBS)

write_hungry_SOURCES = write-hungry.c
write_hungry_LDADD = $(OGGZ_LIBS)
