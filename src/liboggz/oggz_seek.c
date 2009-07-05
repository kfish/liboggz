/*
   Copyright (C) 2009 Annodex Association

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

/*
 * oggz_seek.c
 *
 * Refer to http://wiki.xiph.org/index.php/Seeking for an overview of the
 * algorithm for seeking on Ogg.
 *
 * Define seek to mean: for each logical bitstream, locate the
 * bytewise-latest page in the bitstream with a time < the target
 * time, then choose the bytewise-earliest among these pages. Thus
 * if two pages have the same time, seeking will locate the
 * bytewise-earlier page. 
 *
 * Conrad Parker <conrad@annodex.net>
 */

#include "config.h"

#if OGGZ_CONFIG_READ

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <errno.h>

#include <ogg/ogg.h>

#include "oggz_compat.h"
#include "oggz_private.h"
#include "oggz/oggz_packet.h"
#include "oggz/oggz_table.h"

/*#define DEBUG*/
/*#define DEBUG_LEVEL 2*/
#include "debug.h"

/************************************************************
 * OggzSeekInfo
 */

typedef struct _OggzSeekInfo OggzSeekInfo;

struct _OggzSeekTrack {
  /* Offset of bytewise-latest page with time < target */
  oggz_off_t latest_page_offset;
};

struct OggzSeekCache {
  time_t mtime;
  off_t size;
  oggz_off_t last_page_offset;
  ogg_int64_t unit_end;
};

struct _OggzSeekInfo {
  OGGZ * oggz;

  struct OggzSeekCache cache;

  /* page_next cache */
  size_t current_page_bytes;

  /* Target of current seek */
  ogg_int64_t unit_target;

  /* Current offset */
  oggz_off_t offset_at;

  /** Bounds of current seek */

  /* Seek bound min; Latest begin_page that is before */
  oggz_off_t offset_begin;

  /* Earliest known begin_page that is after */
  oggz_off_t offset_end;

  /* Seek bound max */
  oggz_off_t offset_max;

  /* */
  ogg_int64_t unit_at;
  ogg_int64_t unit_begin;
  ogg_int64_t unit_end;

  ogg_page og_at;

  /* State for guess */
  int prev_guess_was_zoom;

  OggzTable * tracks;
};

#define seek_info_dump(si) \
  debug_printf (3, "Got offset_begin 0x%08llx, offset_end 0x%08llx offset_max 0x%08llx", \
                si->offset_begin, si->offset_end, si->offset_max); \
  debug_printf (3, "Got unit_begin %lld, unit_end %lld", si->unit_begin, si->unit_end); \
  debug_printf (3, "At offset 0x%08llx, unit %lld", si->offset_at, si->unit_at); \
  debug_printf (3, "For unit_target %lld", si->unit_target);

#define NOT_FOUND_WITHIN_BOUNDS (-2)

/************************************************************
 * Primitives
 */

#define PAGESIZE 4096

/*
 * Raw seek
 */

static int
oggz_seek_reset_stream(void *data) {
  oggz_stream_t * stream = (oggz_stream_t *) data;

  ogg_stream_reset (&stream->ogg_stream);
  stream->last_granulepos = -1L;

  return 0;
}

static int
oggz_reset (OGGZ * oggz)
{
  OggzReader * reader = &oggz->x.reader;
  oggz_off_t offset_at;

  offset_at = oggz->offset = oggz_io_tell (oggz);

  ogg_sync_reset (&reader->ogg_sync);

  oggz_vector_foreach(oggz->streams, oggz_seek_reset_stream);
  
  /* Reset page reader state */
  reader->current_serialno = -1;
  reader->current_page_bytes = 0;

  reader->current_packet_pages = 0;
  reader->current_packet_begin_page_offset = oggz->offset;
  reader->current_packet_begin_segment_index = 1;

  return offset_at;
}

/* XXX: Should this take a seek_info? need to keep seek_info->offset_at in sync
 * with oggz->offset, on API entry (before work) and on return.
 */
static oggz_off_t
oggz_seek_raw (OGGZ * oggz, oggz_off_t offset, int whence)
{
  if (oggz_io_seek (oggz, offset, whence) == -1) {
    debug_printf (1, "oggz_io_seek() returned -1");
    return -1;
  }

  return oggz_reset (oggz);
}

/*
 * Scan forwards to the next Ogg page boundary, >= the current
 * position, and load that page.
 * \returns Offset of next page
 * \retval -1 Error
 * \retval -2 Not found within bounds
 */
static off_t
page_next (OggzSeekInfo * seek_info)
{
  OGGZ * oggz = seek_info->oggz;
  ogg_page * og = &seek_info->og_at;
  OggzReader * reader;
  long serialno;
  ogg_int64_t granulepos;
  char * buffer;
  long bytes = 0, more;
  oggz_off_t remaining, read_size;
  oggz_off_t ret;
  int found = 0;

  debug_printf (3, "IN");
  seek_info_dump (seek_info);

  reader = &oggz->x.reader;

  remaining = seek_info->offset_max - seek_info->offset_at;
  if (remaining < seek_info->current_page_bytes) {
    debug_printf (2, "remaining 0x%08llx < current_page_bytes 0x%08llx",
                  remaining, seek_info->current_page_bytes);
    return -1;
  }

  seek_info->offset_at += seek_info->current_page_bytes;
  seek_info->current_page_bytes = 0;

  debug_printf (3, "Updated offset_at ...");
  seek_info_dump (seek_info);

  do {
    remaining = seek_info->offset_max - seek_info->offset_at;
    more = ogg_sync_pageseek (&reader->ogg_sync, og);

    if (more == 0) {
      read_size = MIN(PAGESIZE, remaining);
      buffer = ogg_sync_buffer (&reader->ogg_sync, read_size);
      if ((bytes = (long) oggz_io_read (oggz, buffer, read_size)) == 0) {
	if (oggz->file && feof (oggz->file)) {
	  debug_printf (2, "feof (oggz->file), returning NOT_FOUND_WITHIN_BOUNDS");
	  clearerr (oggz->file);
	  ret = NOT_FOUND_WITHIN_BOUNDS;
          goto page_next_fail;
	}
      }
      if (bytes == OGGZ_ERR_SYSTEM) {
	  /*oggz_set_error (oggz, OGGZ_ERR_SYSTEM);*/
	  ret = -1;
          goto page_next_fail;
      }

      if (bytes == 0) {
	debug_printf (2, "bytes == 0, returning NOT_FOUND_WITHIN_BOUNDS");
	ret = NOT_FOUND_WITHIN_BOUNDS;
        goto page_next_fail;
      }

      ogg_sync_wrote(&reader->ogg_sync, bytes);

    } else if (more < 0) {
      debug_printf (3, "skipped %ld bytes", -more);
      seek_info->offset_at += (-more);
    } else {
      debug_printf (3, "page has %ld bytes", more);
      seek_info->current_page_bytes = more;
      found = 1;
    }

  } while (!found && remaining > 0);

  if (remaining == 0) {
    debug_printf (2, "remaining 0, got only page?");
  }

  ret = seek_info->offset_at;

page_next_ok:

  serialno = ogg_page_serialno (og);
  granulepos = ogg_page_granulepos (og);
  seek_info->unit_at = oggz_get_unit (oggz, serialno, granulepos);

  debug_printf (3, "ok: serialno %010ld, granulepos %lld", serialno, granulepos);

  seek_info_dump (seek_info);

  return ret;

page_next_fail:

  debug_printf (2, "No page within bounds");
  seek_info_dump (seek_info);

  /* XXX: reset to oggz->offset etc. */
  oggz_io_seek (oggz, oggz->offset, SEEK_SET);

  return ret;
}

/*
 * Return values as for page_next()
 */
static oggz_off_t
page_at_or_after (OggzSeekInfo * seek_info, oggz_off_t offset)
{
  OGGZ * oggz = seek_info->oggz;
  oggz_off_t ret;

  debug_printf (3, "IN: offset 0x%08llx", offset);

  seek_info->offset_at = oggz_seek_raw (oggz, offset, SEEK_SET);
  seek_info->unit_at = -1;
  seek_info->current_page_bytes = 0;

  ret =  page_next (seek_info);

  debug_printf (3, "OUT: wanted offset 0x%08llx, got 0x%08llx", offset, ret);

  return ret;
}

/*
 * Seek to the given offset, and set up the reader to deliver the
 * first packet begininning on the page of that offset.
 */
static oggz_off_t
packet_next (OggzSeekInfo * seek_info, oggz_off_t offset)
{
  OGGZ * oggz = seek_info->oggz;
  OggzReader * reader;
  oggz_off_t ret;
  ogg_page * og;
  long serialno;
  oggz_stream_t * stream;
  ogg_stream_state * os;

  debug_printf (3, "IN, offset 0x%08llx", offset);

  ret = page_at_or_after (seek_info, offset);

  og = &seek_info->og_at;
  
  serialno = ogg_page_serialno (og);

  /* Load the page into the ogg_stream */
  stream = oggz_get_stream (oggz, serialno);
  os = &stream->ogg_stream;
  ogg_stream_pagein(os, og);

  reader = &oggz->x.reader;

  reader->current_serialno = serialno;
  reader->current_page_bytes = seek_info->current_page_bytes;

  /* XXX: reader->current_granulepos = ?? */
  reader->current_packet_pages = 1;
  reader->current_packet_begin_page_offset = ret;

  /* If this page is continued, we will not deliver the first segment */
  reader->expect_hole = 0;
  reader->current_packet_begin_segment_index = ogg_page_continued(og);

  debug_printf (2, "begin_seg... %d", reader->current_packet_begin_segment_index);

#if 0
  /* Reset the ogg_sync state, so that this page is loaded again by oggz_read */
  ogg_sync_reset (&reader->ogg_sync);
  oggz_vector_foreach(oggz->streams, oggz_seek_reset_stream);
#endif

  reader->position_ready = OGGZ_POSITION_BEGIN;

  return ret;
}

/************************************************************
 * Cache update
 */

/*
 * Find the last page which has a granulepos.
 * Update its offset and unit in our cache.
 */
static int
update_last_page (OggzSeekInfo * seek_info)
{
  OGGZ * oggz = seek_info->oggz;
  OggzReader * reader;

  reader = &oggz->x.reader;

  if (oggz_io_seek (seek_info->oggz, -4096, SEEK_END) == -1) {
    printf ("%s: oggz_io_seek() returned -1\n", __func__);
    return -1;
  }

  ogg_sync_reset (&reader->ogg_sync);

  seek_info->offset_at = oggz_io_tell (seek_info->oggz);
  seek_info->offset_max = seek_info->cache.size;

  while (page_next (seek_info) >= 0) {
          debug_printf (2, "Setting last_page_offset to 0x%08llx", seek_info->offset_at);
          seek_info->cache.last_page_offset = seek_info->offset_at;
          seek_info->cache.unit_end = seek_info->unit_at;
  }

  debug_printf (3, "OUT: last_page_offset is 0x%08llx", seek_info->cache.last_page_offset);

  return 0;
}

static int
update_seek_cache (OggzSeekInfo * seek_info)
{
  struct stat statbuf;
  int fd;
  oggz_off_t offset_save;
  OGGZ * oggz = seek_info->oggz;

  seek_info->offset_at = oggz->offset;

  if (oggz->file == NULL) {
    /* Can't check validity, just update end */
    offset_save = oggz_io_tell (oggz);
    if (oggz_io_seek (oggz, 0, SEEK_END) == -1) {
      return -1;
    }

    seek_info->cache.size = oggz_io_tell (oggz);

    if (oggz_io_seek (oggz, offset_save, SEEK_SET) == -1) {
      return -1; /* fubar */
    }
  } else {
    if ((fd = fileno (oggz->file)) == -1) {
      return -1;
    }

    if (fstat (fd, &statbuf) == -1) {
      switch (errno) {
      default:
        return -1;
      }
    }

    if (oggz_stat_regular (statbuf.st_mode)) {
      if (seek_info->cache.mtime == statbuf.st_mtime) {
        /* Not modified, cache is valid */
        return 0;
      }
    }

    seek_info->cache.mtime = statbuf.st_mtime;
    seek_info->cache.size = statbuf.st_size;
  }

  update_last_page (seek_info);

  return 1;
}

#define GUESS_MULTIPLIER ((ogg_int64_t)(1<<16))
#define GUESS_ROLLBACK ((oggz_off_t)4*2048)

static oggz_off_t
guess (OggzSeekInfo * si)
{
  ogg_int64_t guess_ratio;
  oggz_off_t offset_guess;

  debug_printf (2, "Guessing, at %lld in (%lld, %lld)", si->unit_at, si->unit_begin, si->unit_end);

  if (si->unit_end != -1) {
    guess_ratio =
      GUESS_MULTIPLIER * (si->unit_target - si->unit_begin) /
        (si->unit_end - si->unit_begin);

    if (si->prev_guess_was_zoom) {
      si->prev_guess_was_zoom = 0;
    } else {
      /* If we're near the extremes, try to zoom in */
      if (guess_ratio < GUESS_MULTIPLIER/5) {
        debug_printf (2, " << guess_ratio %lld near beginning", guess_ratio);
        guess_ratio = 2 * GUESS_MULTIPLIER / 5;
      } else if (guess_ratio > 4*GUESS_MULTIPLIER/5) {
        debug_printf (2, " >> guess_ratio near end");
        guess_ratio = 3 * GUESS_MULTIPLIER / 5;
      }

      /* Force next to not be zoom */
      si->prev_guess_was_zoom = 1;
    }
    debug_printf (2, "unit_target %lld, unit_begin %lld, unit_end %lld",
                  si->unit_target, si->unit_begin, si->unit_end);
    debug_printf (2, "(1) Guess ratio %lld * (target-begin)/(end-begin) = %lld",
                  GUESS_MULTIPLIER, guess_ratio);
  } else {
    if (si->unit_at == si->unit_begin) {
      debug_printf (2, "at unit_begin, FAIL");
      return si->offset_begin;
    }

    guess_ratio =
      GUESS_MULTIPLIER * (si->unit_target - si->unit_begin) /
        (si->unit_at - si->unit_begin);
    debug_printf (2, "(2) Guess ratio %lld", guess_ratio);
  }

  offset_guess = si->offset_begin +
    (oggz_off_t)(((si->offset_end - si->offset_begin) * guess_ratio) / GUESS_MULTIPLIER);

  debug_printf (2, "offset_begin 0x%08llx, offset_end 0x%08llx",
                si->offset_begin, si->offset_end);
  debug_printf (2, "Guessed offset (end-begin)*%lld / %lld = 0x%08llx",
                guess_ratio, GUESS_MULTIPLIER, offset_guess);

  if (offset_guess == si->offset_end) {
    debug_printf (2, "Got it at the end. Found0?");
  } else if (offset_guess-GUESS_ROLLBACK > si->offset_begin) {
    offset_guess -= GUESS_ROLLBACK;
    debug_printf (2, "Subtracting rollback 0x%08llx, offset_guess now 0x%08llx",
                  GUESS_ROLLBACK, offset_guess);
  } else {
    offset_guess = si->offset_begin;
    debug_printf (2, "Sticking to the start! Found?");
  }

  debug_printf (2, "Guessed 0x%08llx", offset_guess);

  return offset_guess;
}

/* Setup bounding units */
static ogg_int64_t
seek_info_setup_units (OggzSeekInfo * si)
{
  oggz_off_t offset;
  ogg_page * og;

  debug_printf (3, "IN");
  seek_info_dump (si);

  debug_printf (3, "Checking ...");
  if (si->offset_begin < 0) {
    si->offset_begin = 0;
    si->unit_begin = 0; /* XXX: cache.unit_begin */
  } else if (si->unit_begin == -1) {
    if (page_at_or_after (si, si->offset_begin) == -1) {
      debug_printf (2, "page_at_or_after(offset_begin) failed");
      return -1;
    }
    si->unit_begin = si->unit_at;
  }

  if (si->offset_end >= si->offset_max) {
      debug_printf (2, "offset_end >= size, setting offset_end, unit_end");
      si->offset_end = si->cache.size;
      si->unit_end = si->cache.unit_end;
  } else if (si->unit_end == -1) {
    if (page_at_or_after (si, si->offset_end) == -1) {
      debug_printf (2, "page_at_or_after(offset_end) failed");
      return -1;
    }
    si->unit_end = si->unit_at;
  }

  seek_info_dump (si);

  /* Fail if target isn't in specified range. */
  if (si->unit_target < si->unit_begin ||
      si->unit_target > si->unit_end) {
    debug_printf (2, "Target not in specified range");
    return -1;
  }

  /* Reduce the search range if possible using read cursor position. */
  if (si->offset_at >= si->offset_begin && si->offset_at < si->offset_end &&
      si->unit_at >= si->unit_begin && si->unit_at < si->unit_end) {
    if (si->unit_target < si->unit_at) {
      debug_printf (2, "Reducing range to (begin 0x%08llx, at 0x%08llx)",
                    si->offset_begin, si->offset_at);
      si->unit_end = si->unit_at;
      si->offset_end = si->offset_at;
    } else {
      debug_printf (2, "Reducing range to (at 0x%08llx, end 0x%08llx)",
                    si->offset_at, si->offset_end);
      si->unit_begin = si->unit_at;
      si->offset_begin = si->offset_at;
    }
  }

  return 0;
}

static ogg_int64_t
seek_bisect (OggzSeekInfo * seek_info)
{
  OGGZ * oggz = seek_info->oggz;
  OggzReader * reader;
  ogg_int64_t result;
  oggz_off_t offset, ret, earliest_nogp=0;
  oggz_off_t pre_offset_begin, pre_offset_end, pre_offset_at;
  int found=0, jumps=0, fwdscan;

  debug_printf (3, "IN");
  seek_info_dump (seek_info);

  reader = &oggz->x.reader;

  if (seek_info->offset_begin > seek_info->offset_end) {
    debug_printf (2, "offset_begin > offset_end");
    return -1;
  }

  seek_info->prev_guess_was_zoom = 0;

  do {
    debug_printf (2, "bisecting, jumps=%d\n", jumps);

    if (seek_info_setup_units (seek_info) == -1) {
      debug_printf (2, "setup units failed");
      break;
    }

    pre_offset_begin = seek_info->offset_begin;
    pre_offset_end = seek_info->offset_end;
    pre_offset_at = seek_info->offset_at;

    if ((offset = guess (seek_info)) == -1) {
      debug_printf (2, "guess failed");
      break;
    }

    fwdscan=0;
    earliest_nogp = 0;
    do {
      debug_printf (3, "Doing fwdscan %d", fwdscan);
      if ((ret = page_at_or_after (seek_info, offset)) == -1) {
        debug_printf (2, "page_at_or_after failed");
        /* XXX: FAIL spectacularly */
        break;
      }

      if (fwdscan==0 && seek_info->unit_at == -1) {
        earliest_nogp = ret;
      }

      offset = ret+1;
      fwdscan++;
    } while (ret != NOT_FOUND_WITHIN_BOUNDS && seek_info->unit_at == -1 && fwdscan < 100);

    debug_printf (2, " + Scanned forward %d pages", fwdscan);

    if (seek_info->unit_at >= seek_info->unit_target) {
      debug_printf (2, "We are beyond! fwdscan=%d", fwdscan);
      if (earliest_nogp > 0) {
        debug_printf (2, "Setting offset_end to earliest_nogp 0x%08llx", earliest_nogp);
        seek_info->offset_end = earliest_nogp;
        seek_info->offset_max = seek_info->offset_at + seek_info->current_page_bytes;
        seek_info->unit_end = seek_info->unit_at;
      } else if (earliest_nogp == 0) {
        debug_printf (2, "Setting offset_end to offset_at 0x%08llx", seek_info->offset_at);
        seek_info->offset_end = seek_info->offset_at;
        seek_info->offset_max = seek_info->offset_at + seek_info->current_page_bytes;
        seek_info->unit_end = seek_info->unit_at;
      }
    } else {
      debug_printf (2, "We are before!");
      
      if (ret == NOT_FOUND_WITHIN_BOUNDS) {
        seek_info->offset_begin -= PAGESIZE;
        if (seek_info->offset_begin < 0)
          seek_info->offset_begin = 0;
      }

      if (seek_info->unit_target - seek_info->unit_at < 500) {
        debug_printf (1, "Within 500ms of target");
        found = 1;
      }
    }

    if (pre_offset_begin == seek_info->offset_begin &&
        pre_offset_end == seek_info->offset_end &&
        pre_offset_at == seek_info->offset_at) {
      debug_printf (2, "Ended bisection");
      found = 1;
    }

    jumps++;
  } while (!found && jumps<100);

  result = seek_info->unit_at;
  reader->current_unit = result;

  debug_printf (1, "Bisected in %d steps", jumps);

  debug_printf (3, "OUT, returning 0x%08llx", result);
  seek_info_dump (seek_info);

  return result;
}

static ogg_int64_t
seek_scan (OggzSeekInfo * seek_info)
{
  OGGZ * oggz = seek_info->oggz;
  OggzReader * reader;
  ogg_int64_t unit, result;
  oggz_off_t offset, ret, earliest_nogp=0;
  ogg_page * og;

  unit = seek_info->unit_at;
  offset = seek_info->offset_at;

  while (page_next (seek_info) > 0) {
    if (seek_info->unit_at == -1)
      continue;

    if (seek_info->unit_at == seek_info->unit_target) {
      debug_printf (2, "at target");
      /* If this page has exactly the desired units, then it is ok to
         update the desired position to here only if the packet with
         that unit begins on this page. This can be determined in two
         ways:
           1. If the page is not continued, then the packet must begin
              on this page.
           2. If the page is continued and at least 2 packets end on
              this page, then the first is the continued packet and
              another is the desired packet.
       */
      if ((!ogg_page_continued(&seek_info->og_at)) ||
          (ogg_page_packets(&seek_info->og_at) > 1)) {
        unit = seek_info->unit_at;
        offset = seek_info->offset_at;
      }
      break;
    } else if (seek_info->unit_at > seek_info->unit_target) {
      debug_printf (2, "beyond target");
      break;
    } else {
      debug_printf (2, "before target");
      unit = seek_info->unit_at;
      offset = seek_info->offset_at;
    }
  }

  offset = page_at_or_after (seek_info, offset);

  reader = &oggz->x.reader;
  reader->current_page_bytes = 0;

  reader->expect_hole = ogg_page_continued(&seek_info->og_at);

  oggz_seek_raw (oggz, offset, SEEK_SET);

  reader->current_unit = seek_info->unit_at = unit;

  return unit;
}

static ogg_int64_t
oggz_seek_bisect_scan (OggzSeekInfo * seek_info)
{
  OGGZ * oggz = seek_info->oggz;
  OggzReader * reader;
  ogg_int64_t result;
  oggz_off_t offset_end, offset_max;
  ogg_int64_t unit_end;
  ogg_page * og;

  offset_end = seek_info->offset_end;
  offset_max = seek_info->offset_max;
  unit_end = seek_info->unit_end;

  result = seek_bisect (seek_info);
  debug_printf (2, "seek_bisect() returned 0x%08llx\n", result);

  /* Reset end */
  seek_info->offset_end = offset_end;
  seek_info->offset_max = offset_max;
  seek_info->unit_end = unit_end;

  result = seek_scan (seek_info);
  debug_printf (2, "seek_scan() returned 0x%08llx\n", result);

  oggz->offset = result;

  reader = &oggz->x.reader;
  reader->current_page_bytes = 0;

  og = &seek_info->og_at;
  reader->expect_hole = ogg_page_continued(og);

  packet_next (seek_info, seek_info->offset_at);

  return result;
}

/************************************************************
 * Public API
 */

/*
 * The typical usage is:
 *
 *   oggz_set_data_start (oggz, oggz_tell (oggz));
 */
int
oggz_set_data_start (OGGZ * oggz, oggz_off_t offset)
{
  if (oggz == NULL) return -1;

  if (offset < 0) return -1;

  oggz->offset_data_begin = offset;

  return 0;
}

int
oggz_purge (OGGZ * oggz)
{
  return -1;
}

off_t
oggz_seek (OGGZ * oggz, oggz_off_t offset, int whence)
{
  OggzSeekInfo seek_info;
  OggzReader * reader;
  off_t result;
  ogg_page * og;

  if (oggz == NULL)
    return OGGZ_ERR_BAD_OGGZ;

  if (oggz->flags & OGGZ_WRITE)
    return OGGZ_ERR_INVALID;

  memset (&seek_info, 0, sizeof(OggzSeekInfo));

  seek_info.oggz = oggz;

  update_seek_cache (&seek_info);

  switch (whence) {
  case SEEK_CUR:
    offset += oggz->offset;
    break;
  case SEEK_END:
    offset = seek_info.cache.size - offset;
    break;
  case SEEK_SET:
  default:
    break;
  }

  result = page_at_or_after (&seek_info, offset);

  reader = &oggz->x.reader;
  reader->current_page_bytes = 0;

  og = &seek_info.og_at;
  reader->expect_hole = ogg_page_continued(og);

  return oggz_seek_raw (oggz, result, SEEK_SET);
  //return packet_next (&seek_info, result);
}

ogg_int64_t
oggz_seek_units (OGGZ * oggz, ogg_int64_t units, int whence)
{
  OggzSeekInfo seek_info;
  OggzReader * reader;

  if (oggz == NULL)
    return OGGZ_ERR_BAD_OGGZ;

  if (oggz->flags & OGGZ_WRITE)
    return OGGZ_ERR_INVALID;

  memset (&seek_info, 0, sizeof(OggzSeekInfo));

  seek_info.oggz = oggz;

  update_seek_cache (&seek_info);

  reader = &oggz->x.reader;

  switch (whence) {
  case SEEK_CUR:
    units += reader->current_unit;
    break;
  case SEEK_END:
    units = seek_info.cache.unit_end - units;
    break;
  case SEEK_SET:
  default:
    break;
  }

  debug_printf (2, "Got last_page_offset 0x%08llx", seek_info.cache.last_page_offset);

  seek_info.offset_begin = 0;
  seek_info.offset_end = seek_info.cache.last_page_offset;
  seek_info.offset_max = seek_info.cache.size;

  seek_info.unit_target = units;
  seek_info.unit_begin = 0;
  seek_info.unit_end = seek_info.cache.unit_end;

  seek_info_dump((&seek_info));

  return oggz_seek_bisect_scan (&seek_info);
}

off_t
oggz_seek_position (OGGZ * oggz, oggz_position * position)
{
  OggzReader * reader;

  if (oggz == NULL)
    return OGGZ_ERR_BAD_OGGZ;

  if (oggz->flags & OGGZ_WRITE)
    return OGGZ_ERR_INVALID;

  if (oggz_seek_raw (oggz, position->begin_page_offset, SEEK_SET) == -1) {
     debug_printf (2, "oggz_seek_raw() returned -1\n");
     return -1;
   }

  oggz->offset = position->begin_page_offset;

  reader = &oggz->x.reader;
  reader->current_unit = -1;

  /* Set up the position info */
  reader->current_granulepos = position->calc_granulepos;
  reader->current_packet_pages = position->pages;
  reader->current_packet_begin_page_offset = position->begin_page_offset;
  reader->current_packet_begin_segment_index = position->begin_segment_index;

  /* Tell oggz_read_sync() that the position info is set up, so it can
   * simply skip over packets until the requested segment is found, then
   * deliver as normal.
   * The actual data fetching is ensured by the next invocation of
   * oggz_read*(). */
  reader->position_ready = OGGZ_POSITION_END;

  return oggz->offset;
}

long
oggz_get_duration (OGGZ * oggz)
{
  OggzSeekInfo seek_info;

  memset (&seek_info, 0, sizeof(OggzSeekInfo));

  seek_info.oggz = oggz;

  update_seek_cache (&seek_info);

  return seek_info.cache.unit_end;
}

long
oggz_seek_byorder (OGGZ * oggz, void * target)
{
  return -1;
}

long
oggz_seek_packets (OGGZ * oggz, long serialno, long packets, int whence)
{
  return -1;
}

#else

#include <ogg/ogg.h>
#include "oggz_private.h"

off_t
oggz_seek (OGGZ * oggz, oggz_off_t offset, int whence)
{
  return OGGZ_ERR_DISABLED;
}

long
oggz_seek_units (OGGZ * oggz, ogg_int64_t units, int whence)
{
  return OGGZ_ERR_DISABLED;
}

long
oggz_seek_byorder (OGGZ * oggz, void * target)
{
  return OGGZ_ERR_DISABLED;
}

long
oggz_seek_packets (OGGZ * oggz, long serialno, long packets, int whence)
{
  return OGGZ_ERR_DISABLED;
}

#endif
