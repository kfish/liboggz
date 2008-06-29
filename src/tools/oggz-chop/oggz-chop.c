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
#include <math.h>

#include <oggz/oggz.h>

#include "oggz-chop.h"
#include "skeleton.h"

#ifdef OGG_H_CONST_CORRECT
#define OGG_PAGE_CONST(x) (x)
#else
#define OGG_PAGE_CONST(x) ((ogg_page *)x)
#endif

/************************************************************
 * OCTrackState
 */

typedef struct _OCTrackState {
  /* Skeleton track info (fisbone) */
  fisbone_packet fisbone;

  /* Page accumulator for the GOP before the chop start */
  OggzTable * page_accum;

  int headers_remaining;

  int rec_skeleton;

  /* Greatest previously inferred keyframe value */
  ogg_int64_t prev_keyframe;

} OCTrackState;

static OCTrackState *
track_state_new (void)
{
  OCTrackState * ts;

  ts = (OCTrackState *) malloc (sizeof(*ts));

  memset (ts, 0, sizeof(*ts));

  return ts;
}

static void
track_state_delete (OCTrackState * ts)
{
  if (ts == NULL) return;

  fisbone_clear (&ts->fisbone);

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

static void
state_init (OCState * state)
{
  /* Initialize fishead presentation time */
  state->fishead.ptime_n = state->start / (ogg_int64_t)1000;
  state->fishead.ptime_d = 1000;

  /* Initialize track table and page accumulator */
  state->tracks = oggz_table_new ();
  state->written_accum = 0;
}

static void
state_clear (OCState * state)
{
  int i, ntracks;

  ntracks = oggz_table_size (state->tracks);
  for (i = 0; i < ntracks; i++) {
    track_state_delete (oggz_table_nth(state->tracks, i, NULL));
  }
  oggz_table_delete (state->tracks);
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
  ogg_page_checksum_set (OGG_PAGE_CONST(og));
}

static void
fwrite_ogg_page (FILE * outfile, const ogg_page * og)
{
  if (og == NULL) return;

  fwrite (og->header, 1, og->header_len, outfile);
  fwrite (og->body, 1, og->body_len, outfile);
}

/************************************************************
 * OCPageAccum
 */

typedef struct _OCPageAccum {
  ogg_page * og;
  double time;
} OCPageAccum;

static OCPageAccum *
page_accum_new (const ogg_page * og, double time)
{
  OCPageAccum * pa;

  pa = malloc(sizeof (*pa));
  pa->og = _ogg_page_copy (og);
  pa->time = time;

  return pa;
}

static void
page_accum_delete (OCPageAccum * pa)
{
  if (pa == NULL) return;

  _ogg_page_free (pa->og);
  free (pa);
}

static void
track_state_remove_page_accum (OCTrackState * ts)
{
  int i, accum_size;

  if (ts == NULL || ts->page_accum == NULL) return;

  accum_size = oggz_table_size (ts->page_accum);

  for (i = accum_size-1; i >= 0; i--) {
    page_accum_delete((OCPageAccum *)oggz_table_lookup (ts->page_accum, i));
    oggz_table_remove (ts->page_accum, (long)i);
  }
}

/*
 * Advance the page accumulator:
 *   - Remove all pages that only contain packets from the previous GOP.
 *   - Shift all remaining packets to start from index 0
 * Returns: the new size of the page accumulator.
 */
static int
track_state_advance_page_accum (OCTrackState * ts)
{
  int i, accum_size;
  int e, earliest_new; /* Index into page accumulator of the earliest page that
                        * contains a packet from the new GOP */
  OCPageAccum * pa;

  if (ts == NULL || ts->page_accum == NULL) return 0;

  earliest_new = accum_size = oggz_table_size (ts->page_accum);

  /* Working backwards through the page accumulator ... */
  for (i = accum_size-1; i >= 0; i--) {
    pa = (OCPageAccum *) oggz_table_lookup (ts->page_accum, i);

    /* If we have a page with granulepos, it necessarily contains the end
     * of a packet from an earlier GOP, and thus this is the last page that
     * we need to recover.
     */
    if (ogg_page_granulepos (pa->og) != -1) {
      earliest_new = i;
      break;
    }
  }

  /* If all accumulated pages have no granulepos, keep them,
   * and do not modify start_granule */
  if (earliest_new == 0) return accum_size;

  if (earliest_new > accum_size)
    earliest_new = accum_size;

  /* Delete the rest */
  for (i = earliest_new-1; i >= 0; i--) {
    pa = (OCPageAccum *) oggz_table_lookup (ts->page_accum, i);
    /* Record this track's start_granule as the granulepos of the page prior
     * to earliest_new */
    if (i == (earliest_new-1)) {
      ts->fisbone.start_granule = ogg_page_granulepos (pa->og);
    }
    page_accum_delete(pa);
    oggz_table_remove (ts->page_accum, (long)i);
  }

  /* Shift the best */
  for (i = 0, e = earliest_new; e < accum_size; i++, e++) {
    pa = (OCPageAccum *) oggz_table_lookup (ts->page_accum, e);
    oggz_table_insert (ts->page_accum, i, pa);
    oggz_table_remove (ts->page_accum, (long)e);
  }

  return accum_size - earliest_new;
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
  long gp;

  ts = oggz_table_lookup (state->tracks, serialno);

  page_time = oggz_tell_units (oggz) / 1000.0;

#ifdef DEBUG
  printf ("page_time: %g\tspan (%g, %g)\n", page_time, state->start, state->end);
  printf ("\tpageno: %d, numheaders %d\n", ogg_page_pageno(og),
          oggz_stream_get_numheaders (oggz, serialno));
#endif

  if (page_time < state->start) {
    if ((gp = ogg_page_granulepos (og)) != -1)
      ts->fisbone.start_granule = ogg_page_granulepos (og);
  } else if (page_time >= state->start &&
      (state->end == -1 || page_time <= state->end)) {
    if (!ts->rec_skeleton) {
#ifdef DEBUG
      printf ("read_plain: start granule for serialno %010ld: %ld\n",
              serialno, ts->fisbone.start_granule);
#endif
      ts->rec_skeleton = 1;
    }
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

static int
write_accum (OCState * state)
{
  OCTrackState * ts;
  OCPageAccum * pa;
  OggzTable * candidates;
  long serialno, min_serialno;
  int i, ntracks, ncandidates=0, remaining=0, min_i, cn, min_cn;
  ogg_page * og, * min_og;
  double min_time;

  if (state->written_accum) return -1;

  /*
   * We create a table of candidate tracks, which are all those which
   * have a page_accum table, ie. for which granuleshift matters.
   * We insert into this table the index of the next page_accum element
   * to be merged for that track. All start at 0.
   * The variable 'remaining' counts down the total number of accumulated
   * pages to be written from all candidate tracks.
   */

  /* Prime candidates */
  candidates = oggz_table_new ();
  ntracks = oggz_table_size (state->tracks);
  for (i=0; i < ntracks; i++) {
    ts = oggz_table_nth (state->tracks, i, &serialno);
    if (ts != NULL && ts->page_accum != NULL) {
      ncandidates++;
      /* XXX: we offset the candidate index by 7 to avoid storing 0, as
       * OggzTable treats insertion of NULL as a deletion */
      oggz_table_insert (candidates, serialno, (void *)0x7);
      remaining += oggz_table_size (ts->page_accum);
    }
  }

  /* Merge candidates */
  while (remaining > 0) {
    /* Find minimum page in all accum buffers */
    min_time = 10e100;
    min_cn = -1;
    min_og = NULL;
    min_serialno = -1;
    for (i=0; i < ncandidates; i++) {
      cn = ((int) oggz_table_nth (candidates, i, &serialno)) - 7;
      ts = oggz_table_lookup (state->tracks, serialno);
      if (ts && ts->page_accum) {
        if (cn < oggz_table_size (ts->page_accum)) {
          pa = oggz_table_nth (ts->page_accum, cn, NULL);

          if (pa->time < min_time) {
            min_i = i;
            min_cn = cn;
            min_og = pa->og;
            min_serialno = serialno;
            min_time = pa->time;
          }
        }
      }
    }

    if (min_og) {
      /* Increment index for minimum page */
      oggz_table_insert (candidates, min_serialno, (void *)(min_cn+1+7));

      /* Write out minimum page */
      fwrite_ogg_page (state->outfile, min_og);
    }

    remaining--;
  }

  /* Cleanup */
  for (i=0; i < ntracks; i++) {
    ts = oggz_table_nth (state->tracks, i, &serialno);
    track_state_remove_page_accum (ts);
  }

  oggz_table_delete (candidates);

  state->written_accum = 1;
 
  return 0;
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
  OCPageAccum * pa;
  double page_time;
  ogg_int64_t granulepos, keyframe;
  int granuleshift, i, accum_size;
  ogg_page * accum_og;

  page_time = oggz_tell_units (oggz) / 1000.0;

  ts = oggz_table_lookup (state->tracks, serialno);
  accum_size = oggz_table_size (ts->page_accum);

  if (page_time >= state->start) {
    /* Write out accumulated pages */
    write_accum (state);

    if (!ts->rec_skeleton) {
#ifdef DEBUG
      printf ("read_gs: start granule for serialno %010ld: %ld\n",
              serialno, ts->fisbone.start_granule);
#endif
      ts->rec_skeleton = 1;
    }

    /* Switch to the plain page reader */
    oggz_set_read_page (oggz, serialno, read_plain, state);
    return read_plain (oggz, og, serialno, user_data);
  } /* else { ... */

  granulepos = ogg_page_granulepos (OGG_PAGE_CONST(og));
  if (granulepos != -1) {
    granuleshift = oggz_get_granuleshift (oggz, serialno);
    keyframe = granulepos >> granuleshift;

    if (keyframe != ts->prev_keyframe) {
      if (ogg_page_continued(og)) {
        /* If this new-keyframe page is continued, advance the page accumulator,
         * ie. recover earlier pages from this new GOP */
        accum_size = track_state_advance_page_accum (ts);
      } else {
        /* Otherwise, just clear the page accumulator */
        track_state_remove_page_accum (ts);
        accum_size = 0;
      }

      /* Record this as prev_keyframe */
      ts->prev_keyframe = keyframe;
    }
  }

  /* Add a copy of this to the page accumulator */
  pa = page_accum_new (og, page_time);
  oggz_table_insert (ts->page_accum, accum_size, pa);

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
  int content_type;
  fisbone_packet fisbone;

  fwrite_ogg_page (state->outfile, og);

  content_type = oggz_stream_get_content(oggz, serialno);
  if(content_type == OGGZ_CONTENT_SKELETON) {
    if (fisbone_from_ogg_page (og, &fisbone) != -1) {
      if ((ts = oggz_table_lookup (state->tracks, fisbone.serial_no)) != NULL) {
        ts->fisbone.current_header_size = fisbone.current_header_size;
        ts->fisbone.message_header_fields = fisbone.message_header_fields;
      }
    }
  } else {
    ts = oggz_table_lookup (state->tracks, serialno);
    ts->headers_remaining -= ogg_page_packets (OGG_PAGE_CONST(og));

    if (ts->headers_remaining <= 0) {
      if (state->start == 0.0 || oggz_get_granuleshift (oggz, serialno) == 0) {
        oggz_set_read_page (oggz, serialno, read_plain, state);
      } else {
        ts->page_accum = oggz_table_new();
        oggz_set_read_page (oggz, serialno, read_gs, state);
      }
    }
  }

  return OGGZ_CONTINUE;
}

static int
fisbone_init (OGGZ * oggz, OCState * state, OCTrackState * ts, long serialno)
{
  const char * content_type;
  int len;

  if (ts == NULL) return -1;

  ts->fisbone.serial_no = serialno;
  ts->fisbone.nr_header_packet = oggz_stream_get_numheaders (oggz, serialno);
  oggz_get_granulerate (oggz, serialno, &ts->fisbone.granule_rate_n, &ts->fisbone.granule_rate_d);
  ts->fisbone.start_granule = 0;
  ts->fisbone.preroll = 0;
  ts->fisbone.granule_shift = (unsigned char) oggz_get_granuleshift (oggz, serialno);
  if (state->original_had_skeleton) {
    /* Wait, and copy over message headers from original */
    ts->fisbone.message_header_fields = NULL;
    ts->fisbone.current_header_size = FISBONE_SIZE;
  } else {
    /* XXX: C99 */
#define CONTENT_TYPE_FMT "Content-Type: %s\r\n"
    content_type = oggz_stream_get_content_type (oggz, serialno);
    len = snprintf (NULL, 0, CONTENT_TYPE_FMT, content_type);
    ts->fisbone.message_header_fields = malloc(len+1);
    snprintf (ts->fisbone.message_header_fields, len+1, CONTENT_TYPE_FMT, content_type);
    ts->fisbone.current_header_size = len+1;
  }

  return 0;
}

static int
fishead_update (OCState * state, const ogg_page * og)
{
  fishead_packet fishead;

  fishead_from_ogg_page (og, &fishead);
  state->fishead.btime_n = fishead.btime_n;
  state->fishead.btime_d = fishead.btime_d;

  return 0;
}

static int
read_bos (OGGZ * oggz, const ogg_page * og, long serialno, void * user_data)
{
  OCState * state = (OCState *)user_data;
  OCTrackState * ts;
  double page_time;
  int content_type;

  if (ogg_page_bos (OGG_PAGE_CONST(og))) {
    content_type = oggz_stream_get_content(oggz, serialno);
    if(content_type == OGGZ_CONTENT_SKELETON) {
      state->original_had_skeleton = 1;
      fishead_update (state, og);
    } else {
      ts = track_state_add (state->tracks, serialno);
      fisbone_init (oggz, state, ts, serialno);
      ts->headers_remaining = ts->fisbone.nr_header_packet;
    }

    oggz_set_read_page (oggz, serialno, read_headers, state);
    read_headers (oggz, og, serialno, user_data);
  } else {
    /* Deregister the catch-all page reading callback */
    oggz_set_read_page (oggz, -1, NULL, NULL);
  }

  return OGGZ_CONTINUE;
}

int
chop (OCState * state)
{
  OGGZ * oggz;

  state_init (state);

  if (strcmp (state->infilename, "-") == 0) {
    oggz = oggz_open_stdio (stdin, OGGZ_READ|OGGZ_AUTO);
  } else {
    oggz = oggz_open (state->infilename, OGGZ_READ|OGGZ_AUTO);
  }

  if (state->outfilename == NULL) {
    state->outfile = stdout;
  } else {
    state->outfile = fopen (state->outfilename, "wb");
    if (state->outfile == NULL) {
      fprintf (stderr, "oggz-chop: unable to open output file %s\n",
	       state->outfilename);
      return -1;
    }
  }

  /* set up a demux filter */
  oggz_set_read_page (oggz, -1, read_bos, state);

  oggz_run_set_blocksize (oggz, 1024*1024);
  oggz_run (oggz);

  oggz_close (oggz);

  if (state->outfilename != NULL) {
    fclose (state->outfile);
  }

  state_clear (state);

  return 0; 
}
