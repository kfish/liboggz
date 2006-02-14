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

#include <oggz/oggz.h>

#ifdef HAVE_VORBIS
#include <vorbis/codec.h>
#endif

#ifdef HAVE_SPEEX
#include <speex.h>
#include <speex_header.h>
#include <speex_callbacks.h>
#endif

#ifdef HAVE_THEORA
#include <theora/theora.h>
#endif

typedef struct _ogg_rate ogg_rate;

struct _ogg_rate {
  long serialno;
  long rate_numerator;
  long rate_denominator;
  int keyframe_shift;
  double rate_multiplier;
};

#define MAX_STREAMS 16
ogg_rate rates[MAX_STREAMS];

static long granule_rate, rate_interval;
static ogg_int64_t current_granule;
static long current_serialno;

static int
init_stream (long serialno, long rate_numerator, long rate_denominator,
	     int keyframe_shift)
{
  int i;

  for (i = 0; i < MAX_STREAMS; i++) {
    if (rates[i].serialno == -1) {
      rates[i].serialno = serialno;
      rates[i].rate_numerator = rate_numerator;
      rates[i].rate_denominator = rate_denominator;
      rates[i].keyframe_shift = keyframe_shift;
      break;
    }
  }

  if (i == MAX_STREAMS)
    return -1;

  for (i = 0; i < MAX_STREAMS; i++) {
    if (rates[i].serialno != -1) {
      rates[i].rate_multiplier =
	(double)rates[i].rate_denominator / (double)rates[i].rate_numerator;

      printf ("(%ld): %ld / %ld = %f\n", rates[i].serialno,
	      rates[i].rate_denominator, rates[i].rate_numerator,
	      rates[i].rate_multiplier);
    }
  }

  return 0;
}

static ogg_int64_t
gp_metric (OGGZ * oggz, long serialno, ogg_int64_t granulepos,
	   void * user_data)
{
  int i;
  ogg_int64_t units;

  for (i = 0; i < MAX_STREAMS; i++) {
    if (rates[i].serialno == serialno) {
      if (rates[i].keyframe_shift) {
	ogg_int64_t iframe, pframe;
	iframe= granulepos >> rates[i].keyframe_shift;
	pframe= granulepos - (iframe << rates[i].keyframe_shift);
	granulepos = (iframe + pframe);
      }

      units = (1000 * granulepos * rates[i].rate_multiplier);
      printf ("%lld\t(%ld * %f)\n", units,
	      (long)granulepos, rates[i].rate_multiplier);
      return units;
    }
  }

  return -1;
}

#ifdef HAVE_THEORA
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
read_packet (OGGZ * oggz, ogg_packet * op, long serialno, void * user_data)
{
#if 0
  printf ("[%8lx]\t%ld bytes\tgranulepos %lld\n", serialno, op->bytes,
	  op->granulepos);
#endif

  current_granule = op->granulepos;
  current_serialno = serialno;

  if (op->b_o_s) {
    if (!strncmp ((char *)&op->packet[1], "vorbis", 6)) {
#ifdef HAVE_VORBIS
      struct vorbis_info vi;
      struct vorbis_comment vc;
      int ret;

      vorbis_info_init (&vi);
      vorbis_comment_init (&vc);

      if ((ret = vorbis_synthesis_headerin (&vi, &vc, op)) == 0) {
	if (vi.rate != 0) {
	  printf ("Got vorbis info: version %d\tchannels %d\trate %ld\n",
		  vi.version, vi.channels, vi.rate);

	  init_stream (serialno, vi.rate, 1, 0);
	}
      }
#endif
    } else if (!strncmp ((char *)&op->packet[0], "Speex   ", 8)) {
#ifdef HAVE_SPEEX
      SpeexHeader * header;

      header = speex_packet_to_header ((char *)op->packet, op->bytes);

      if (header) {
	init_stream (serialno, header->rate, 1, 0);

	printf ("Got speex samplerate %d\n", header->rate);

	free (header);
      }
#endif
    } else if (!strncmp ((char *)&op->packet[1], "theora", 5)) {
#ifdef HAVE_THEORA
      theora_info t_info;
      theora_comment t_comment;

      theora_info_init (&t_info);
      theora_comment_init (&t_comment);

      if (theora_decode_header(&t_info, &t_comment, op) >= 0) {

	printf ("Got theora %d/%d FPS\n",
		t_info.fps_numerator, t_info.fps_denominator);

	init_stream (serialno, t_info.fps_numerator,
		     t_info.fps_denominator,
		     intlog (t_info.keyframe_frequency_force-1));
      }
#endif
    }
  }

  return 0;
}

int
main (int argc, char ** argv)
{
  OGGZ * oggz;
  int i;
  long n;

  if (argc < 2) {
    printf ("Usage: %s filename\n", argv[0]);
    return (1);
  }

  granule_rate = 1000000;
  rate_interval = 1;

  for (i = 0; i < MAX_STREAMS; i++) {
    rates[i].serialno = -1;
  }

  if ((oggz = oggz_open ((char *)argv[1], OGGZ_READ)) == NULL) {
    printf ("unable to open file %s\n", argv[1]);
    return (1);
  }

  oggz_set_metric (oggz, -1, gp_metric, NULL);

  oggz_set_read_callback (oggz, -1, read_packet, NULL);
  while ((n = oggz_read (oggz, 1024)) > 0);

  printf ("Last unit: %lld\n",
	  gp_metric (oggz, current_serialno, current_granule, NULL));

  oggz_seek (oggz, 10000, SEEK_SET);
  oggz_seek (oggz, 20000, SEEK_SET);
  oggz_seek (oggz, 30000, SEEK_SET);
  oggz_seek (oggz, 10000, SEEK_SET);

  oggz_close (oggz);

  return (0);
}
