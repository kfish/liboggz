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

#include "oggz_tests.h"

#define ARTIST1 "Trout Junkies"
#define ARTIST2 "DJ Fugu"
#define COPYRIGHT "Copyright (C) 2004. Some Rights Reserved."
#define LICENSE "Creative Commons Attribute Share-Alike v1.0"
#define COMMENT "Unstructured comments are evil."

static OGGZ * oggz;

int
main (int argc, char * argv[])
{
  const OggzComment * comment, * comment2;
  OggzComment mycomment;
  int err;

  long serialno = 7;

#if OGGZ_CONFIG_WRITE
  INFO ("Initializing OGGZ for comments (writer)");
  oggz = oggz_new (OGGZ_WRITE);

  INFO ("+ Adding ARTIST1 byname");
  err = oggz_comment_add_byname (oggz, serialno, "ARTIST", ARTIST1);
  if (err == OGGZ_ERR_BAD_SERIALNO)
    FAIL ("Comment add to fresh bitstream failed");
  if (err < 0) FAIL ("Operation failed");

  INFO ("+ Testing add of invalid unstructured COMMENT byname");
  err = oggz_comment_add_byname (oggz, serialno, COMMENT, NULL);
  if (err != OGGZ_ERR_COMMENT_INVALID)
    FAIL ("Invalid comment not detected");

  INFO ("+ Testing add of invalid unstructured COMMENT from local storage");
  mycomment.name = COMMENT;
  mycomment.value = NULL;
  err = oggz_comment_add (oggz, serialno, &mycomment);
  if (err != OGGZ_ERR_COMMENT_INVALID)
    FAIL ("Invalid comment not detected");

  INFO ("+ Adding COPYRIGHT byname");
  err = oggz_comment_add_byname (oggz, serialno, "COPYRIGHT", COPYRIGHT);
  if (err < 0) FAIL ("Operation failed");

  INFO ("+ Retrieving first (expect ARTIST1)");
  comment = oggz_comment_first (oggz, serialno);

  if (comment == NULL)
    FAIL ("Recently inserted ARTIST1 not retrieved");

  if (strcmp (comment->name, "ARTIST"))
    FAIL ("Incorrect ARTIST1 name found");

  if (strcmp (comment->value, ARTIST1))
    FAIL ("Incorrect ARTIST1 value found");

  INFO ("+ Retrieving next (expect COPYRIGHT)");
  comment = oggz_comment_next (oggz, serialno, comment);

  if (comment == NULL)
    FAIL ("Recently inserted COPYRIGHT not retrieved");

  if (strcmp (comment->name, "COPYRIGHT"))
    FAIL ("Incorrect COPYRIGHT name found");

  if (strcmp (comment->value, COPYRIGHT))
    FAIL ("Incorrect COPYRIGHT value found");

  INFO ("+ Checking comments termination");
  comment2 = oggz_comment_next (oggz, serialno, comment);

  if (comment2 != NULL)
    FAIL ("Comments unterminated");

  INFO ("+ Adding LICENSE from local storage");
  mycomment.name = "LICENSE";
  mycomment.value = LICENSE;
  err = oggz_comment_add (oggz, serialno, &mycomment);
  if (err < 0) FAIL ("Operation failed");

  INFO ("+ Retrieving next (expect LICENSE)");
  comment = oggz_comment_next (oggz, serialno, comment);

  if (comment == NULL)
    FAIL ("Recently inserted LICENSE not retrieved");

  if (comment == &mycomment)
    FAIL ("Recently inserted LICENSE not restored");

  if (strcmp (comment->name, "LICENSE"))
    FAIL ("Incorrect LICENSE name found");

  if (strcmp (comment->value, LICENSE))
    FAIL ("Incorrect LICENSE value found");

  INFO ("+ Adding ARTIST2 byname");  
  err = oggz_comment_add_byname (oggz, serialno, "ARTIST", ARTIST2);
  if (err < 0) FAIL ("Operation failed");

  INFO ("+ Retrieving first ARTIST using wierd caps (expect ARTIST1)");
  comment = oggz_comment_first_byname (oggz, serialno, "ArTiSt");

  if (comment == NULL)
    FAIL ("Recently inserted ARTIST1 not retrieved");

  if (strcmp (comment->name, "ARTIST"))
    FAIL ("Incorrect ARTIST1 name found");

  if (strcmp (comment->value, ARTIST1))
    FAIL ("Incorrect ARTIST1 value found");

  INFO ("+ Retrieving next ARTIST (expect ARTIST2)");
  comment = oggz_comment_next_byname (oggz, serialno, comment);

  if (comment == NULL)
    FAIL ("Recently inserted ARTIST2 not retrieved");

  if (strcmp (comment->name, "ARTIST"))
    FAIL ("Incorrect ARTIST2 name found");

  if (strcmp (comment->value, ARTIST2))
    FAIL ("Incorrect ARTIST2 value found");

  INFO ("+ Removing LICENSE byname");
  err = oggz_comment_remove_byname (oggz, serialno, "LICENSE");
  if (err != 1) FAIL ("Operation failed");

  INFO ("+ Attempting to retrieve LICENSE");
  comment = oggz_comment_first_byname (oggz, serialno, "LICENSE");

  if (comment != NULL)
    FAIL ("Removed comment incorrectly retrieved");

  INFO ("+ Removing COPYRIGHT from local storage");
  mycomment.name = "COPYRIGHT";
  mycomment.value = COPYRIGHT;
  err = oggz_comment_remove (oggz, serialno, &mycomment);
  if (err != 1) FAIL ("Operation failed");

  INFO ("+ Attempting to retrieve COPYRIGHT");
  comment = oggz_comment_first_byname (oggz, serialno, "COPYRIGHT");

  if (comment != NULL)
    FAIL ("Removed comment incorrectly retrieved");

  INFO ("Closing OGGZ (writer)");
  oggz_close (oggz);
#endif /* OGGZ_CONFIG_WRITE */

#if OGGZ_CONFIG_READ
  INFO ("Initializing OGGZ for comments (reader)");
  oggz = oggz_new (OGGZ_READ);

  INFO ("+ Adding ARTIST1 byname (invalid for reader)");
  err = oggz_comment_add_byname (oggz, serialno, "ARTIST", ARTIST1);

  if (err == 0)
    FAIL ("Operation disallowed");

  INFO ("+ Removing ARTIST byname (invalid for reader)");
  err = oggz_comment_remove_byname (oggz, serialno, "ARTIST");

  if (err == 0)
    FAIL ("Operation disallowed");

  INFO ("Closing OGGZ (reader)");
  oggz_close (oggz);
#endif /* OGGZ_CONFIG_READ */

  exit (0);
}
