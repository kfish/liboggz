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

#include <stdio.h>
#include <stdlib.h>
#include <limits.h> /* LONG_MAX */
#include <math.h>
#include <oggz/oggz.h>

#include "oggz_tools.h"

#ifndef WIN32
#include <inttypes.h>
#else
#define PRId64 "ld"
#endif

typedef struct _OI_Info OI_Info;
typedef struct _OI_Stats OI_Stats;
typedef struct _OI_TrackInfo OI_TrackInfo;

/* Let's get functional */
typedef void (*OI_TrackFunc) (OI_Info * info, OI_TrackInfo * oit, long serialno);

struct _OI_Info {
  OggzTable * tracks;
  ogg_int64_t duration;
  long length_total;
};

struct _OI_Stats {
  /* Pass 1 */
  long count;
  long length_total;
  long length_min;
  long length_max;

  /* Pass 2 */
  long length_avg;
  long length_deviation_total;
  double length_stddev;
};

struct _OI_TrackInfo {
  OI_Stats pages;
  OI_Stats packets;
  const char * codec_name;
  char * codec_info;
};

static void
oggzinfo_apply (OI_TrackFunc func, OI_Info * info)
{
  OI_TrackInfo * oit;
  long serialno;
  int n, i;

  n = oggz_table_size (info->tracks);
  for (i = 0; i < n; i++) {
    oit = oggz_table_nth (info->tracks, i, &serialno);
    if (oit) func (info, oit, serialno);
  }
}

static void
oi_stats_clear (OI_Stats * stats)
{
  stats->count = 0;

  stats->length_total = 0;
  stats->length_min = LONG_MAX;
  stats->length_max = 0;

  stats->length_avg = 0;
  stats->length_deviation_total = 0;
  stats->length_stddev = 0;
}

static OI_TrackInfo *
oggzinfo_trackinfo_new (void)
{
  OI_TrackInfo * oit;

  oit = malloc (sizeof (OI_TrackInfo));

  oi_stats_clear (&oit->pages);
  oi_stats_clear (&oit->packets);

  return oit;
}

static long
oi_bitrate (long bytes, ogg_int64_t ms)
{
  return (long) (((ogg_int64_t)bytes * 8 * 1000) / ms);
}

static void
oi_stats_print (OI_Info * info, OI_Stats * stats, char * label)
{
  printf ("\t%s-Length-Maximum: %ld bytes\n", label, stats->length_max);
  /*printf ("\t%s-Length-Average: %ld bytes\n", label, stats->length_avg);*/
  printf ("\t%s-Length-StdDev: %.0f bytes\n", label, stats->length_stddev);
  /*
  printf ("\tRange: [%ld - %ld] bytes, Std.Dev. %.3f bytes\n",
	  stats->length_min, stats->length_max, stats->length_stddev);
  */
  printf ("\n");
}

/* oggzinfo_trackinfo_print() */
static void
oit_print (OI_Info * info, OI_TrackInfo * oit, long serialno)
{
  printf ("%s: serialno %010ld\n", oit->codec_name, serialno);

  printf ("\t%ld packets in %ld pages, %.1f packets/page\n",
	  oit->packets.count, oit->pages.count,
	  (double)oit->packets.count / (double)oit->pages.count);
  printf ("\tContent-Length: %ld bytes\n", oit->pages.length_total);
  printf ("\tContent-Bitrate-Average: %ld bps\n",
	  oi_bitrate (oit->pages.length_total, info->duration));
  printf ("\n");

  if (oit->codec_info != NULL) {
    puts (oit->codec_info);
  }

  /* Page stats */
  oi_stats_print (info, &oit->pages, "Page");

  /* Packet stats */
  oi_stats_print (info, &oit->packets, "Packet");
}

static void
oi_stats_average (OI_Stats * stats)
{
  stats->length_avg = stats->length_total / stats->count;
}

static void
oit_calc_average (OI_Info * info, OI_TrackInfo * oit, long serialno)
{
  oi_stats_average (&oit->pages);
  oi_stats_average (&oit->packets);
}

static void
oi_stats_stddev (OI_Stats * stats)
{
  double variance;

  variance = (double)stats->length_deviation_total / (double)(stats->count - 1);
  stats->length_stddev = sqrt (variance);

}

static void
oit_calc_stddev (OI_Info * info, OI_TrackInfo * oit, long serialno)
{
  oi_stats_stddev (&oit->pages);
  oi_stats_stddev (&oit->packets);
}

static int
read_page_pass1 (OGGZ * oggz, const ogg_page * og, long serialno, void * user_data)
{
  OI_Info * info = (OI_Info *)user_data;
  OI_TrackInfo * oit;
  long bytes;

  oit = oggz_table_lookup (info->tracks, serialno);
  if (oit == NULL) {
    oit = oggzinfo_trackinfo_new ();
    oggz_table_insert (info->tracks, serialno, oit);
  }

  if (ogg_page_bos ((ogg_page *)og)) {
    oit->codec_name = ot_page_identify (og, &oit->codec_info);
  }

  bytes = og->header_len + og->body_len;

  /* Increment the total stream length */
  info->length_total += bytes;

  /* Increment the page statistics */
  oit->pages.count++;
  oit->pages.length_total += bytes;
  if (bytes < oit->pages.length_min)
    oit->pages.length_min = bytes;
  if (bytes > oit->pages.length_max)
    oit->pages.length_max = bytes;

  return 0;
}

static int
read_page_pass2 (OGGZ * oggz, const ogg_page * og, long serialno, void * user_data)
{
  OI_Info * info = (OI_Info *)user_data;
  OI_TrackInfo * oit;
  long bytes, deviation;

  oit = oggz_table_lookup (info->tracks, serialno);

  /* Increment the page length deviation squared total */
  bytes = og->header_len + og->body_len;
  deviation = bytes - oit->pages.length_avg;
  oit->pages.length_deviation_total += (deviation * deviation);

  return 0;
}

static int
read_packet_pass1 (OGGZ * oggz, ogg_packet * op, long serialno,
		   void * user_data)
{
  OI_Info * info = (OI_Info *)user_data;
  OI_TrackInfo * oit;

  oit = oggz_table_lookup (info->tracks, serialno);

  /* Increment the packet statistics */
  oit->packets.count++;
  oit->packets.length_total += op->bytes;
  if (op->bytes < oit->packets.length_min)
    oit->packets.length_min = op->bytes;
  if (op->bytes > oit->packets.length_max)
    oit->packets.length_max = op->bytes;

  return 0;
}

static int
read_packet_pass2 (OGGZ * oggz, ogg_packet * op, long serialno,
		   void * user_data)
{
  OI_Info * info = (OI_Info *)user_data;
  OI_TrackInfo * oit;
  long deviation;

  oit = oggz_table_lookup (info->tracks, serialno);

  /* Increment the packet length deviation squared total */
  deviation = op->bytes - oit->packets.length_avg;
  oit->packets.length_deviation_total += (deviation * deviation);

  return 0;
}

static int
oi_pass1 (OGGZ * oggz, OI_Info * info)
{
  long n;

  oggz_seek (oggz, 0, SEEK_SET);
  oggz_set_read_page (oggz, -1, read_page_pass1, info);
  oggz_set_read_callback (oggz, -1, read_packet_pass1, info);

  while ((n = oggz_read (oggz, 1024)) > 0);

  oggzinfo_apply (oit_calc_average, info);

  return 0;
}

static int
oi_pass2 (OGGZ * oggz, OI_Info * info)
{
  long n;

  oggz_seek (oggz, 0, SEEK_SET);
  oggz_set_read_page (oggz, -1, read_page_pass2, info);
  oggz_set_read_callback (oggz, -1, read_packet_pass2, info);

  while ((n = oggz_read (oggz, 1024)) > 0);

  oggzinfo_apply (oit_calc_stddev, info);

  return 0;
}

static void
oit_delete (OI_Info * info, OI_TrackInfo * oit, long serialno)
{
  if (oit->codec_info) free (oit->codec_info);
}

int
main (int argc, char ** argv)
{
  OGGZ * oggz;
  OI_Info info;

  if (argc < 2) {
    printf ("Usage: %s filename\n", argv[0]);
    return (1);
  }

  if ((oggz = oggz_open ((char *)argv[1], OGGZ_READ|OGGZ_AUTO)) == NULL) {
    printf ("unable to open file %s\n", argv[1]);
    return (1);
  }

  info.tracks = oggz_table_new ();
  info.length_total = 0;

  oi_pass1 (oggz, &info);

  oggz_seek_units (oggz, 0, SEEK_END);
  info.duration = oggz_tell_units (oggz);

  oi_pass2 (oggz, &info);

  oggz_close (oggz);

  /* Print summary information */
  printf ("Content-Duration: %lld ms\n", info.duration);
  printf ("Content-Length: %ld bytes\n", info.length_total);
  printf ("Content-Bitrate-Average: %ld bps\n",
	  oi_bitrate (info.length_total, info.duration));
  printf ("\n");

  oggzinfo_apply (oit_print, &info);

  oggzinfo_apply (oit_delete, &info);
  oggz_table_delete (info.tracks);

  return (0);
}
