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
#include <strings.h>

#include <oggz/oggz.h>
#include "oggz_tools.h"

/* #define DEBUG */

static char * progname;

static void
usage (char * progname)
{
  printf ("Usage: %s filename [options] tagname=tagvalue ...\n", progname);
  printf ("List or edit comments in an Ogg file.\n");
  printf ("\nOutput options\n");
  printf ("  -l, --list            List the comments in the given file.\n");
  printf ("\nEditing options\n");
  printf ("  -o filename, --output filename\n");
  printf ("                         Specify output filename\n");
  printf ("  -d, --delete           Delete comments before editing\n");
  printf ("  -a, --all              Edit comments for all logical bitstreams\n");
  printf ("  -c content-type, --content-type content-type\n");
  printf ("                         Edit comments of the logical bitstreams with\n");
  printf ("                         specified content-type\n");
  printf ("  -s serialno, --serialno serialno\n");
  printf ("                         Edit comments of the logical bitstream with\n");
  printf ("                         specified serialno\n");
  printf ("\nMiscellaneous options\n");
  printf ("  -h, --help             Display this help and exit\n");
  printf ("  -v, --version          Output version information and exit\n");
  printf ("\n");
  printf ("Please report bugs to <ogg-dev@xiph.org>\n");
}

int copy_replace_comments(OGGZ *oggz,
                         ogg_packet *op,
                         long serialno,
                         void *user_data) {
  OGGZ *oggz_write = (OGGZ *)user_data;
  int flush;
  if(op->granulepos == -1)
    flush = 0;
  else
    flush = OGGZ_FLUSH_AFTER;
  if(op->packetno == 1) {
    oggz_write_feed(oggz_write,
                   oggz_comment_generate(oggz_write, serialno,
                                         oggz_stream_get_content(oggz, serialno),
                                         0),
                   serialno, flush, NULL);
  } else
    oggz_write_feed(oggz_write, op, serialno, flush, NULL);
  return 0;
}

int copy_comments(OGGZ *oggz,
                 ogg_packet *op,
                 long serialno,
                 void *user_data) {
  OGGZ *oggz_write = (OGGZ *)user_data;
  OggzComment * comment;
  if(op->packetno == 1) {
    oggz_comment_set_vendor(oggz_write, serialno,
                           oggz_comment_get_vendor(oggz, serialno));
    for(comment = oggz_comment_first(oggz, serialno); comment;
       comment = oggz_comment_next(oggz, serialno, comment))
      oggz_comment_add(oggz_write, serialno, comment);
  }
  return 0;
}

int list_comments(OGGZ *oggz,
                 ogg_packet *op, 
                 long serialno,
                 void *user_data) {
  const OggzComment * comment;
  if(op->packetno == 1) {
    printf("%s (serial = %ld):\n",
          oggz_stream_get_content_type(oggz, serialno), serialno);
    printf("\tVendor: %s\n", oggz_comment_get_vendor(oggz, serialno));
    for (comment = oggz_comment_first(oggz, serialno); comment;
        comment = oggz_comment_next(oggz, serialno, comment))
      printf ("\t%s: %s\n", comment->name, comment->value);
  }
  return 0;
}

int get_stream_types(OGGZ *oggz,
                    ogg_packet *op,
                    long serialno,
                    void *user_data) {
  OggzTable *table = (OggzTable *)user_data;
  OggzStreamContent *content = malloc(sizeof(OggzStreamContent));
  if(oggz_table_lookup(table, serialno) == NULL) {
    *content = oggz_stream_get_content(oggz, serialno);
    oggz_table_insert(table, serialno, content);
  }
  return 0;
}

void edit_comments(OGGZ *oggz,
                  long serialno,
                  OggzComment *comments) {
  int i;
  for(i = 0; strcmp(comments[i].name, "0"); i++) {
    oggz_comment_remove_byname(oggz, serialno, comments[i].name);
    oggz_comment_add(oggz, serialno, &comments[i]);
  }
}

int comment_table_insert(OggzTable *type_table,
                        OggzTable *comment_table,
                        long serialno,
                        OggzComment *comments) {
  OggzStreamContent type;
  long type_serialno;
  int i;
  if(!strcmp(comments[0].name, "0"))
    return 0;
  if(serialno > 0) {
    oggz_table_insert(comment_table, serialno, comments);
  } else if(serialno > -11) {
    for(i = 0; i < oggz_table_size(type_table); i++) {
      type = *(OggzStreamContent *)oggz_table_nth(type_table, i, &type_serialno);
      if(type == serialno * -1)
       oggz_table_insert(comment_table, type_serialno, comments);
    }
  } else {
    for(i = 0; i < oggz_table_size(type_table); i++) {
      oggz_table_nth(type_table, i, &type_serialno);
      oggz_table_insert(comment_table, type_serialno, comments);
    }
  }
  return 1;
}

OggzComment parse_comment_field(char *arg) {
  int i;
  char *c;
  OggzComment comment;
  comment.name = strcpy(calloc(strlen(arg) + 1, sizeof(char)), arg);
  c = strchr(comment.name, '=');
  *c = '\0';
  for(i = 0; comment.name[i]; i++)
    if(islower(arg[i]))
      comment.name[i] = toupper(arg[i]);
  comment.value = c + 1;
  return comment;
}

OggzStreamContent strto_oggz_content(char *type) {
  if(!strcasecmp(type, "theora"))
    return OGGZ_CONTENT_THEORA;
  if(!strcasecmp(type, "vorbis"))
    return OGGZ_CONTENT_VORBIS;
  if(!strcasecmp(type, "speex"))
    return OGGZ_CONTENT_SPEEX;
  if(!strcasecmp(type, "pcm"))
    return OGGZ_CONTENT_PCM;
  if(!strcasecmp(type, "cmml"))
    return OGGZ_CONTENT_CMML;
  if(!strcasecmp(type, "anx2"))
    return OGGZ_CONTENT_ANX2;
  if(!strcasecmp(type, "skeleton"))
    return OGGZ_CONTENT_SKELETON;
  if(!strcasecmp(type, "flac0"))
    return OGGZ_CONTENT_FLAC0;
  if(!strcasecmp(type, "flac"))
    return OGGZ_CONTENT_FLAC;
  if(!strcasecmp(type, "anxdata"))
    return OGGZ_CONTENT_ANXDATA;
  return OGGZ_CONTENT_UNKNOWN;
}

void version() {
  printf ("%s version " VERSION "\n", progname);
}


int main(int argc, char *argv[]) {
  int i, temp, clear = 0;
  long n, serialno = -11;
  char *out_file;
  OGGZ *oggz_in;
  OGGZ *oggz_out;
  OggzComment *comments;
  OggzTable *type_table = oggz_table_new();
  OggzTable *comment_table = oggz_table_new();

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
    usage(progname);
    return 0;
  } else {
    oggz_in = oggz_open(argv[1], OGGZ_READ);
    out_file = argv[1];
  }

  oggz_set_read_callback(oggz_in, -1, get_stream_types, type_table);
  oggz_run(oggz_in);
  comments = calloc(argc - 1, sizeof(OggzComment));
  comments[temp = 0].name = "0";
  for(i = 2; i < argc; i++) {
    if(!strcmp(argv[i], "-o"))
      out_file = argv[++i];
    else if(!strcmp(argv[i], "-d")
           || !strcmp(argv[i], "--delete"))
      clear = 1;
    else if(!strcmp(argv[i], "-l")
           || !strcmp(argv[i], "--list")) {
      oggz_seek(oggz_in, 0, SEEK_SET);
      oggz_set_read_callback(oggz_in, -1, list_comments, NULL);
      oggz_run(oggz_in);
    } else if(strchr(argv[i], '=') != NULL) {
      comments[temp] = parse_comment_field(argv[i]);
      comments[++temp].name = "0";
    } else if(!strcmp(argv[i], "-a")) {
      if(comment_table_insert(type_table, comment_table, serialno, comments)) {
       comments = calloc(argc - 2, sizeof(OggzComment));
       comments[temp = 0].name = "0";
      }
      serialno = -11;
    } else if(!strcmp(argv[i], "-c")
           || !strcmp(argv[i], "--content-type")) {
      if(comment_table_insert(type_table, comment_table, serialno, comments)) {
       comments = calloc(argc - 2, sizeof(OggzComment));
       comments[temp = 0].name = "0";
      }
      serialno = strto_oggz_content(argv[++i]) * -1;
    } else if(!strcmp(argv[i], "-s")
           || !strcmp(argv[i], "--serialno")) {
      if(comment_table_insert(type_table, comment_table, serialno, comments)) {
       comments = calloc(argc - 2, sizeof(OggzComment));
       comments[temp = 0].name = "0";
      }
      serialno = strtol(argv[++i], NULL, 10);
    } else {
      printf("Error: option or field \"");
      printf(argv[i]);
      printf("\" unrecognized.\n");
      return 0;
    }
  }
  comment_table_insert(type_table, comment_table, serialno, comments);

  if(oggz_table_size(comment_table)) {
    temp = 0;
    if(!strcmp(out_file, argv[1])) {
      out_file = tmpnam(NULL);
      temp = 1;
    }
    oggz_out = oggz_open(out_file, OGGZ_WRITE);
    if(!clear) {
      oggz_seek(oggz_in, 0, SEEK_SET);
      oggz_set_read_callback(oggz_in, -1, copy_comments, oggz_out);
      oggz_run(oggz_in);
    }
    for(i = 0; i < oggz_table_size(comment_table); i++) {
      comments = oggz_table_nth(comment_table, i, &serialno);
      edit_comments(oggz_out, serialno, comments);
    }
    oggz_seek(oggz_in, 0, SEEK_SET);
    oggz_set_read_callback(oggz_in, -1, copy_replace_comments, oggz_out);
    while((n = oggz_read(oggz_in, 1024)) > 0)
      while(oggz_write(oggz_out, n) > 0);
    if(temp) {
      remove(argv[1]);
      rename(out_file, argv[1]);
    }
    oggz_close(oggz_out);
  } else if(oggz_in) {
    oggz_seek(oggz_in, 0, SEEK_SET);
    oggz_set_read_callback(oggz_in, -1, list_comments, NULL);
    oggz_run(oggz_in);
  } else {
    printf("Error: file \"");
    printf(argv[1]);
    printf("\" could not be opened.\n");
  }

  oggz_close(oggz_in);
  return 0;
}
