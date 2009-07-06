#include "config.h"

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h> /* For off_t not found in stdio.h */
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "httpdate.h"

#include "oggz/oggz_off_t.h"

#define CONTENT_TYPE_OGG "Content-Type: application/ogg\n"
#define ACCEPT_TIMEURI_OGG "X-Accept-TimeURI: application/ogg\n"
#define ACCEPT_RANGES "Accept-Ranges: bytes\n"

int
header_status_206 (void)
{
  return printf ("Status: 206 Partial Content\n");
}

int
header_status_416 (void)
{
  return printf ("Status: 416 Requested range not satisfiable\n");
}

int
header_last_modified (time_t mtime)
{
  char buf[30];

  httpdate_snprint (buf, 30, mtime);
  return printf ("Last-Modified: %s\n", buf);
}

int
header_not_modified (void)
{
  fprintf (stderr, "304 Not Modified\n");
  return printf ("Status: 304 Not Modified\n");
}

int
header_content_type_ogg (void)
{
  return printf (CONTENT_TYPE_OGG);
}

int
header_accept_timeuri_ogg (void)
{
  return printf (ACCEPT_TIMEURI_OGG);
}

int
header_content_length (oggz_off_t len)
{
  return printf ("Content-Length: %lld\n", len);
}

int
header_content_duration (double duration)
{
  return printf ("Content-Duration: %06.3f\n", duration);
}

int
header_content_range_bytes (oggz_off_t range_start, oggz_off_t range_end,
                            oggz_off_t size)
{
  return printf ("Content-Range: bytes %lld-%lld/%lld\n", range_start, range_end, size);
}

int
header_content_range_star (oggz_off_t size)
{
  return printf ("Content-Range: bytes */%lld\n", size);
}

int
header_accept_ranges (void)
{
  return printf (ACCEPT_RANGES);
}

int
header_end (void)
{
  putchar('\n');
  fflush (stdout);
  return 0;
}
