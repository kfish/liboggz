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
#include <ctype.h>

#ifdef HAVE_INTTYPES_H
#  include <inttypes.h>
#else
#  define PRId64 "I64d"
#endif

#include <getopt.h>
#include <errno.h>

#include <oggz/oggz.h>
#include "oggz_tools.h"

/*#define DEBUG*/

#undef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))

static char * progname;
static FILE * outfile = NULL;
static int dump_bits = 0;
static int dump_char = 1;
static int dump_all_serialnos = 1;
static int truth = 1;

static int hide_offset = 0;
static int hide_serialno = 0;
static int hide_granulepos = 0;
static int hide_packetno = 0;

static void
usage (char * progname)
{
  printf ("Usage: %s [options] filename\n", progname);
  printf ("Hexdump packets of an Ogg file, or revert an Ogg file from such a hexdump\n");
  printf ("\nDump format options\n");
  printf ("  -b, --binary           Generate a binary dump of each packet\n");
  printf ("  -x, --hexadecimal      Generate a hexadecimal dump of each packet\n");
  printf ("\nFiltering options\n");
  printf ("  -n, --new              Only dump the first packet of each logical bitstream\n");
  printf ("  -s serialno, --serialno serialno\n");
  printf ("                         Dump only the logical bitstream with specified serialno\n");
  printf ("  -O, --hide-offset      Hide the byte offset of each packet\n");
  printf ("  -S, --hide-serialno    Hide the serialno field of each packet\n");
  printf ("  -G, --hide-granulepos  Hide the granulepos field of each packet\n");
  printf ("  -P, --hide-packetno    Hide the packetno field of each packet\n");
  printf ("\nMode options\n");
  printf ("  -r, --revert           Revert an oggzdump. Generates an Ogg bitstream\n");
  printf ("                         as prescribed in the input oggzdump\n");
  printf ("\nMiscellaneous options\n");
  printf ("  -o filename, --output filename\n");
  printf ("                         Specify output filename\n");
  printf ("  -h, --help             Display this help and exit\n");
  printf ("  -v, --version          Output version information and exit\n");
  printf ("\n");
  printf ("Please report bugs to <ogg-dev@xiph.org>\n");
}

static void
dump_char_line (unsigned char * buf, long n)
{
  int i;

  fprintf (outfile, "  ");

  for (i = 0; i < n; i++) {
    if (isgraph(buf[i])) fprintf (outfile, "%c", buf[i]);
    else if (isspace(buf[i])) fprintf (outfile, " ");
    else fprintf (outfile, ".");
  }
}

static void
hex_dump (unsigned char * buf, long n)
{
  int i;
  long remaining = n, count = 0;
  long rowlen;

  while (remaining > 0) {
    rowlen = MIN (remaining, 16);

    if (n > 0xffffff)
      fprintf (outfile, "%08lx:", count);
    else if (n > 0xffff)
      fprintf (outfile, "  %06lx:", count);
    else
      fprintf (outfile, "    %04lx:", count);

    for (i = 0; i < rowlen; i++) {
      if (!(i%2)) fprintf (outfile, " ");
      fprintf (outfile, "%02x", buf[i]);
    }

    for (; i < 16; i++) {
      if (!(i%2)) fprintf (outfile, " ");
      fprintf (outfile, "  ");
    }

    if (dump_char)
      dump_char_line (buf, rowlen);

    fprintf(outfile, "\n");

    remaining -= rowlen;
    buf += rowlen;
    count += rowlen;
  }
}

static void
bin_dump (unsigned char * buf, long n)
{
  int i, j;
  long remaining = n, count = 0;
  long rowlen;

  while (remaining > 0) {
    rowlen = MIN (remaining, 6);

    if (n > 0xffffff)
      fprintf (outfile, "%08lx:", count);
    else if (n > 0xffff)
      fprintf (outfile, "  %06lx:", count);
    else
      fprintf (outfile, "    %04lx:", count);

    for (i = 0; i < rowlen; i++) {
      fprintf (outfile, " ");
#ifdef WORDS_BIGENDIAN
      for (j = 0; j < 8; j++)
#else
      for (j = 7; j >= 0; j--)
#endif
	fprintf (outfile, "%c", (buf[i]&(1<<j)) ? '1' : '0');
    }

    for (; i < 6; i++) {
      if (!(i%2)) fprintf (outfile, " ");
      fprintf (outfile, "         ");
    }

    if (dump_char)
      dump_char_line (buf, rowlen);

    printf("\n");

    remaining -= rowlen;
    buf += rowlen;
    count += rowlen;
  }
}

/* FIXME: on Mac OS X, off_t is 64-bits.  Obviously we want a nicer
 * way to do it than this, but a quick fix is a good fix */
#ifdef __APPLE__
#  define PRI_off_t "q"
#else
#  define PRI_off_t "l"
#endif

static int
read_packet (OGGZ * oggz, ogg_packet * op, long serialno, void * user_data)
{
  ogg_int64_t units;
  double time_offset;

  units = oggz_tell_units (oggz);
  if (hide_offset) {
    fprintf (outfile, "oOo");
  } else if (units == -1) {
    fprintf (outfile, "%08" PRI_off_t "x", oggz_tell (oggz));
  } else {
    time_offset = (double)units / 1000.0;
    ot_fprint_time (outfile, time_offset);
  }

  fprintf (outfile, ": serialno %010ld, "
	   "granulepos %" PRId64 ", packetno %" PRId64,
	   hide_serialno ? -1 : serialno,
	   hide_granulepos ? -1 : op->granulepos,
	   hide_packetno ? -1 : op->packetno);

  if (op->b_o_s) {
    fprintf (outfile, " *** bos");
  }

  if (op->e_o_s) {
    fprintf (outfile, " *** eos");
  }

  fprintf (outfile, ": ");
  ot_fprint_bytes (outfile, op->bytes);
  fputc ('\n', outfile);

  if (dump_bits) {
    bin_dump (op->packet, op->bytes);
  } else {
    hex_dump (op->packet, op->bytes);
  }

  fprintf (outfile, "\n");

  return 0;
}

static int
ignore_packet (OGGZ * oggz, ogg_packet * op, long serialno, void * user_data)
{
  return -1;
}

static int
read_new_packet (OGGZ * oggz, ogg_packet * op, long serialno, void * user_data)
{
  oggz_set_read_callback (oggz, serialno, ignore_packet, NULL);
  read_packet (oggz, op, serialno, user_data);

  return -1;
}

static void
revert_file (char * infilename)
{
  OGGZ * oggz;
  FILE * infile;
  char line[80];
  int hh, mm, ss;
  unsigned int offset;
  long current_serialno = -1, serialno;
  ogg_int64_t granulepos, packetno;
  int bos = 0, eos = 0;

  int line_offset = 0, consumed = 0;

  unsigned char * packet = NULL;
  long max_bytes = 0;

  unsigned char buf[1024];
  ogg_packet op;
  int flush = 1;
  long n;
  char c;

  if (strcmp (infilename, "-") == 0) {
    infile = stdin;
  } else {
    infile = fopen (infilename, "rb");
  }

  oggz = oggz_new (OGGZ_WRITE|OGGZ_NONSTRICT);

  while (fgets (line, 80, infile)) {
    line_offset = 0;

    /* Skip time offsets, OR ensure line_offset is 0 */
    if (sscanf (line, "%2d:%2d:%2d.%n", &hh, &mm, &ss, &line_offset) < 3)
      line_offset = 0;

    if (sscanf (&line[line_offset], "%x: serialno %ld, granulepos %lld, packetno %lld%n",
		&offset, &serialno, &granulepos, &packetno,
		&line_offset) >= 4) {

      /* flush any existing packets */
      if (current_serialno != -1) {
	int ret;

#ifdef DEBUG
	printf ("feeding packet (%010ld) %ld bytes\n",
		current_serialno, op.bytes);
#endif
	if ((ret = oggz_write_feed (oggz, &op, current_serialno, flush, NULL)) != 0) {
	  fprintf (stderr, "%s: oggz_write_feed error %d\n", progname, ret);
	}

	while ((n = oggz_write_output (oggz, buf, 1024)) > 0) {
	  fwrite (buf, 1, n, outfile);
	}
      }

      /* Start new packet */
      bos = 0; eos = 0;
      if (sscanf (&line[line_offset], " *** %[b]%[o]%[s]%n", &c, &c, &c,
		  &consumed) >= 3) {
	bos = 1;
	line_offset += consumed;
      }
      if (sscanf (&line[line_offset], " *** %[e]%[o]%[s]%n", &c, &c, &c,
		  &consumed) >= 3) {
	eos = 1;
	line_offset += consumed;
      }

      current_serialno = serialno;

      op.packet = packet;
      op.bytes = 0;
      op.b_o_s = bos;
      op.e_o_s = eos;
      op.granulepos = granulepos;
      op.packetno = packetno;

    } else {
      int nread = 0;
      unsigned int val = 0;
      unsigned int offset;

      if (current_serialno != -1 &&
	  sscanf (line, "%x:%n", &offset, &line_offset) >= 1) {
	while (nread < 16 &&
	       (sscanf (&line[line_offset], "%2x%n", &val, &consumed) > 0)) {
	  op.bytes++;
	  if (op.bytes > max_bytes) {
	    unsigned char * new_packet;
	    size_t new_size;

	    if (max_bytes == 0) {
	      new_size = 128;
	    } else {
	      new_size = max_bytes * 2;
	    }

	    new_packet =
	      (unsigned char *) realloc ((void *)packet, new_size);
	    if (new_packet == NULL) {
	      fprintf (stderr,
		       "%s: error allocating memory for packet data\n",
		       progname);
	      exit (1);
	    } else {
	      max_bytes = (long)new_size;
	      packet = new_packet;
	      op.packet = packet;
	    }
	  }

	  packet[op.bytes-1] = (unsigned char) val;

	  line_offset += consumed;
	  nread++;
	}
      }
    }
  }

  fclose (infile);
}

int
main (int argc, char ** argv)
{
  int show_version = 0;
  int show_help = 0;

  OGGZ * oggz;
  char * infilename = NULL, * outfilename = NULL;
  int revert = 0;
  OggzTable * table = NULL;
  long serialno;
  OggzReadPacket my_read_packet = read_packet;
  int i, size;
  long n;

  ot_init ();

  progname = argv[0];

  if (argc < 2) {
    usage (progname);
    return (1);
  }

  table = oggz_table_new();

  while (1) {
    char * optstring = "hvbxnro:s:OSGP";

#ifdef HAVE_GETOPT_LONG
    static struct option long_options[] = {
      {"help", no_argument, 0, 'h'},
      {"version", no_argument, 0, 'v'},
      {"binary", no_argument, 0, 'b'},
      {"hexadecimal", no_argument, 0, 'x'},
      {"new", no_argument, 0, 'n'},
      {"revert", no_argument, 0, 'r'},
      {"output", required_argument, 0, 'o'},
      {"serialno", required_argument, 0, 's'},
      {"hide-offset", no_argument, 0, 'O'},
      {"hide-serialno", no_argument, 0, 'S'},
      {"hide-granulepos", no_argument, 0, 'G'},
      {"hide-packetno", no_argument, 0, 'P'},
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
    case 'h': /* help */
      show_help = 1;
      break;
    case 'v': /* version */
      show_version = 1;
      break;
    case 'b': /* binary */
      dump_bits = 1;
      break;
    case 'n': /* new */
      my_read_packet = read_new_packet;
      break;
    case 'o': /* output */
      outfilename = optarg;
      break;
    case 'r': /* revert */
      revert = 1;
      break;
    case 's': /* serialno */
      dump_all_serialnos = 0;
      serialno = atol (optarg);
      oggz_table_insert (table, serialno, &truth);
      break;
    case 'O': /* hide offset */
      hide_offset = 1;
      break;
    case 'S': /* hide serialno */
      hide_serialno = 1;
      break;
    case 'G': /* hide granulepos */
      hide_granulepos = 1;
      break;
    case 'P': /* hide packetno */
      hide_packetno = 1;
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

  if (outfilename == NULL) {
    outfile = stdout;
  } else {
    outfile = fopen (outfilename, "wb");
    if (outfile == NULL) {
      fprintf (stderr, "%s: unable to open output file %s\n",
	       progname, outfilename);
      goto exit_err;
    }
  }

  if (revert) {
    if (dump_bits) {
      fprintf (stderr, "%s: Revert of binary dump not supported\n", progname);
      goto exit_err;
    }

    revert_file (infilename);
  } else {
    errno = 0;

    if (strcmp (infilename, "-") == 0) {
      oggz = oggz_open_stdio (stdin, OGGZ_READ|OGGZ_AUTO);
    } else {
      oggz = oggz_open (infilename, OGGZ_READ|OGGZ_AUTO);
    }

    if (oggz == NULL) {
      if (errno == 0) {
	fprintf (stderr, "%s: %s: OGGZ error opening input file\n",
		 progname, infilename);
      } else {
	fprintf (stderr, "%s: %s: %s\n",
		 progname, infilename, strerror (errno));
      }
      goto exit_err;
    }

    if (dump_all_serialnos) {
      oggz_set_read_callback (oggz, -1, my_read_packet, NULL);
    } else {
      size = oggz_table_size (table);
      for (i = 0; i < size; i++) {
	oggz_table_nth (table, i, &serialno);
	oggz_set_read_callback (oggz, serialno, my_read_packet, NULL);
      }
    }

    while ((n = oggz_read (oggz, 1024)) > 0);

    oggz_close (oggz);
  }

 exit_ok:
  oggz_table_delete (table);
  exit (0);

 exit_err:
  oggz_table_delete (table);
  exit (1);
}
