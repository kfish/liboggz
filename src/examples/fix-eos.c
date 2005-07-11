/*
   Copyright (C) 2003 Commonwealth Scientific and Industrial Research
   Organisation (CSIRO) Australia
   Also (C) 2005 Michael Smith <msmith@xiph.org>

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
#include <oggz/oggz.h>
#include <ogg/ogg.h>

static FILE *out;

static char * progname;

static void clear_table(OggzTable *table) {
  int i, size = oggz_table_size(table);
  for(i = 0; i < size; i++) {
    free(oggz_table_nth(table, i, NULL));
  }
}

static int
read_page (OGGZ * oggz, const ogg_page * og, long serialno, void * user_data)
{
  OggzTable * tracks = (OggzTable *)user_data;
  long pageno = ogg_page_pageno(og);
  long *data = (long *)oggz_table_lookup(tracks, serialno);
  if(data == NULL) {
    data = malloc(sizeof(long));
  }
  *data = pageno;
  oggz_table_insert (tracks, serialno, data);

  return 0;
}

static int
write_page (OGGZ * oggz, const ogg_page * og, long serialno, void * user_data)
{
  OggzTable * tracks = (OggzTable *)user_data;
  long pageno = ogg_page_pageno(og);
  long *data = (long *)oggz_table_lookup(tracks, serialno);

  if(data == NULL) {
    fprintf(stderr, "%s: Bailing out, internal consistency failure\n", progname);
    abort();
  }

  if(*data == pageno) {
    unsigned char header_type = og->header[5];
    if(!(header_type & 0x4)) {
      fprintf(stderr, "%s: Setting EOS on final page of stream %ld\n",
		      progname, serialno);
      header_type |= 0x4;
      og->header[5] = header_type;

      ogg_page_checksum_set(og);
    }
  }

  fwrite (og->header, 1, og->header_len, out);
  fwrite (og->body, 1, og->body_len, out);

  return 0;
}

int
main (int argc, char ** argv)
{
  OGGZ * oggz;
  OggzTable * tracks;
  long n;

  progname = argv[0];

  if (argc < 3) {
    printf ("usage: %s in.ogg out.ogg\n", progname);
  }

  tracks = oggz_table_new ();

  if ((oggz = oggz_open ((char *)argv[1], OGGZ_READ | OGGZ_AUTO)) == NULL) {
    printf ("%s: unable to open file %s\n", progname, argv[1]);
    exit (1);
  }

  oggz_set_read_page (oggz, -1, read_page, tracks);

  while ((n = oggz_read (oggz, 1024)) > 0);

  oggz_close (oggz);

  out = fopen(argv[2], "wb");
  if(!out) {
    fprintf(stderr, "%s: Failed to open output file \"%s\"\n", progname, argv[2]);
    exit(1);
  }


  if ((oggz = oggz_open ((char *)argv[1], OGGZ_READ | OGGZ_AUTO)) == NULL) {
    printf ("%s: unable to open file %s\n", progname, argv[1]);
    exit (1);
  }

  oggz_set_read_page (oggz, -1, write_page, tracks);

  while ((n = oggz_read (oggz, 1024)) > 0);

  oggz_close (oggz);

  clear_table(tracks);
  oggz_table_delete (tracks);

  fclose(out);

  exit (0);
}
