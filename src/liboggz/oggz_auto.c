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

/*
 * oggz_auto.c
 *
 * Conrad Parker <conrad@annodex.net>
 */

#include "config.h"

#if OGGZ_CONFIG_READ
#include <stdlib.h>
#include <string.h>

#include <oggz/oggz.h>
#include "oggz_auto.h"
#include "oggz_byteorder.h"
#include "oggz_macros.h"
#include "oggz_stream.h"

/*#define DEBUG*/

/* Allow use of internal metrics; ie. the user_data for these gets free'd
 * when the metric is overwritten, or on close */
int oggz_set_metric_internal (OGGZ * oggz, long serialno, OggzMetric metric,
			      void * user_data, int internal);

int oggz_set_metric_linear (OGGZ * oggz, long serialno,
			    ogg_int64_t granule_rate_numerator,
			    ogg_int64_t granule_rate_denominator);

#define INT32_LE_AT(x) _le_32((*(ogg_int32_t *)(x)))
#define INT32_BE_AT(x) _be_32((*(ogg_int32_t *)(x)))
#define INT64_LE_AT(x) _le_64((*(ogg_int64_t *)(x)))

#define OGGZ_AUTO_MULT 1000

static int
auto_speex (OGGZ * oggz, ogg_packet * op, long serialno, void * user_data)
{
  unsigned char * header = op->packet;
  ogg_int64_t granule_rate = 0;

  if (op->bytes < 68) return 0;

  if (strncmp ((char *)header, "Speex   ", 8)) return 0;
  if (!op->b_o_s) return 0;

  granule_rate = (ogg_int64_t) INT32_LE_AT(&header[36]);
#ifdef DEBUG
  printf ("Got speex rate %d\n", (int)granule_rate);
#endif

  oggz_set_metric_linear (oggz, serialno, granule_rate, OGGZ_AUTO_MULT);

  return 1;
}

static int
auto_vorbis (OGGZ * oggz, ogg_packet * op, long serialno, void * user_data)
{
  unsigned char * header = op->packet;
  ogg_int64_t granule_rate = 0;

  if (op->bytes < 30) return 0;

  if (header[0] != 0x01) return 0;
  if (strncmp ((char *)&header[1], "vorbis", 6)) return 0;
  if (!op->b_o_s) return 0;

  granule_rate = (ogg_int64_t) INT32_LE_AT(&header[12]);
#ifdef DEBUG
  printf ("Got vorbis rate %d\n", (int)granule_rate);
#endif

  oggz_set_metric_linear (oggz, serialno, granule_rate, OGGZ_AUTO_MULT);

  return 1;
}

typedef struct {
  ogg_int32_t fps_numerator;
  ogg_int32_t fps_denominator;
  int keyframe_shift;
} oggz_theora_metric_t;

#if USE_THEORA_PRE_ALPHA_3_FORMAT
static int intlog(int num) {
  int ret=0;
  while(num>0){
    num=num/2;
    ret=ret+1;
  }
  return(ret);
}
#endif

static ogg_int64_t
auto_theora_metric (OGGZ * oggz, long serialno, ogg_int64_t granulepos,
		    void * user_data)
{
  oggz_theora_metric_t * tdata = (oggz_theora_metric_t *)user_data;
  ogg_int64_t iframe, pframe;
  ogg_int64_t units;

  iframe = granulepos >> tdata->keyframe_shift;
  pframe = granulepos - (iframe << tdata->keyframe_shift);
  granulepos = (iframe + pframe);

  units = OGGZ_AUTO_MULT * granulepos * tdata->fps_denominator /
    tdata->fps_numerator;

#ifdef DEBUG
  printf ("oggz_auto: serialno %010ld Got theora frame %lld (%lld + %lld): %lld units\n",
	  serialno, granulepos, iframe, pframe, units);
#endif

  return units;
}

static int
auto_theora (OGGZ * oggz, ogg_packet * op, long serialno, void * user_data)
{
  unsigned char * header = op->packet;
  char keyframe_granule_shift = 0;
  oggz_theora_metric_t * tdata;

  if (op->bytes < 41) return 0;

  if (header[0] != 0x80) return 0;
  if (strncmp ((char *)&header[1], "theora", 6)) return 0;
  if (!op->b_o_s) return 0;


  tdata = oggz_malloc (sizeof (oggz_theora_metric_t));

  tdata->fps_numerator = INT32_BE_AT(&header[22]);
  tdata->fps_denominator = INT32_BE_AT(&header[26]);

#if USE_THEORA_PRE_ALPHA_3_FORMAT
  /* old header format, used by Theora alpha2 and earlier */
  keyframe_granule_shift = (header[36] & 0xf8) >> 3;
  tdata->keyframe_shift = intlog (keyframe_granule_shift - 1);
#else
  keyframe_granule_shift = (char) ((header[40] & 0x03) << 3);
  keyframe_granule_shift |= (header[41] & 0xe0) >> 5;
  tdata->keyframe_shift = keyframe_granule_shift;
#endif

#ifdef DEBUG
  printf ("Got theora fps %d/%d, keyframe_shift %d\n",
	  tdata->fps_numerator, tdata->fps_denominator,
	  tdata->keyframe_shift);
#endif

  oggz_set_metric_internal (oggz, serialno, auto_theora_metric, tdata, 1);
  /*oggz_set_metric (oggz, serialno, auto_theora_metric, tdata);*/

  return 1;
}

static int
auto_annodex (OGGZ * oggz, ogg_packet * op, long serialno, void * user_data)
{
  unsigned char * header = op->packet;

  if (op->bytes < 8) return 0;

  if (strncmp ((char *)header, "Annodex", 8)) return 0;
  if (!op->b_o_s) return 0;

  /* Apply a zero metric */
  oggz_set_metric_zero (oggz, serialno);

  return 1;
}

static int
auto_anxdata (OGGZ * oggz, ogg_packet * op, long serialno, void * user_data)
{
  unsigned char * header = op->packet;
  ogg_int64_t granule_rate_numerator = 0, granule_rate_denominator = 0;

  if (op->bytes < 28) return 0;

  if (strncmp ((char *)header, "AnxData", 8)) return 0;
  if (!op->b_o_s) return 0;

  granule_rate_numerator = INT64_LE_AT(&header[8]);
  granule_rate_denominator = INT64_LE_AT(&header[16]);
#ifdef DEBUG
  printf ("Got AnxData rate %lld/%lld\n", granule_rate_numerator,
	  granule_rate_denominator);
#endif

  oggz_set_metric_linear (oggz, serialno,
			  granule_rate_numerator,
			  OGGZ_AUTO_MULT * granule_rate_denominator);

  return 1;
}

static int
auto_flac (OGGZ * oggz, ogg_packet * op, long serialno, void * user_data)
{
  unsigned char * header = op->packet;
  ogg_int64_t granule_rate = 0;

  if (op->bytes < 51) return 0;

  if (header[0] != 0x7f) return 0;
  if (strncmp ((char *)&header[1], "FLAC", 4)) return 0;
  if (!op->b_o_s) return 0;

  granule_rate = (ogg_int64_t) (header[27] << 12) | (header[28] << 4) | ((header[29] >> 4)&0xf);
#ifdef DEBUG
  printf ("Got flac rate %d\n", (int)granule_rate);
#endif

  oggz_set_metric_linear (oggz, serialno, granule_rate, OGGZ_AUTO_MULT);

  return 1;
}

static int
auto_cmml (OGGZ * oggz, ogg_packet * op, long serialno, void * user_data)
{
  unsigned char * header = op->packet;
  ogg_int64_t granule_rate_numerator = 0, granule_rate_denominator = 0;

  if (op->bytes < 28) return 0;

  if (strncmp ((char *)header, "CMML", 4)) return 0;
  if (!op->b_o_s) return 0;

  granule_rate_numerator = INT64_LE_AT(&header[12]);
  granule_rate_denominator = INT64_LE_AT(&header[20]);
#ifdef DEBUG
  printf ("Got CMML rate %lld/%lld\n", granule_rate_numerator,
	  granule_rate_denominator);
#endif

  oggz_set_metric_linear (oggz, serialno,
			  granule_rate_numerator,
			  OGGZ_AUTO_MULT * granule_rate_denominator);

  return 1;
}

static int
auto_fishead (OGGZ * oggz, ogg_packet * op, long serialno, void * user_data)
{
  unsigned char * header = op->packet;
  int content;
  
  if (op->b_o_s) {
    if (op->bytes < 8) return 0;
    if (strncmp ((char *)header, "fishead", 8)) return 0;
    oggz_stream_set_content (oggz, serialno, OGGZ_CONTENT_SKELETON);
  } else if (op->e_o_s) {
    content =  oggz_stream_get_content (oggz, serialno);
    if (content != OGGZ_CONTENT_SKELETON) return 0;

    /* Finished processing the skeleton; apply a zero metric */
    oggz_set_metric_zero (oggz, serialno);
  }

  return 1;
}

static int
auto_fisbone (OGGZ * oggz, ogg_packet * op, long serialno, void * user_data)
{
  int content;
  unsigned char * header = op->packet;
  long fisbone_serialno; /* The serialno referred to in this fisbone */
  ogg_int64_t granule_rate_numerator = 0, granule_rate_denominator = 0;

  if (op->bytes < 48) return 0;

  if (strncmp ((char *)header, "fisbone", 7)) return 0;
  content =  oggz_stream_get_content (oggz, serialno);
  if (content != OGGZ_CONTENT_SKELETON) return 0;

  fisbone_serialno = (long) INT32_LE_AT(&header[12]);

  /* Don't override an already assigned metric */
  if (oggz_stream_has_metric (oggz, fisbone_serialno)) return 1;

  granule_rate_numerator = INT64_LE_AT(&header[20]);
  granule_rate_denominator = INT64_LE_AT(&header[28]);
#ifdef DEBUG
  printf ("Got fisbone granulerate %lld/%lld for serialno %010ld\n",
	  granule_rate_numerator, granule_rate_denominator,
	  fisbone_serialno);
#endif

  oggz_set_metric_linear (oggz, fisbone_serialno,
			  granule_rate_numerator,
			  OGGZ_AUTO_MULT * granule_rate_denominator);

  return 1;
}

static const OggzReadPacket auto_readers[] = {
  auto_speex,
  auto_vorbis,
  auto_theora,
  auto_annodex,
  auto_anxdata,
  auto_flac,
  auto_cmml,
  auto_fishead,
  auto_fisbone,
  NULL
};

int
oggz_auto (OGGZ * oggz, ogg_packet * op, long serialno, void * user_data)
{
  OggzReadPacket read_packet;
  int i = 0;

  for (read_packet = auto_readers[0]; read_packet;
       read_packet = auto_readers[++i]) {
    if (read_packet (oggz, op, serialno, user_data)) return 0;
  }

  return 0;
}

#endif /* OGGZ_CONFIG_READ */
