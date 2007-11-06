/*
   Copyright (C) 2007 Annodex Association

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

/* Kangyuan Niu: original version (Aug 2007) */

#include "config.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <oggz/oggz.h>
#include "oggz_tools.h"

/* #define DEBUG */

static char * progname;

static void
usage (char * progname)
{
  printf ("Usage: %s [options] filename\n", progname);
  printf ("List or edit comments in an Ogg file.\n");
  printf ("\nOutput options\n");
  printf ("  -l, --list            List the comments in the given file.\n");
  printf ("\nEditing options\n");
  printf ("  -o filename, --output filename\n");
  printf ("                         Specify output filename\n");
  printf ("  -s, --set              Set a comment\n");
  printf ("\nMiscellaneous options\n");
  printf ("  -h, --help             Display this help and exit\n");
  printf ("  -v, --version          Output version information and exit\n");
  printf ("\n");
  printf ("Please report bugs to <ogg-dev@xiph.org>\n");
}

int get_stream_types(OGGZ *oggz, ogg_packet *op,
		     long serialno, void* user_data) {
  OggzTable *table = (OggzTable *)user_data;
  char *content;
  if(!oggz_table_lookup(table, serialno)) {
    content = oggz_stream_get_content_type(oggz, serialno);
    oggz_table_insert(table, serialno, content);
  }
  return 0;
}

int copy_replace_comments(OGGZ *oggz, ogg_packet *op,
			  long serialno, void *user_data) {
  OGGZ *oggz_write = (OGGZ *)user_data;
  ogg_packet *comment_packet;
  OggzStreamContent content = oggz_stream_get_content(oggz, serialno);
  int flush=0;
/*
  if (op->granulepos == -1) {
    flush = 0;
  } else {
    flush = OGGZ_FLUSH_AFTER;
  }
*/
  if(op->packetno == 1) {
    oggz_comment_set_vendor(oggz_write, serialno,
			    oggz_comment_get_vendor(oggz, serialno));
    comment_packet = oggz_comment_generate(oggz_write, serialno, content, 0);
    oggz_write_feed(oggz_write, comment_packet, serialno, flush, NULL);
  } else {
    oggz_write_feed(oggz_write, op, serialno, flush, NULL);
  }
  return 0;
}

void list_comments(OGGZ *oggz, long serialno) {
  OggzComment *comment;
  printf("\tVendor: ");
  printf(oggz_comment_get_vendor(oggz, serialno));
  printf("\n");
  comment = oggz_comment_first(oggz, serialno);
  while(comment) {
    printf("\t");
    printf(comment->name);
    printf(": ");
    printf(comment->value);
    printf("\n");
    comment = oggz_comment_next(oggz, serialno, comment);
  }
}

void list_comments_all(OGGZ *oggz) {
  int i;
  long serialno;
  OggzTable *table = oggz_table_new();
  oggz_set_read_callback(oggz, -1, get_stream_types, table);
  oggz_run(oggz);
  for(i = 0; i < oggz_table_size(table); i++) {
    printf(oggz_table_nth(table, i, &serialno));
    printf(" (serialno = %ld):\n", serialno);
    list_comments(oggz, serialno);
  }
}

void edit_comments(OGGZ *oggz, long serialno, OggzComment *comments) {
  int i;
  for(i = 0; strcmp(comments[i].name, "0") != 0; i++) {
    printf(comments[i].name);
    oggz_comment_remove_byname(oggz, serialno, comments[i].name);
    printf(": ");
    printf(comments[i].value);
    oggz_comment_add(oggz, serialno, &comments[i]);
  }
}

OggzComment parse_comment_field(char *arg) {
  int i;
  char *c;
  OggzComment comment;
  comment.name = strcpy(calloc(strlen(arg) + 1, sizeof(char)), arg);
  c = strchr(comment.name, '=');
  *c = '\0';
  for(i = 0; comment.name[i]; i++) {
    if(islower(arg[i]))
      comment.name[i] = toupper(arg[i]);
  }
  comment.value = c + 1;
  return comment;
}

void version() {
  printf ("%s version " VERSION "\n", progname);
}

int main(int argc, char *argv[]) {
  int i, index;
  long n, nout, serialno = 0;
  unsigned char buffer[1024];
  char *out_filename;
  FILE *out_file;
  OGGZ *oggz_in;
  OGGZ *oggz_out;
  OggzComment *comments;
  OggzTable *table = oggz_table_new();

  progname = argv[0];

  if(argc < 2) {
    usage (progname);
    return 1;
  }

  if(strcmp(argv[1], "--version") == 0
     || strcmp(argv[1], "-v") == 0) {
    version();
    return 0;
  } else if(strcmp(argv[1], "--help") == 0
	    || strcmp(argv[1], "-h") == 0) {
    usage (progname);
    return 0;
  } else {
    oggz_in = oggz_open(argv[1], OGGZ_READ);
    oggz_out = oggz_new(OGGZ_WRITE);
    out_filename = argv[1];
  }

  comments = calloc(argc - 1, sizeof(OggzComment));
  comments[index = 0].name = "0";
  for(i = 2; i < argc; i++) {
    if(strcmp(argv[i], "-l") == 0
	      || strcmp(argv[i], "--list") == 0)
      list_comments_all(oggz_in);
    else if(strcmp(argv[i], "-o") == 0
	      || strcmp(argv[i], "--list") == 0)
      out_filename = argv[++i];
    else if(strchr(argv[i], '=') != NULL) {
      comments[index] = parse_comment_field(argv[i]);
      comments[++index].name = "0";
    } else if(strncmp(argv[i], "-s", 2) == 0) {
      if(strcmp(comments[0].name, "0") != 0) {
	oggz_table_insert(table, serialno, comments);
	comments = calloc(argc - 2, sizeof(OggzComment));
	comments[index = 0].name = "0";
      }
      serialno = strtol(&argv[i][2], NULL, 10);
    } else {
      printf("Error: option or field \"");
      printf(argv[i]);
      printf("\" unrecognized.\n");
      return 0;
    }
  }
  if(strcmp(comments[0].name, "0") != 0)
    oggz_table_insert(table, serialno, comments);

  if(oggz_table_size(table) != 0) {
    for(i = 0; i < oggz_table_size(table); i++) {
      comments = oggz_table_nth(table, i, &serialno);
      edit_comments(oggz_out, serialno, comments);
    }
    oggz_set_read_callback(oggz_in, -1, copy_replace_comments, oggz_out);
    out_file = fopen(out_filename, "w");

    while((n = oggz_read(oggz_in, 1024)) != 0) {
      while((nout = oggz_write_output(oggz_out, buffer, n)) > 0) {
	fwrite(buffer, 1, n, out_file);
      }
    }

  } else if(oggz_in)
    list_comments_all(oggz_in);
  else {
    printf("Error: file \"");
    printf(argv[1]);
    printf("\" could not be opened.\n");
  }

  oggz_close(oggz_in);
  oggz_close(oggz_out);
  return 0;
}
