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
#include <ctype.h>
#include <inttypes.h>

#include <oggz/oggz.h>

#undef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))

static void hex_dump (unsigned char * buf, long n) {
  int i;
  long remaining = n, count = 0;
  long rowlen;

  while (remaining > 0) {
    rowlen = MIN (remaining, 16);

    if (n > 0xffffff)
      printf ("%08lx:", count);
    else if (n > 0xffff)
      printf ("  %06lx:", count);
    else
      printf ("    %04lx:", count);

    for (i = 0; i < rowlen; i++) {
      if (!(i%2)) printf (" ");
      printf ("%02x", buf[i]);
    }

    for (; i < 16; i++) {
      if (!(i%2)) printf (" ");
      printf ("  ");
    }

    printf ("  ");

    for (i = 0; i < rowlen; i++) {
      if (isgraph(buf[i])) printf ("%c", buf[i]);
      else if (isspace(buf[i])) printf (" ");
      else printf (".");
    }

    printf("\n");

    remaining -= rowlen;
    buf += rowlen;
    count += rowlen;
  }
}

static void bin_dump (unsigned char * buf, long n) {
  int i, j;
  long remaining = n, count = 0;
  long rowlen;

  while (remaining > 0) {
    rowlen = MIN (remaining, 6);

    if (n > 0xffffff)
      printf ("%08lx:", count);
    else if (n > 0xffff)
      printf ("  %06lx:", count);
    else
      printf ("    %04lx:", count);

    for (i = 0; i < rowlen; i++) {
      printf (" ");
#ifdef WORDS_BIGENDIAN
      for (j = 0; j < 8; j++)
#else
      for (j = 7; j >= 0; j--)
#endif
	printf ("%c", (buf[i]&(1<<j)) ? '1' : '0');
    }

    for (; i < 6; i++) {
      if (!(i%2)) printf (" ");
      printf ("         ");
    }

    printf ("  ");

    for (i = 0; i < rowlen; i++) {
      if (isgraph(buf[i])) printf ("%c", buf[i]);
      else if (isspace(buf[i])) printf (" ");
      else printf (".");
    }

    printf("\n");

    remaining -= rowlen;
    buf += rowlen;
    count += rowlen;
  }
}

static int
read_packet (OGGZ * oggz, ogg_packet * op, long serialno, void * user_data)
{
  long position = oggz_tell(oggz);

  printf ("%08lx: serialno %010ld, "
	  "granulepos %" PRId64 ", packetno %" PRId64,
	  position, serialno, op->granulepos, op->packetno);

  if (op->b_o_s) {
    printf (" *** bos");
  }

  if (op->e_o_s) {
    printf (" *** eos");
  }

  printf (":\n");

  hex_dump (op->packet, op->bytes);

  printf ("\n");

  return 0;
}

static int
read_bos_packet (OGGZ * oggz, ogg_packet * op, long serialno, void * user_data)
{
  if (!op->b_o_s) {
    oggz_set_read_callback (oggz, -1, NULL, NULL);
    return 1;
  }

  return read_packet (oggz, op, serialno, user_data);
}

int
main (int argc, char ** argv)
{
  OGGZ * oggz;
  long n;

  if (argc < 2) {
    printf ("usage: %s filename\n", argv[0]);
  }

  if ((oggz = oggz_open ((char *)argv[1], OGGZ_READ)) == NULL) {
    printf ("unable to open file %s\n", argv[1]);
    exit (1);
  }

  oggz_set_read_callback (oggz, -1, read_packet, NULL);
  while ((n = oggz_read (oggz, 1024)) > 0);

  oggz_close (oggz);

  exit (0);
}
