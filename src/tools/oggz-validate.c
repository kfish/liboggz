/*
   Copyright (C) 2003 Commonwealth Scientific and Industrial Research
   Organisation (CSIRO) Australia

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

   - Neither the name of CSIRO Australia nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
   PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE ORGANISATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <errno.h>

#include <oggz/oggz.h>

#include "oggz_tools.h"

#define MAX_ERRORS 10

#define SUBSECONDS 1000.0

typedef ogg_int64_t timestamp_t;

typedef struct {
  int error;
  char * description;
} error_text;

static error_text errors[] = {
  {5, "Multiple bos packets"},
  {6, "Multiple eos packets"},
  {20, "Packet belongs to unknown serialno"},
  {24, "Granulepos out of order within logical bitstream"},
  {0, NULL}
};

static int multifile = 0;
static char * current_filename = NULL;
static timestamp_t current_timestamp = 0;
static int exit_status = 0;
static int nr_errors = 0;

static void
usage (char * progname)
{
  int i = 0;

  printf ("Usage: %s [options] filename ...\n", progname);
  printf ("Validate the Ogg framing of one or more files\n");
  printf ("\n%s detects the following errors in Ogg framing:\n", progname);
  printf ("  Packet out of order\n");
  for (i = 0; errors[i].error; i++) {
    printf ("  %s\n", errors[i].description);
  }
  printf ("\nMiscellaneous options\n");
  printf ("  -h, --help             Display this help and exit\n");
  printf ("  -v, --version          Output version information and exit\n");
  printf ("\n");
  printf ("Exit status is 0 if all input files are valid, 1 otherwise.\n\n");
  printf ("Please report bugs to <ogg-dev@xiph.org>\n");
}

static int
log_error (void)
{
  if (multifile && nr_errors == 0) {
    fprintf (stderr, "%s: Error:\n", current_filename);
  }

  exit_status = 1;

  nr_errors++;
  if (nr_errors > MAX_ERRORS)
    return OGGZ_STOP_ERR;

  return OGGZ_STOP_OK;
}

static ogg_int64_t
gp_to_granule (OGGZ * oggz, long serialno, ogg_int64_t granulepos)
{
  int granuleshift;
  ogg_int64_t iframe, pframe;

  granuleshift = oggz_get_granuleshift (oggz, serialno);

  iframe = granulepos >> granuleshift;
  pframe = granulepos - (iframe << granuleshift);

  return (iframe + pframe);
}

static timestamp_t
gp_to_time (OGGZ * oggz, long serialno, ogg_int64_t granulepos)
{
  ogg_int64_t gr_n, gr_d;
  ogg_int64_t granule;

  if (granulepos == -1) return -1.0;
  if (oggz_get_granulerate (oggz, serialno, &gr_n, &gr_d) != 0) return -1.0;

  granule = gp_to_granule (oggz, serialno, granulepos);

  return (timestamp_t)((double)(granule * gr_d) / (double)gr_n);
}

static int
read_packet (OGGZ * oggz, ogg_packet * op, long serialno, void * user_data)
{
  OGGZ * writer = (OGGZ *)user_data;
  timestamp_t timestamp;
  int flush;
  int ret = 0, feed_err = 0;

  timestamp = gp_to_time (oggz, serialno, op->granulepos);
  if (timestamp != -1.0) {
    if (timestamp < current_timestamp) {
      ret = log_error();
      ot_fprint_time (stderr, (double)timestamp/SUBSECONDS);
      fprintf (stderr, ": serialno %010ld: Packet out of order (previous ",
	       serialno);
      ot_fprint_time (stderr, (double)current_timestamp/SUBSECONDS);
      fprintf (stderr, ")\n");
    }
    current_timestamp = timestamp;
  }

  if (op->granulepos == -1) {
    flush = 0;
  } else {
    flush = OGGZ_FLUSH_AFTER;
  }

  if ((feed_err = oggz_write_feed (writer, op, serialno, flush, NULL)) != 0) {
    ret = log_error ();
    if (timestamp == -1.0) {
      fprintf (stderr, "%ld", oggz_tell (oggz));
    } else {
      ot_fprint_time (stderr, (double)timestamp/SUBSECONDS);
    }
    fprintf (stderr,
	     ": serialno %010ld: Packet violates Ogg framing constraints: %d\n",
	     serialno, feed_err);
  }

  return ret;
}

static int
validate (char * filename)
{
  OGGZ * reader, * writer;
  unsigned char buf[1024];
  long n;
  int active = 1;

  current_filename = filename;
  current_timestamp = 0.0;
  nr_errors = 0;

  /*printf ("oggz-validate: %s\n", filename);*/
  
  if ((reader = oggz_open (filename, OGGZ_READ|OGGZ_AUTO)) == NULL) {
    fprintf (stderr, "oggz-validate: unable to open file %s\n", filename);
    exit (1);
  }

  if ((writer = oggz_new (OGGZ_WRITE|OGGZ_AUTO)) == NULL) {
    fprintf (stderr, "oggz-validate: unable to create new writer\n");
    exit (1);
  }

  oggz_set_read_callback (reader, -1, read_packet, writer);

  while (active && (n = oggz_read (reader, 1024)) != 0) {
    if (nr_errors > MAX_ERRORS) {
      fprintf (stderr,
	       "oggz-validate: maximum error count reached, bailing out ...\n");
      active = 0;
    } else while (oggz_write_output (writer, buf, n) > 0);
  }

  oggz_close (writer);
  oggz_close (reader);

  return active ? 0 : -1;
}

int
main (int argc, char ** argv)
{
  int show_version = 0;
  int show_help = 0;

  char * progname;
  char * filename;
  int i = 1;

  ot_init();

  progname = argv[0];

  if (argc < 2) {
    usage (progname);
    return (1);
  }

  while (1) {
    char * optstring = "hv";

#ifdef HAVE_GETOPT_LONG
    static struct option long_options[] = {
      {"help", no_argument, 0, 'h'},
      {"version", no_argument, 0, 'v'},
      {0,0,0,0}
    };

    i = getopt_long(argc, argv, optstring, long_options, NULL);
#else
    i = getopt (argc, argv, optstring);
#endif
    if (i == -1) {
      i = 1;
      break;
    }
    if (i == ':') {
      usage (progname);
      goto exit_err;
    }

    switch (i) {
    case 'h': /* help */
      show_help = 1;
      break;
    case 'v': /* version */
      show_version = 1;
      break;
    default:
      break;
    }
  }

  if (show_version) {
    printf ("%s version " VERSION "\n", progname);
  }

  if (show_help) {
    usage (progname);
  }

  if (show_version || show_help) {
    goto exit_out;
  }

  if (optind >= argc) {
    usage (progname);
    goto exit_err;
  }

  if (argc-i > 2) multifile = 1;

  for (; i < argc; i++) {
    filename = argv[i];
    validate (filename);
  }

 exit_out:
  exit (exit_status);

 exit_err:
  exit (1);
}
