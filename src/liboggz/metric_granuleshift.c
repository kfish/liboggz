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

#include "oggz_private.h"

typedef struct {
  ogg_int64_t gr_n;
  ogg_int64_t gr_d;
  int granuleshift;
} oggz_metric_granuleshift_t;

static ogg_int64_t
oggz_metric_default_granuleshift (OGGZ * oggz, long serialno,
				  ogg_int64_t granulepos, void * user_data)
{
  oggz_metric_granuleshift_t * gdata = (oggz_metric_granuleshift_t *)user_data;
  ogg_int64_t iframe, pframe;
  ogg_int64_t units;

  iframe = granulepos >> gdata->granuleshift;
  pframe = granulepos - (iframe << gdata->granuleshift);
  granulepos = (iframe + pframe);

  units = granulepos * gdata->gr_d / gdata->gr_n;

#ifdef DEBUG
  printf ("oggz_..._granuleshift: serialno %010ld Got frame %lld (%lld + %lld): %lld units\n",
	  serialno, granulepos, iframe, pframe, units);
#endif

  return units;
}

int
oggz_set_metric_granuleshift (OGGZ * oggz, long serialno,
			      ogg_int64_t granule_rate_numerator,
			      ogg_int64_t granule_rate_denominator,
			      int granuleshift)
{
  oggz_metric_granuleshift_t * granuleshift_data;

  /* we divide by the granulerate, ie. mult by gr_d/gr_n, so ensure
   * numerator is non-zero */
  if (granule_rate_numerator == 0) {
    granule_rate_numerator = 1;
    granule_rate_denominator = 0;
  }

  granuleshift_data = oggz_malloc (sizeof (oggz_metric_granuleshift_t));
  granuleshift_data->gr_n = granule_rate_numerator;
  granuleshift_data->gr_d = granule_rate_denominator;
  granuleshift_data->granuleshift = granuleshift;

  return oggz_set_metric_internal (oggz, serialno,
				   oggz_metric_default_granuleshift,
				   granuleshift_data, 1);
}
