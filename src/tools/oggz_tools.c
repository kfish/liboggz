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
*/

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <oggz/oggz.h>

#ifdef WIN32                                                                   
#include <fcntl.h>                                                             
#endif  

static  ogg_uint32_t
_le_32 (ogg_uint32_t i)
{
   ogg_uint32_t ret=i;
#ifdef WORDS_BIGENDIAN
   ret =  (i>>24);
   ret += (i>>8) & 0x0000ff00;
   ret += (i<<8) & 0x00ff0000;
   ret += (i<<24);
#endif
   return ret;
}

static  ogg_uint16_t
_be_16 (ogg_uint16_t s)
{
  unsigned short ret=s;
#ifndef WORDS_BIGENDIAN
  ret = (s>>8) & 0x00ffU;
  ret += (s<<8) & 0xff00U;
#endif
  return ret;
}

static  ogg_uint32_t
_be_32 (ogg_uint32_t i)
{
   ogg_uint32_t ret=i;
#ifndef WORDS_BIGENDIAN
   ret =  (i>>24);
   ret += (i>>8) & 0x0000ff00;
   ret += (i<<8) & 0x00ff0000;
   ret += (i<<24);
#endif
   return ret;
}

static  ogg_int64_t
_le_64 (ogg_int64_t l)
{
  ogg_int64_t ret=l;
  unsigned char *ucptr = (unsigned char *)&ret;
#ifdef WORDS_BIGENDIAN
  unsigned char temp;

  temp = ucptr [0] ;
  ucptr [0] = ucptr [7] ;
  ucptr [7] = temp ;

  temp = ucptr [1] ;
  ucptr [1] = ucptr [6] ;
  ucptr [6] = temp ;

  temp = ucptr [2] ;
  ucptr [2] = ucptr [5] ;
  ucptr [5] = temp ;

  temp = ucptr [3] ;
  ucptr [3] = ucptr [4] ;
  ucptr [4] = temp ;

#endif
  return (*(ogg_int64_t *)ucptr);
}

#define INT32_LE_AT(x) _le_32((*(ogg_int32_t *)(x)))
#define INT16_BE_AT(x) _be_16((*(ogg_uint16_t *)(x)))
#define INT32_BE_AT(x) _be_32((*(ogg_int32_t *)(x)))
#define INT64_LE_AT(x) _le_64((*(ogg_int64_t *)(x)))

typedef char * (* OTCodecInfoFunc) (unsigned char * data, long n);

typedef struct {
  const char *bos_str;
  int bos_str_len;
  const char *content_type;
  OTCodecInfoFunc info_func;
} OTCodecIdent;

static char *
ot_theora_info (unsigned char * data, long len)
{
  char * buf;
  int width, height;

  if (len < 41) return NULL;

  buf = malloc (80);

  width = INT16_BE_AT(&data[10]) << 4;
  height = INT16_BE_AT(&data[12]) << 4;

  snprintf (buf, 80,
	    "\tVideo-Framerate: %.3f fps\n"
	    "\tVideo-Width: %d\n\tVideo-Height: %d\n",
	    (double)INT32_BE_AT(&data[22])/ (double)INT32_BE_AT(&data[26]),
	    width, height);

  return buf;
}

static char *
ot_vorbis_info (unsigned char * data, long len)
{
  char * buf;

  if (len < 30) return NULL;

  buf = malloc (60);

  snprintf (buf, 60,
	    "\tAudio-Samplerate: %d Hz\n\tAudio-Channels: %d\n",
	    INT32_LE_AT(&data[12]), (int)(data[11]));

  return buf;
}

static char *
ot_speex_info (unsigned char * data, long len)
{
  char * buf;

  if (len < 68) return NULL;

  buf = malloc (60);

  snprintf (buf, 60,
	    "\tAudio-Samplerate: %d Hz\n\tAudio-Channels: %d\n",
	    INT32_LE_AT(&data[36]), INT32_LE_AT(&data[48]));

  return buf;
}

static char *
ot_skeleton_info (unsigned char * data, long len)
{
  char * buf;

  if (len < 64) return NULL;

  buf = malloc (60);

  snprintf (buf, 60,
	    "\tPresentation-Time: %.3f\n\tBasetime: %.3f\n",
	    (double)INT64_LE_AT(&data[12]) / (double)INT64_LE_AT(&data[20]),
	    (double)INT64_LE_AT(&data[28]) / (double)INT64_LE_AT(&data[36]));

  return buf;
}

static const OTCodecIdent codec_ident[] = {
  {"\200theora", 7, "Theora", ot_theora_info},
  {"\001vorbis", 7, "Vorbis", ot_vorbis_info},
  {"Speex", 5, "Speex", ot_speex_info},
  {"CMML\0\0\0\0", 8, "CMML", NULL},
  {"Annodex", 8, "Annodex", NULL},
  {"fishead", 8, "Skeleton", ot_skeleton_info},
  {NULL}
};

const char *
ot_page_identify (const ogg_page * og, char ** info)
{
  const char * ret = NULL;
  int i;

  /* try to identify stream codec name by looking at the first bytes of the
   * first packet */
  for (i = 0;; i++) {
    const OTCodecIdent *ident = &codec_ident[i];
    
    if (ident->bos_str == NULL) {
      ret = NULL;
      break;
    }

    if (og->body_len >= ident->bos_str_len &&
	memcmp (og->body, ident->bos_str, ident->bos_str_len) == 0) {
      ret = ident->content_type;
      if (info) {
	if (ident->info_func) {
	  *info = ident->info_func (og->body, og->body_len);
	} else {
	  *info = NULL;
	}
      }
      break;
    }
  }

  return ret;
}

/*
 * Print a number of bytes to 3 significant figures
 * using standard abbreviations (GB, MB, kB, byte[s])
 */
int
ot_fprint_bytes (FILE * stream, long nr_bytes)
{
  if (nr_bytes > (1L<<30)) {
    return fprintf (stream, "%0.3f GB",
		   (double)nr_bytes / (1024.0 * 1024.0 * 1024.0));
  } else if (nr_bytes > (1L<<20)) {
    return fprintf (stream, "%0.3f MB",
		   (double)nr_bytes / (1024.0 * 1024.0));
  } else if (nr_bytes > (1L<<10)) {
    return fprintf (stream, "%0.3f kB",
		   (double)nr_bytes / (1024.0));
  } else if (nr_bytes == 1) {
    return fprintf (stream, "1 byte");
  } else {
    return fprintf (stream, "%ld bytes", nr_bytes);
  }
}

/*
 * Print a bitrate to 3 significant figures
 * using quasi-standard abbreviations (Gbps, Mbps, kbps, bps)
 */
int
ot_print_bitrate (long bps)
{
  if (bps > (1000000000L)) {
    return printf ("%0.3f Gbps",
		   (double)bps / (1000.0 * 1000.0 * 1000.0));
  } else if (bps > (1000000L)) {
    return printf ("%0.3f Mbps",
		   (double)bps / (1000.0 * 1000.0));
  } else if (bps > (1000L)) {
    return printf ("%0.3f kbps",
		   (double)bps / (1000.0));
  } else {
    return printf ("%ld bps", bps);
  }
}

int
ot_fprint_time (FILE * stream, double seconds)
{
  int hrs, min;
  double sec;
  char * sign;

  sign = (seconds < 0.0) ? "-" : "";

  if (seconds < 0.0) seconds = -seconds;

  hrs = (int) (seconds/3600.0);
  min = (int) ((seconds - ((double)hrs * 3600.0)) / 60.0);
  sec = seconds - ((double)hrs * 3600.0)- ((double)min * 60.0);

  /* XXX: %02.3f workaround */
  if (sec < 10.0) {
    return fprintf (stream, "%s%02d:%02d:0%2.3f", sign, hrs, min, sec);
  } else {
    return fprintf (stream, "%s%02d:%02d:%02.3f", sign, hrs, min, sec);
  }
}

void
ot_init (void)
{
#ifdef _WIN32
  /* We need to set stdin/stdout to binary mode */

  _setmode( _fileno( stdin ), _O_BINARY );
  _setmode( _fileno( stdout ), _O_BINARY );
#endif
}
