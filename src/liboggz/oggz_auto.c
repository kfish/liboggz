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

#include "oggz_private.h"
#include "oggz_byteorder.h"

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

  granule_rate = (ogg_int64_t) INT32_LE_AT(&header[36]);
#ifdef DEBUG
  printf ("Got speex rate %d\n", (int)granule_rate);
#endif

  oggz_set_granulerate (oggz, serialno, granule_rate, OGGZ_AUTO_MULT);

  return 1;
}

static int
auto_vorbis (OGGZ * oggz, ogg_packet * op, long serialno, void * user_data)
{
  unsigned char * header = op->packet;
  ogg_int64_t granule_rate = 0;

  if (op->bytes < 30) return 0;

  granule_rate = (ogg_int64_t) INT32_LE_AT(&header[12]);
#ifdef DEBUG
  printf ("Got vorbis rate %d\n", (int)granule_rate);
#endif

  oggz_set_granulerate (oggz, serialno, granule_rate, OGGZ_AUTO_MULT);

  return 1;
}

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

static int
auto_theora (OGGZ * oggz, ogg_packet * op, long serialno, void * user_data)
{
  unsigned char * header = op->packet;
  ogg_int32_t fps_numerator, fps_denominator;
  char keyframe_granule_shift = 0;
  int keyframe_shift;

  if (op->bytes < 41) return 0;

  fps_numerator = INT32_BE_AT(&header[22]);
  fps_denominator = INT32_BE_AT(&header[26]);

  /* Very old theora versions used a value of 0 to mean 1.
   * Unfortunately theora hasn't incremented its version field,
   * hence we hardcode this workaround for old or broken streams.
   */
  if (fps_numerator == 0) fps_numerator = 1;

#if USE_THEORA_PRE_ALPHA_3_FORMAT
  /* old header format, used by Theora alpha2 and earlier */
  keyframe_granule_shift = (header[36] & 0xf8) >> 3;
  keyframe_shift = intlog (keyframe_granule_shift - 1);
#else
  keyframe_granule_shift = (char) ((header[40] & 0x03) << 3);
  keyframe_granule_shift |= (header[41] & 0xe0) >> 5;
  keyframe_shift = keyframe_granule_shift;
#endif

#ifdef DEBUG
  printf ("Got theora fps %d/%d, keyframe_shift %d\n",
	  fps_numerator, fps_denominator, keyframe_shift);
#endif

  oggz_set_granulerate (oggz, serialno, (ogg_int64_t)fps_numerator,
			OGGZ_AUTO_MULT * (ogg_int64_t)fps_denominator);
  oggz_set_granuleshift (oggz, serialno, keyframe_shift);

  return 1;
}

static int
auto_annodex (OGGZ * oggz, ogg_packet * op, long serialno, void * user_data)
{
  /* Apply a zero metric */
  oggz_set_granulerate (oggz, serialno, 0, 1);

  return 1;
}

static int
auto_anxdata (OGGZ * oggz, ogg_packet * op, long serialno, void * user_data)
{
  unsigned char * header = op->packet;
  ogg_int64_t granule_rate_numerator = 0, granule_rate_denominator = 0;

  if (op->bytes < 28) return 0;

  granule_rate_numerator = INT64_LE_AT(&header[8]);
  granule_rate_denominator = INT64_LE_AT(&header[16]);
#ifdef DEBUG
  printf ("Got AnxData rate %lld/%lld\n", granule_rate_numerator,
	  granule_rate_denominator);
#endif

  oggz_set_granulerate (oggz, serialno,
			granule_rate_numerator,
			OGGZ_AUTO_MULT * granule_rate_denominator);

  return 1;
}

static int
auto_flac0 (OGGZ * oggz, ogg_packet * op, long serialno, void * user_data)
{
  unsigned char * header = op->packet;
  ogg_int64_t granule_rate = 0;

  granule_rate = (ogg_int64_t) (header[14] << 12) | (header[15] << 4) | 
            ((header[16] >> 4)&0xf);
#ifdef DEBUG
    printf ("Got flac rate %d\n", (int)granule_rate);
#endif
    
  oggz_set_granulerate (oggz, serialno, granule_rate, OGGZ_AUTO_MULT);

  return 1;
}

static int
auto_flac (OGGZ * oggz, ogg_packet * op, long serialno, void * user_data)
{
  unsigned char * header = op->packet;
  ogg_int64_t granule_rate = 0;

  if (op->bytes < 51) return 0;

  granule_rate = (ogg_int64_t) (header[27] << 12) | (header[28] << 4) | 
            ((header[29] >> 4)&0xf);
#ifdef DEBUG
  printf ("Got flac rate %d\n", (int)granule_rate);
#endif

  oggz_set_granulerate (oggz, serialno, granule_rate, OGGZ_AUTO_MULT);

  return 1;
}

/**
 * Recognizer for OggPCM2:
 * http://wiki.xiph.org/index.php/OggPCM2
 */
static int
auto_oggpcm2 (OGGZ * oggz, ogg_packet * op, long serialno, void * user_data)
{
  unsigned char * header = op->packet;
  ogg_int64_t granule_rate;

  if (op->bytes < 28) return 0;

  granule_rate = (ogg_int64_t) INT32_BE_AT(&header[16]);
#ifdef DEBUG
  printf ("Got OggPCM2 rate %d\n", (int)granule_rate);
#endif

  oggz_set_granulerate (oggz, serialno, granule_rate, OGGZ_AUTO_MULT);

  return 1;
}

static int
auto_cmml (OGGZ * oggz, ogg_packet * op, long serialno, void * user_data)
{
  unsigned char * header = op->packet;
  ogg_int64_t granule_rate_numerator = 0, granule_rate_denominator = 0;
  int granuleshift;

  if (op->bytes < 28) return 0;

  granule_rate_numerator = INT64_LE_AT(&header[12]);
  granule_rate_denominator = INT64_LE_AT(&header[20]);
  if (op->bytes > 28)
    granuleshift = (int)header[28];
  else
    granuleshift = 0;

#ifdef DEBUG
  printf ("Got CMML rate %lld/%lld\n", granule_rate_numerator,
	  granule_rate_denominator);
#endif

  oggz_set_granulerate (oggz, serialno,
			granule_rate_numerator,
			OGGZ_AUTO_MULT * granule_rate_denominator);
  oggz_set_granuleshift (oggz, serialno, granuleshift);

  return 1;
}

static int
auto_fisbone (OGGZ * oggz, ogg_packet * op, long serialno, void * user_data)
{
  unsigned char * header = op->packet;
  long fisbone_serialno; /* The serialno referred to in this fisbone */
  ogg_int64_t granule_rate_numerator = 0, granule_rate_denominator = 0;
  int granuleshift;

  if (op->bytes < 48) return 0;

  fisbone_serialno = (long) INT32_LE_AT(&header[12]);

  /* Don't override an already assigned metric */
  if (oggz_stream_has_metric (oggz, fisbone_serialno)) return 1;

  granule_rate_numerator = INT64_LE_AT(&header[20]);
  granule_rate_denominator = INT64_LE_AT(&header[28]);
  granuleshift = (int)header[48];

#ifdef DEBUG
  printf ("Got fisbone granulerate %lld/%lld, granuleshift %d for serialno %010ld\n",
	  granule_rate_numerator, granule_rate_denominator, granuleshift,
	  fisbone_serialno);
#endif

  oggz_set_granulerate (oggz, fisbone_serialno,
			granule_rate_numerator,
			OGGZ_AUTO_MULT * granule_rate_denominator);
  oggz_set_granuleshift (oggz, fisbone_serialno, granuleshift);
				
  return 1;
}

static int
auto_fishead (OGGZ * oggz, ogg_packet * op, long serialno, void * user_data)
{
  if (!op->b_o_s)
  {
    return auto_fisbone(oggz, op, serialno, user_data);
  }
  
  oggz_set_granulerate (oggz, serialno, 0, 1);
  
  return 1;
}

static ogg_int64_t 
auto_calc_speex(ogg_int64_t now, oggz_stream_t *stream, ogg_packet *op) {
  
  /*
   * on the first (b_o_s) packet, set calculate_data to be the number
   * of speex frames per packet
   */
  if (stream->calculate_data == NULL) {
    stream->calculate_data = malloc(sizeof(int));
    *(int *)stream->calculate_data = 
        (*(int *)(op->packet + 64)) * (*(int *)(op->packet + 56));
  }
  
  if (now > -1)
    return now;

  /*
   * the first data packet has smaller-than-usual granulepos to account
   * for the fact that several of the output samples from the beginning
   * of this packet need to be thrown away.  We calculate the granulepos
   * by taking the mod of the page's granulepos with respect to the increment
   * between packets.
   */
  if (stream->last_granulepos == 0) {
    return stream->page_granulepos % *(int *)(stream->calculate_data);
  }
  
  return stream->last_granulepos + *(int *)(stream->calculate_data);
}
  
static ogg_int64_t 
auto_calc_theora(ogg_int64_t now, oggz_stream_t *stream, ogg_packet *op) {

  long keyframe_no;
  int keyframe_shift;
  unsigned char first_byte;
  
  if (now > (ogg_int64_t)(-1))
    return now;

  first_byte = op->packet[0];

  if (first_byte & 0x80)
  {
    /* header packet */
    return (ogg_int64_t)0;
  }
  
  if (first_byte & 0x40)
  {
    /* inter-coded packet */
    return stream->last_granulepos + 1;
  }

  /* intra-coded packet */
  if (stream->last_granulepos == 0)
  {
    /* first intra-coded packet */
    return (ogg_int64_t)0;
  }

  keyframe_shift = stream->granuleshift; 
  /*
   * retrieve last keyframe number
   */
  keyframe_no = (int)(stream->last_granulepos >> keyframe_shift);
  /*
   * add frames since last keyframe number
   */
  keyframe_no += (stream->last_granulepos & ((1 << keyframe_shift) - 1)) + 1;
  return ((ogg_int64_t)keyframe_no) << keyframe_shift;
  

}

const oggz_auto_contenttype_t oggz_auto_codec_ident[] = {
  {"\200theora", 7, "Theora", auto_theora, auto_calc_theora},
  {"\001vorbis", 7, "Vorbis", auto_vorbis, NULL},
  {"Speex", 5, "Speex", auto_speex, auto_calc_speex},
  {"PCM     ", 8, "PCM", auto_oggpcm2, NULL},
  {"CMML\0\0\0\0", 8, "CMML", auto_cmml, NULL},
  {"Annodex", 8, "Annodex", auto_annodex, NULL},
  {"fishead", 7, "Skeleton", auto_fishead, NULL},
  {"fLaC", 4, "Flac0", auto_flac0, NULL},
  {"\177FLAC", 4, "Flac", auto_flac, NULL},
  {"AnxData", 7, "AnxData", auto_anxdata, NULL},
  {"", 0, "Unknown", NULL}
}; 

int oggz_auto_identify (OGGZ *oggz, ogg_page *og, long serialno) {

  int i;
  
  for (i = 0; i < OGGZ_CONTENT_UNKNOWN; i++)
  {
    const oggz_auto_contenttype_t *codec = oggz_auto_codec_ident + i;
    
    if (og->body_len >= codec->bos_str_len &&
              memcmp (og->body, codec->bos_str, codec->bos_str_len) == 0) {
      
      oggz_stream_set_content (oggz, serialno, i);
      
      return 1;
    }
  }
                      
  oggz_stream_set_content (oggz, serialno, OGGZ_CONTENT_UNKNOWN);
  return 0;
}

int
oggz_auto_get_granulerate (OGGZ * oggz, ogg_packet * op, long serialno, 
                void * user_data) {
  OggzReadPacket read_packet;
  int content = 0;
  int will_run_function;

  content = oggz_stream_get_content(oggz, serialno);
  if (content < 0 || content >= OGGZ_CONTENT_UNKNOWN) {
    return 0;
  }

  oggz_auto_codec_ident[content].reader(oggz, op, serialno, user_data);
  return 0;
}

ogg_int64_t 
oggz_auto_calculate_granulepos(int content, ogg_int64_t now, 
                oggz_stream_t *stream, ogg_packet *op) {
  if (oggz_auto_codec_ident[content].calculator != NULL) {
    return oggz_auto_codec_ident[content].calculator(now, stream, op);
  } else {
    return now;
  }
  
}

#endif /* OGGZ_CONFIG_READ */
