## Process this file with automake to produce Makefile.in

AM_CFLAGS = -Wall -pedantic -g

INCLUDES = -I$(top_srcdir)/include

OGGZDIR = ../liboggz
OGGZ_LIBS = $(OGGZDIR)/liboggz.la @OGG_LIBS@

if OGGZ_CONFIG_READ
if OGGZ_CONFIG_WRITE
oggz_rw_programs = identity
endif
endif

if OGGZ_CONFIG_READ
oggz_read_programs = read-file
endif

# Programs to build
noinst_PROGRAMS = $(oggz_rw_programs) $(oggz_read_programs)

identity_SOURCES = identity.c
identity_LDADD = $(OGGZ_LIBS)

read_file_SOURCES = read-file.c
read_file_LDADD = $(OGGZ_LIBS)
