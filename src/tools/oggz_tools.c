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

#include <stdlib.h>
#include <string.h>
#include <oggz/oggz.h>

typedef struct {
  const char *bos_str;
  int bos_str_len;
  const char *content_type;
} OTCodecIdent;

static const OTCodecIdent codec_ident[] = {
  {"\200theora", 7, "theora"},
  {"\001vorbis", 7, "vorbis"},
  {"Speex", 5, "speex"},
  {"CMML\0\0\0\0", 8, "cmml"},
  {"Annodex", 8, "annodex"},
  {"fishead", 8, "skeleton"},
  {NULL}
};

const char *
ot_page_identify (const ogg_page * og)
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
      break;
    }
  }

  return ret;
}
