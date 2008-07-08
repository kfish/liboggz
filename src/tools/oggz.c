/*
   Copyright (C) 2008 Annodex Association

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
#include <unistd.h>

/* #define DEBUG */

#define TOOLNAME_LEN 16

int
usage (char * progname)
{
  printf ("Usage: oggz [--help] COMMAND [ARGS]\n\n");

  printf ("The most commonly used oggz commands are:\n\n");

  printf ("  chop          Extract the part of an Ogg file between given start\n"
          "                and/or end times.\n");

  printf ("  comment       List or edit comments in an Ogg file.\n");

  printf ("  diff          Hexdump the packets of two Ogg files and output\n"
          "                differences.\n");

  printf ("  dump          Hexdump packets of an Ogg file, or revert an Ogg file\n"
          "                from such a hexdump.\n");

  printf ("  info          Display information about one or more Ogg files and\n"
          "                their bitstreams.\n");

  printf ("  merge         Merge Ogg files together, interleaving pages in order\n"
          "                of presentation time.\n");

  printf ("  rip           Extract one or more logical bitstreams from an Ogg file.\n");

  printf ("  scan          Scan an Ogg file and output characteristic landmarks.\n");

  printf ("  sort          Sort the pages of an Ogg file in order of presentation\n"
          "                time.\n");

  printf ("  validate      Validate the Ogg framing of one or more files.\n");

  return 0;
}

int
main (int argc, char ** argv)
{
  char * progname = argv[0];
  char toolname[TOOLNAME_LEN];

  if (argc < 2) {
     usage (progname);
  } else {
    if (!strncmp (argv[1], "help", 4) || !strncmp(argv[1], "--help", 6)) {
      if (argc == 2) {
        usage (progname);
      } else {
        sprintf (toolname, "oggz-%s", argv[2]);
#ifdef _WIN32
        argv[1] = toolname;
        argv[2] = "--help";
        execvp (toolname, &argv[1]);
#else
        argv[1] = "man";
        argv[2] = toolname;
        execvp ("man", &argv[1]);
#endif
      }
    } else {
      sprintf (toolname, "oggz-%s", argv[1]);
      argv[1] = toolname;
      execvp (toolname, &argv[1]);
    }
  }

  exit (0);
}
