/* -*- c-file-style: "gnu" -*- */
/*
  Copyright (C) 2005 Commonwealth Scientific and Industrial Research
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

  Author: David Kuehling <dvdkhlng@gmx.de>
  Created: 20041231
*/

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <getopt.h>
#include <errno.h>

#include <oggz/oggz.h>

#ifdef _WIN32
/* Supply missing headers and functions for Win32 */

#include <fcntl.h>
#endif

#define READ_SIZE 4096
#define WRITE_SIZE 4096

#define MAX_FILTER 64

typedef struct {
  OGGZ *reader;
  FILE *outfile;
  int numwrite;
  OggzTable *streams;
  int verbose;
  int num_serialnos;
  long serialnos[MAX_FILTER];
  int num_streamids;
  long streamids[MAX_FILTER];
  int num_content_types;
  const char *content_types[MAX_FILTER];
} ORData;

typedef struct {
  long serialno;
  int streamid;
  const char *content_type;
  int bos;
} ORStream;

typedef struct {
  const char *bos_str;
  int bos_str_len;
  const char *content_type;
} ORCodecIdent;

static int streamid_count = 0;

static const ORCodecIdent codec_ident[] = {
  {"\200theora", 7, "theora"},
  {"\001vorbis", 7, "vorbis"},
  {"Speex", 5, "speex"},
  {"Annodex", 8, "annodex"},
  {NULL}
};

static void
usage (char * progname)
{
  printf ("Usage: %s [options] filename ...\n", progname);
  printf ("\nMiscellaneous options\n");
  printf ("  -o filename, --output filename\n");
  printf ("                         Specify output filename\n");
  printf ("  -h, --help             Display this help and exit\n");
  printf ("  -v, --version          Output version information and exit\n");
  printf ("  -V                     Verbose operation\n");
  printf ("\nFilter options\n");
  printf ("  These options can be used multiple times. Pages matching ANY of\n");
  printf ("  the filter options will be included into the output.\n\n");
  printf ("  -s serialno, --serialno serialno\n");
  printf ("                         Output streams with given serialno.\n");
  printf ("  -i streamid, --streamid streamid\n");
  printf ("                         Filter by stream-ID, IDs are assigned to\n");
  printf ("                         streams in the order of their BOS pages,\n");
  printf ("                         starting at 0.\n");
  printf ("  -c content-type --content-type content-type\n");
  printf ("                         Filter by content-type.  The following codec\n");
  printf ("                         names are currently detected: \"theora\",\n");
  printf ("                         \"vorbis\", \"speex\", \"annodex\"\n");
  printf ("\n");
  printf ("Please report bugs to <ogg-dev@xiph.org>\n");
}

static ORData *
ordata_new ()
{
  ORData *ordata = malloc (sizeof (ordata));
  assert (ordata != NULL);
  memset (ordata, 0, sizeof (ORData));
  
  ordata->streams = oggz_table_new ();
  assert (ordata->streams != NULL);
  
  return ordata;
}

static void 
ordata_delete (ORData *ordata)
{
  oggz_table_delete (ordata->streams);
  
  if (ordata->reader)
    oggz_close (ordata->reader);
  if (ordata->outfile)
    fclose (ordata->outfile);
  
  free (ordata);
}

static int
filter_stream_p (const ORData *ordata, ORStream *stream, 
		 const ogg_page *og, long serialno)
{
  int i;

  for (i = 0; i < ordata->num_serialnos; i++) {
    if (ordata->serialnos[i] == serialno)
      return 1;
  }

  if (stream == NULL)
    return 0;
  
  for (i = 0; i < ordata->num_streamids; i++) {
    if (ordata->streamids[i] == stream->streamid)
      return 1;
  }

  for (i = 0; i < ordata->num_content_types; i++) {
    if (strcmp (ordata->content_types[i], stream->content_type) == 0)
      return 1;
  }

  return 0;
}

static ORStream *
orstream_new (const ORData *ordata, const ogg_page *og, long serialno)
{
  int i;

  ORStream *stream = malloc (sizeof (ORStream));
  assert (stream != NULL);

  stream->serialno = serialno;
  stream->streamid = streamid_count++;
  stream->content_type = "unknown";

  /* try to identify stream codec name by looking at the first bytes of the
   * first packet */
  for (i = 0;; i++) {
    const ORCodecIdent *ident = &codec_ident[i];
    
    if (ident->bos_str == NULL)
      break;
    if (og->body_len >= ident->bos_str_len &&
	memcmp (og->body, ident->bos_str, ident->bos_str_len) == 0) {
      stream->content_type = ident->content_type;
      break;
    }
  }
   
  if (ordata->verbose)
    fprintf (stderr, 
	     "New logical stream, serialno %li, id %i, codec %s, will be %s\n",
	     stream->serialno, stream->streamid, stream->content_type,
	     (filter_stream_p (ordata, stream, og, serialno) ? 
	      "copied" :"dropped"));

  return stream;
}

static void 
orstream_delete (ORData *ordata, ORStream *stream)
{
  if (ordata->verbose)
    fprintf (stderr, "End of logical stream %li   \n", stream->serialno);

  free (stream);
}

static void
checked_fwrite (const void *data, size_t size, size_t count, FILE *stream)
{
  int n = fwrite (data, size, count, stream);
  if (n != count) {
    perror ("write failed");
    exit (1);
  }
}

static int
read_page (OGGZ *oggz, const ogg_page *og, long serialno, void *user_data)
{
  ORData *ordata = (ORData *) user_data;
  ORStream *stream = oggz_table_lookup (ordata->streams, serialno);

  if (ogg_page_bos ((ogg_page *)og)) {
    stream = orstream_new (ordata, og, serialno);
    stream = oggz_table_insert (ordata->streams, serialno, stream);
    assert (stream != NULL);
  } else if (stream == NULL) {
    fprintf (stderr, "WARNING: found page for nonexistent stream %li\n",
	     serialno);
  }

  if (filter_stream_p (ordata, stream, og, serialno)) {
    checked_fwrite (og->header, 1, og->header_len, ordata->outfile);
    checked_fwrite (og->body, 1, og->body_len, ordata->outfile);
  }

  if (ogg_page_eos ((ogg_page *)og) && stream != NULL) {
    oggz_table_remove (ordata->streams, serialno);
    orstream_delete (ordata, stream);
  }

  return 0;
}

static int
oggz_rip (ORData * ordata)
{
  long n;

  oggz_set_read_page (ordata->reader, -1, read_page, ordata);
  
  while ((n = oggz_read (ordata->reader, READ_SIZE))) {

    if (n <= 0)
      return n;

    if (ordata->verbose) {
      fprintf (stderr, "\r Read %li k, wrote %li k ...\r",
	       (long) (oggz_tell (ordata->reader)/1024),
	       (long) (ftell (ordata->outfile)/1024));
    }
  }

  if (ordata->verbose) 
    fprintf (stderr, "\r Done.                                 \n");

  return 0;
}

static int 
or_get_long (const char *optarg, const char *currentopt,
			long *value)
{
  char *tailptr;

  *value = strtol (optarg, &tailptr, 10);

  if (*tailptr != '\0') {
    fprintf (stderr, "ERROR: non-integer argument to option `%s': %s\n",
	     currentopt, optarg);
    return -1;
  }

  return 0;
}

int
main (int argc, char * argv[]) 
{
  int show_version = 0;
  int show_help = 0;

  char * progname;
  char * infilename = NULL, * outfilename = NULL;
  FILE * infile = NULL;
  const char *currentopt = argv[1];
  ORData * ordata;
  int i;

#ifdef _WIN32
  /* We need to set stdin/stdout to binary mode */

  _setmode( _fileno( stdin ), _O_BINARY );
  _setmode( _fileno( stdout ), _O_BINARY );
#endif

  progname = argv[0];

  if (argc < 2) {
    usage (progname);
    return (1);
  }

  ordata = ordata_new ();

  while (1) {
    char * optstring = "hvVo:s:i:c:";

#ifdef HAVE_GETOPT_LONG
    static struct option long_options[] = {
      {"help", no_argument, 0, 'h'},
      {"version", no_argument, 0, 'v'},
      {"output", required_argument, 0, 'o'},
      {"verbose", no_argument, 0, 'V'},
      {"serialno", required_argument, 0, 's'},
      {"streamid", required_argument, 0, 'i'},
      {"content-type", required_argument, 0, 'c'},
      {0,0,0,0}
    };

    i = getopt_long (argc, argv, optstring, long_options, NULL);
#else
    i = getopt (argc, argv, optstring);
#endif
    if (i == -1) break;
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
    case 'o': /* output */
      outfilename = optarg;
      break;
    case 'V': /* verbose */
      ordata->verbose = 1;
      break;
    case 's': /* serialno */
      if (ordata->num_serialnos >= MAX_FILTER) {
	fprintf (stderr, "ERROR: too many serialnos on command line\n");
	goto exit_err;
      }
      if (or_get_long (optarg, currentopt, 
		       &ordata->serialnos[ordata->num_serialnos++]))
	goto exit_err;

      break;
    case 'i': /* streamid */
      if (ordata->num_streamids >= MAX_FILTER) {
	fprintf (stderr, "ERROR: too many streamids on command line\n");
	goto exit_err;
      }
      if (or_get_long (optarg, currentopt, 
		       &ordata->streamids[ordata->num_streamids++]))
	goto exit_err;
    case 'c': /* content-type */
      if (ordata->num_content_types >= MAX_FILTER) {
	fprintf (stderr, "ERROR: too many content-types on command line\n");
	goto exit_err;
      }
      ordata->content_types[ordata->num_content_types++] = optarg;	     
      break;
	  
    default:
      break;
    }

    currentopt = argv[optind];
  }

  if (show_version) {
    printf ("%s version " VERSION "\n", progname);
  }

  if (show_help) {
    usage (progname);
  }

  if (show_version || show_help) {
    goto exit_ok;
  }

  if (optind != argc-1) {
    usage (progname);
    goto exit_err;
  }

  infilename = argv[optind];
  infile = fopen (infilename, "rb");
  if (infile == NULL) {
    fprintf (stderr, "%s: unable to open input file %s : %s\n", progname,
	     infilename, strerror (errno));
    goto exit_err;
  } else {
    ordata->reader = oggz_open_stdio (infile, OGGZ_READ|OGGZ_AUTO);
  }

  if (outfilename == NULL) {
    ordata->outfile = stdout;
  } else {
    ordata->outfile = fopen (outfilename, "wb");
    if (ordata->outfile == NULL) {
      fprintf (stderr, "%s: unable to open output file %s : %s\n",
	       progname, outfilename, strerror (errno));
      goto exit_err;
    }
  }

  oggz_rip (ordata);

 exit_ok:
  ordata_delete (ordata);
  exit (0);

 exit_err:
  ordata_delete (ordata);
  exit (1);
}
