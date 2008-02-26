
/*
   Copyright (C) 2008 Annodex Association

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

   - Neither the name of the Annodex Association nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
   PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE ASSOCIATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>


#include <oggz/oggz.h>

static char * progname;

static void
usage (char * progname)
{
  printf ("Usage: %s [options] filename\n", progname);
  printf ("Chop an Ogg file.\n");
  printf ("\nOutput options\n");
  printf ("  -o filename, --output filename\n");
  printf ("                         Specify output filename\n");
  printf ("  -s start_time, --start start_time\n");
  printf ("                         Specify start time\n");
  printf ("  -e end_time, --end end_time\n");
  printf ("                         Specify end time\n");
  printf ("\nMiscellaneous options\n");
  printf ("  -h, --help             Display this help and exit\n");
  printf ("  -v, --version          Output version information and exit\n");
  printf ("\n");
  printf ("Please report bugs to <ogg-dev@xiph.org>\n");
}

/************************************************************
 * OCState
 */

typedef struct _OCState {
  OggzTable * tracks;
  FILE * outfile;
  double start;
  double end;
} OCState;

/************************************************************
 * OCTrackState
 */

typedef struct _OCTrackState {
  OggzTable * page_accum;

  int headers_remaining;

  long start_granule;

  /* Greatest previously inferred keyframe value */
  ogg_int64_t prev_keyframe;

} OCTrackState;

static OCTrackState *
track_state_new (void)
{
  OCTrackState * ts;

  ts = (OCTrackState *) malloc (sizeof(*ts));

  memset (ts, 0, sizeof(*ts));
  ts->page_accum = oggz_table_new();

  return ts;
}

static void
track_state_delete (OCTrackState * ts)
{
  if (ts == NULL) return;

  /* XXX: delete accumulated pages */
  oggz_table_delete (ts->page_accum);

  free (ts);

  return;
}

/* Add a track to the overall state */
static OCTrackState *
track_state_add (OggzTable * state, long serialno)
{
  OCTrackState * ts;

  ts = track_state_new ();
  if (oggz_table_insert (state, serialno, ts) == ts) {
    return ts;
  } else {
    track_state_delete (ts);
    return NULL;
  }
}

/************************************************************
 * ogg_page helpers
 */

static ogg_page *
_ogg_page_copy (const ogg_page * og)
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

static void
_ogg_page_free (const ogg_page * og)
{
  if (og == NULL) return;

  free (og->header);
  free (og->body);
  free ((ogg_page *)og);
}

static void
_ogg_page_set_eos (const ogg_page * og)
{
  if (og == NULL) return;

  og->header[5] |= 0x04;
  ogg_page_checksum_set (og);
}

static void
fwrite_ogg_page (FILE * outfile, ogg_page * og)
{
  if (og == NULL) return;

  fwrite (og->header, 1, og->header_len, outfile);
  fwrite (og->body, 1, og->body_len, outfile);
}

/************************************************************
 * chop
 */

/*
 * OggzReadPageCallback read_plain
 *
 * A page reading callback for tracks without granuleshift.
 */
static int
read_plain (OGGZ * oggz, const ogg_page * og, long serialno, void * user_data)
{
  OCState * state = (OCState *)user_data;
  OCTrackState * ts;
  double page_time;

  ts = oggz_table_lookup (state->tracks, serialno);

  page_time = oggz_tell_units (oggz) / 1000.0;

#if 0
  printf ("page_time: %g\tspan (%g, %g)\n", page_time, state->start, state->end);
  printf ("\tpageno: %d, numheaders %d\n", ogg_page_pageno(og),
          oggz_stream_get_numheaders (oggz, serialno));
#endif

  if (page_time >= state->start &&
      (state->end == -1 || page_time <= state->end)) {
    fwrite_ogg_page (state->outfile, og);
  } else if (state->end != -1.0 && page_time > state->end) {
    /* This is the first page past the end time; set EOS */
    _ogg_page_set_eos (og);
    fwrite_ogg_page (state->outfile, og);

    /* Stop handling this track */
    oggz_set_read_page (oggz, serialno, NULL, NULL);
  }

  return OGGZ_CONTINUE;
}

/*
 * OggzReadPageCallback read_gs
 *
 * A page reading callback for tracks with granuleshift.
 */
static int
read_gs (OGGZ * oggz, const ogg_page * og, long serialno, void * user_data)
{
  OCState * state = (OCState *)user_data;
  OCTrackState * ts;
  double page_time;
  ogg_int64_t granulepos, keyframe;
  int granuleshift, i, accum_size;
  ogg_page * accum_og;

  page_time = oggz_tell_units (oggz) / 1000.0;

  ts = oggz_table_lookup (state->tracks, serialno);
  accum_size = oggz_table_size (ts->page_accum);

  if (page_time >= state->start) {
    /* Write out accumulated pages */
    for (i = 0; i < accum_size; i++) {
      accum_og = (ogg_page *)oggz_table_lookup (ts->page_accum, i);
      fwrite_ogg_page (state->outfile, accum_og);
      _ogg_page_free (accum_og);
    }
    oggz_table_delete (ts->page_accum);
    ts->page_accum = NULL;

    /* Switch to the plain page reader */
    oggz_set_read_page (oggz, serialno, read_plain, state);
    return read_plain (oggz, og, serialno, user_data);
  } /* else { ... */

  granulepos = ogg_page_granulepos (og);
  if (granulepos != -1) {
    granuleshift = oggz_get_granuleshift (oggz, serialno);
    keyframe = granulepos >> granuleshift;

    if (keyframe != ts->prev_keyframe) {
      /* Clear the page accumulator */
      for (i = accum_size; i >= 0; i--) {
        _ogg_page_free ((ogg_page *)oggz_table_lookup (ts->page_accum, i));
        oggz_table_remove (ts->page_accum, (long)i);
      }
      accum_size = 0;

      /* Record this as prev_keyframe */
      ts->prev_keyframe = keyframe;
    }
  }

  /* Add a copy of this to the page accumulator */
  oggz_table_insert (ts->page_accum, accum_size, _ogg_page_copy (og));

  return OGGZ_CONTINUE;
}

/*
 * OggzReadPageCallback read_headers
 *
 * A page reading callback for header pages
 */
static int
read_headers (OGGZ * oggz, const ogg_page * og, long serialno, void * user_data)
{
  OCState * state = (OCState *)user_data;
  OCTrackState * ts;

  fwrite_ogg_page (state->outfile, og);

  ts = oggz_table_lookup (state->tracks, serialno);
  ts->headers_remaining -= ogg_page_packets (og);

  if (ts->headers_remaining <= 0) {
    if (state->start == 0.0 || oggz_get_granuleshift (oggz, serialno) == 0) {
      oggz_set_read_page (oggz, serialno, read_plain, state);
    } else {
      oggz_set_read_page (oggz, serialno, read_gs, state);
    }
  }

  return OGGZ_CONTINUE;
}

static int
read_bos (OGGZ * oggz, const ogg_page * og, long serialno, void * user_data)
{
  OCState * state = (OCState *)user_data;
  OCTrackState * ts;
  double page_time;

  if (ogg_page_bos (og)) {
    ts = track_state_add (state->tracks, serialno);
    ts->headers_remaining = oggz_stream_get_numheaders (oggz, serialno);

    oggz_set_read_page (oggz, serialno, read_headers, state);
    read_headers (oggz, og, serialno, user_data);
  } else {
    /* Deregister the catch-all page reading callback */
    oggz_set_read_page (oggz, -1, NULL, NULL);
  }

  return OGGZ_CONTINUE;
}

static int
chop (char * infilename, char * outfilename, double start, double end)
{
  OCState state;
  OGGZ * oggz;

  state.tracks = oggz_table_new ();

  if (strcmp (infilename, "-") == 0) {
    oggz = oggz_open_stdio (stdin, OGGZ_READ|OGGZ_AUTO);
  } else {
    oggz = oggz_open (infilename, OGGZ_READ|OGGZ_AUTO);
  }

  if (outfilename == NULL) {
    state.outfile = stdout;
  } else {
    state.outfile = fopen (outfilename, "wb");
    if (state.outfile == NULL) {
      fprintf (stderr, "%s: unable to open output file %s\n",
	       progname, outfilename);
      return -1;
    }
  }

  state.start = start;
  state.end = end;

  /* set up a demux filter */
  oggz_set_read_page (oggz, -1, read_bos, &state);

  oggz_run_set_blocksize (oggz, 1024*1024);
  oggz_run (oggz);

  oggz_close (oggz);

  return 0; 
}

int
main (int argc, char * argv[])
{
  int show_version = 0;
  int show_help = 0;
  double start = 0.0, end = -1.0;
  char * infilename = NULL, * outfilename = NULL;
  int i;

  progname = argv[0];

  if (argc < 2) {
    usage (progname);
    return (1);
  }

  while (1) {
    char * optstring = "s:e:o:hv";

#ifdef HAVE_GETOPT_LONG
    static struct option long_options[] = {
      {"output",   required_argument, 0, 'o'},
      {"format",   required_argument, 0, 'f'},
      {"keyframe", no_argument, 0, 'k'},
      {"help",     no_argument, 0, 'h'},
      {"version",  no_argument, 0, 'v'},
      {0,0,0,0}
    };

    i = getopt_long(argc, argv, optstring, long_options, NULL);
#else
    i = getopt (argc, argv, optstring);
#endif
    if (i == -1) break;
    if (i == ':') {
      usage (progname);
      goto exit_err;
    }

    switch (i) {
    case 's': /* start */
      start = atof (optarg);
      break;
    case 'e': /* end */
      end = atof (optarg);
      break;
    case 'h': /* help */
      show_help = 1;
      break;
    case 'v': /* version */
      show_version = 1;
      break;
    case 'o': /* output */
      outfilename = optarg;
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
    goto exit_ok;
  }

  if (optind >= argc) {
    usage (progname);
    goto exit_err;
  }

  infilename = argv[optind++];

  return chop (infilename, outfilename, start, end);

exit_ok:
  return 0;

exit_err:
  return 1;
}
