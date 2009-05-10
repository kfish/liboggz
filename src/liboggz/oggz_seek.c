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

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <errno.h>

#include <ogg/ogg.h>

#include "oggz_compat.h"
#include "oggz_private.h"
#include "oggz/oggz_table.h"

typedef struct _OggzSeekInfo OggzSeekInfo;

struct _OggzSeekTrack {
  /* Offset of bytewise-latest page with time < target */
  oggz_off_t latest_page_offset;
};

struct OggzSeekCache {
  time_t mtime;
  off_t size;
  off_t last_page_offset;
  long unit_end;
};

struct _OggzSeekInfo {
  OGGZ * oggz;

  struct OggzSeekCache cache;

  long unit_target;

  OggzTable * tracks;
};

static int
update_last_page (OggzSeekInfo * seek_info)
{
  return 0;
}

static int
update_seek_cache (OggzSeekInfo * seek_info)
{
  struct stat statbuf;
  int fd;
  oggz_off_t offset_save;
  OGGZ * oggz = seek_info->oggz;

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

off_t
oggz_seek (OGGZ * oggz, oggz_off_t offset, int whence)
{
  return -1;
}

long
oggz_seek_units (OGGZ * oggz, ogg_int64_t units, int whence)
{
  return -1;
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
