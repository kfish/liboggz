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

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <getopt.h>
#include <errno.h>

#include <oggz/oggz.h>

#define READ_SIZE 4096

static int
read_page (OGGZ * oggz, const ogg_page * og, void * user_data);

static void
usage (char * progname)
{
  printf ("Usage: %s [options] filename\n", progname);
}

typedef struct _OMData OMData;
typedef struct _OMInput OMInput;
typedef struct _OMITrack OMITrack;

struct _OMData {
  OggzTable * inputs;
};

struct _OMInput {
  OMData * omdata;
  OGGZ * reader;
  const ogg_page * og;
};

struct _OMITrack {
  long output_serialno;
};

static ogg_page *
_ogg_page_copy (ogg_page * og)
{
  ogg_page * new_og;

  new_og = malloc (sizeof (*og));
  new_og->header = malloc (og->header_len);
  new_og->header_len = og->header_len;
  memcpy (new_og->header, og->header, og->header_len);
  new_og->body = malloc (og->body_len);
  new_og->body_len = og->body_len;
  memcpy (new_og->body, og->body, og->body_len);

  return new_og;
}

static int
_ogg_page_free (ogg_page * og)
{
  free (og->header);
  free (og->body);
  free (og);
  return 0;
}

static void
ominput_delete (OMInput * input)
{
  oggz_close (input->reader);

  free (input);
}

static OMData *
omdata_new (void)
{
  OMData * omdata;

  omdata = (OMData *) malloc (sizeof (OMData));

  omdata->inputs = oggz_table_new ();

  return omdata;
}

static void
omdata_delete (OMData * omdata)
{
  OMInput * input;
  int i, ninputs;

  ninputs = oggz_table_size (omdata->inputs);
  for (i = 0; i < ninputs; i++) {
    input = (OMInput *) oggz_table_nth (omdata->inputs, i, NULL);
    ominput_delete (input);
  }
  oggz_table_delete (omdata->inputs);

  free (omdata);
}

static int
omdata_add_input (OMData * omdata, FILE * infile)
{
  OMInput * input;
  int nfiles;

  input = (OMInput *) malloc (sizeof (OMInput));
  if (input == NULL) return -1;

  input->omdata = omdata;
  input->reader = oggz_open_stdio (infile, OGGZ_READ|OGGZ_AUTO);
  input->og = NULL;

  oggz_set_read_page (input->reader, read_page, input);

  nfiles = oggz_table_size (omdata->inputs);
  if (!oggz_table_insert (omdata->inputs, nfiles++, input)) {
    ominput_delete (input);
    return -1;
  }

  return 0;
}

static int
read_page (OGGZ * oggz, const ogg_page * og, void * user_data)
{
  OMInput * input = (OMInput *) user_data;

  input->og = _ogg_page_copy (og);

  return OGGZ_STOP_OK;
}

static int
oggz_merge (OMData * omdata, FILE * outfile)
{
  unsigned char buf[READ_SIZE];
  OMInput * input;
  int ninputs, i, min_i;
  long key, n;
  ogg_int64_t units, min_units;
  const ogg_page * og;
  int active;

  while ((ninputs = oggz_table_size (omdata->inputs)) > 0) {
    min_units = -1;
    min_i = -1;
    active = 1;

    /* Reload all pages, and find the min (earliest) */
    for (i = 0; active && i < oggz_table_size (omdata->inputs); i++) {
      input = (OMInput *) oggz_table_nth (omdata->inputs, i, &key);
      if (input != NULL) {
	if (input->og == NULL) {
	  n = oggz_read (input->reader, READ_SIZE);
	  if (n == 0) {
	    oggz_table_remove (omdata->inputs, key);
	    ominput_delete (input);
	    input = NULL;
	  }
	}
	if (input && input->og) {
	  if (ogg_page_bos (input->og)) {
	    min_i = i;
	    active = 0;
	  }
	  units = oggz_tell_units (input->reader);
	  printf ("cmp units %lld < %lld ?\n", units, min_units);
	  if (min_units == -1 || units == 0 ||
	      (units > -1 && units < min_units)) {
	    min_units = units;
	    min_i = i;
	  }
	}
      }
    }

    /* Write the earliest page */
    if (min_i != -1) {
      input = (OMInput *) oggz_table_nth (omdata->inputs, min_i, &key);
      og = input->og;
      fwrite (og->header, 1, og->header_len, outfile);
      fwrite (og->body, 1, og->body_len, outfile);

      _ogg_page_free (og);
      input->og = NULL;
    }
  }

  return 0;
}

int
main (int argc, char * argv[])
{
  char * progname;
  char * infilename = NULL, * outfilename = NULL;
  FILE * infile = NULL, * outfile = NULL;
  OMData * omdata;
  int i;

  progname = argv[0];

  if (argc < 2) {
    usage (progname);
    return (1);
  }

  omdata = omdata_new();

  while (1) {
    char * optstring = "ho:";

#ifdef HAVE_GETOPT_LONG
    static struct option long_options[] = {
      {"help", no_argument, 0, 'h'},
      {"output", required_argument, 0, 'o'},
      {0,0,0,0}
    };

    i = getopt_long (argc, argv, optstring, long_options, NULL);
#else
    i = getopt (argc, argv, optstring);
#endif
    if (i == -1) break;
    if (i == ':') {
      usage (progname);
      omdata_delete (omdata);
      return (1);
    }

    switch (i) {
    case 'h': /* help */
      usage (progname);
      omdata_delete (omdata);
      return (0);
      break;
    case 'o': /* output */
      outfilename = optarg;
      break;
    default:
      printf ("Random option %c\n", i);
      break;
    }
  }

  if (optind >= argc) {
    usage (progname);
    omdata_delete (omdata);
    return (1);
  }

  while (optind < argc) {
    infilename = argv[optind++];
    infile = fopen (infilename, "rb");
    if (infile == NULL) {
      fprintf (stderr, "%s: unable to open input file %s\n", progname,
	       infilename);
    } else {
      omdata_add_input (omdata, infile);
    }
  }

  if (outfilename == NULL) {
    outfile = stdout;
  } else {
    outfile = fopen (outfilename, "wb");
    if (outfile == NULL) {
      fprintf (stderr, "%s: unable to open output file %s\n",
	       progname, outfilename);
      omdata_delete (omdata);
      return (1);
    }
  }

  oggz_merge (omdata, outfile);

  omdata_delete (omdata);

  return (0);
}