#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <libgen.h> /* basename() */

#include "oggz-chop.h"
#include "header.h"
#include "httpdate.h"
#include "timespec.h"

/* Customization: for servers that do not set PATH_TRANSLATED, specify the
 * DocumentRoot here and it will be prepended to PATH_INFO */
//#define DOCUMENT_ROOT "/var/www"
#define DOCUMENT_ROOT NULL

static void
set_param (OCState * state, char * key, char * val)
{
  char * sep;

  if (!strncmp ("s", key, 2)) state->start = parse_timespec (val);
  if (!strncmp ("start", key, 6)) state->start = parse_timespec (val);

  if (!strncmp ("e", key, 2)) state->end = parse_timespec (val);
  if (!strncmp ("end", key, 6)) state->end = parse_timespec (val);

  if (!strncmp ("t", key, 2)) {
    if (val && (sep = strchr (val, '/')) != NULL) {
      *sep++ = '\0';
      state->end = parse_timespec (sep);
    } else {
      state->end = -1.0;
    }
    state->start = parse_timespec (val);
  }

  /* Append &download to set "Content-Disposition: attachment" */
  if (!strncmp ("download", key, 8)) state->is_attachment = 1;
}

/**
 * Parse the name=value pairs in the query string and set parameters
 * @param start,end The range parameters to set
 * @param query The query string
 */
static void
parse_query (OCState * state, char * query)
{
  char * key, * val, * end;

  if (!query) return;

  key = query;

  do {
    val = strchr (key, '=');
    end = strchr (key, '&');

    if (end) {
      if (val) {
        if (val < end) {
          *val++ = '\0';
        } else {
          val = NULL;
        }
      }
      *end++ = '\0';
    } else {
      if (val) *val++ = '\0';
    }

    /* fprintf (stderr, "%s = %s\n", key, val);*/
    set_param (state, key, val);

    key = end;

  } while (end != NULL);

  return;
}

int
parse_range (OCState * state, char * range, oggz_off_t size)
{
  char * range_unit, * start, * end, * next;
  oggz_off_t start_offset=0, end_offset=-1;

  if (!range) return 0;

  range_unit = range;

  start = strchr (range, '=');
  end = strchr (range, '-'); 
  next = strchr (range, ',');

  if (!start) return -1;

  *start++ = '\0';

  /* Only handle byte ranges */
  if (strcmp (range_unit, "bytes")) {
    fprintf (stderr, "oggz-chop: parse_range: not bytes\n");
    return -1;
  }

  if (end == NULL) {
    fprintf (stderr, "oggz-chop: range has no '-'\n");
    return -1;
  }

  *end++ = '\0';

  if (*start == '\0') {
    start_offset = size - strtol (end, NULL, 0);
    end_offset = size-1;
  } else {
    start_offset = strtol (start, NULL, 0);
    if (*end == '\0') {
      end_offset = size-1;
    } else {
      end_offset = strtol (end, NULL, 0);
    }
  }

  state->byte_range_start = start_offset;
  state->byte_range_end = end_offset;

  return 0;
}

int
cgi_test (void)
{
  char * gateway_interface;

  gateway_interface = getenv ("GATEWAY_INTERFACE");
  if (gateway_interface == NULL) {
    return 0;
  }

  return 1;
}

static char *
prepend_document_root (char * path_info)
{
  char * dr = DOCUMENT_ROOT;
  char * path_translated;
  int dr_len, pt_len;

  if (path_info == NULL) return NULL;

  if (dr == NULL || *dr == '\0') {
    if ((path_translated = strdup (path_info)) == NULL)
      goto prepend_oom;
  } else {
    dr_len = strlen (dr);

    pt_len = dr_len + strlen(path_info) + 1;
    if ((path_translated = malloc (pt_len)) == NULL)
      goto prepend_oom;
    snprintf (path_translated, pt_len , "%s%s", dr, path_info);
  }

  return path_translated;

prepend_oom:
  fprintf (stderr, "oggz-chop: Out of memory");
  return NULL;
}

static int
path_undefined (char * vars)
{
  fprintf (stderr, "oggz-chop: Cannot determine real filename due to CGI configuration error: %s undefined\n", vars);
  return -1;
}

static int
sprint_time (char * s, double seconds)
{
  int hrs, min;
  double sec;
  char * sign;

  sign = (seconds < 0.0) ? "-" : "";

  if (seconds < 0.0) seconds = -seconds;

  hrs = (int) (seconds/3600.0);
  min = (int) ((seconds - ((double)hrs * 3600.0)) / 60.0);
  sec = seconds - ((double)hrs * 3600.0)- ((double)min * 60.0);

  return sprintf (s, "%s%02d:%02d:%06.3f", sign, hrs, min, sec);
}

void
set_disposition_attachment (OCState * state, char * path_translated)
{
  int n=0;
  char *p, *b, *ext;
  char buf[512];

  p = strdup (path_translated);
  b = basename (p);
  ext = strrchr (b, '.');
  if (ext != NULL) {
    *ext++ = '\0';
  }

  if (state->end == -1.0) {
    if (state->start == 0.0) {
      strcpy (buf, b);
    } else {
      n = sprintf (buf, "%s_", b);
      n += sprint_time (&buf[n], state->start);
    }
  } else {
    if (state->start == 0.0) {
      n = sprintf (buf, "%s_0-", b);
      n += sprint_time (&buf[n], state->end);
    } else {
      n = sprintf (buf, "%s_", b);
      n += sprint_time (&buf[n], state->start);
      n += sprintf (&buf[n], "-");
      n += sprint_time (&buf[n], state->end);
    }
  }

  /* Append file extension if earlier removed */
  if (ext) {
      sprintf (&buf[n], ".%s", ext);
  }

  header_content_disposition_attachment (buf);
  free (p);
}

int
cgi_main (OCState * state)
{
  int err = 0;
  char * path_info;
  char * path_translated;
  char * query_string;
  char * if_modified_since;
  char * range;
  time_t since_time, last_time;
  struct stat statbuf;
  int built_path_translated=0;
  double duration;
  oggz_off_t size;

  httpdate_init ();

  path_info = getenv ("PATH_INFO");
  path_translated = getenv ("PATH_TRANSLATED");
  query_string = getenv ("QUERY_STRING");
  if_modified_since = getenv ("HTTP_IF_MODIFIED_SINCE");
  range = getenv ("HTTP_RANGE");

  /* Default values */
  memset (state, 0, sizeof(*state));
  state->end = -1.0;
  state->byte_range_end = -1;
  state->do_skeleton = 1;

  if (path_translated == NULL) {
    if (path_info == NULL)
      return path_undefined ("PATH_TRANSLATED and PATH_INFO");

    path_translated = prepend_document_root (path_info);
    if (path_translated == NULL)
      return path_undefined ("PATH_TRANSLATED");

    built_path_translated = 1;
  }

  state->infilename = path_translated;

  /* Get Last-Modified time */
  if (stat (path_translated, &statbuf) == -1) {
    switch (errno) {
    case ENOENT:
      return 0;
    default:
      fprintf (stderr, "oggz-chop: %s: %s\n", path_translated, strerror(errno));
      return -1;
    }
  }

  last_time = statbuf.st_mtime;

  if (if_modified_since != NULL) {
    int len;

    fprintf (stderr, "If-Modified-Since: %s\n", if_modified_since);

    len = strlen (if_modified_since) + 1;
    since_time = httpdate_parse (if_modified_since, len);

    if (last_time <= since_time) {
      header_not_modified();
      header_end();
      return 1;
    }
  }

  header_content_type_ogg ();

  header_last_modified (last_time);

  header_accept_ranges ();
  header_accept_timeuri_ogg ();

  parse_query (state, query_string);

  /* Init */

  err = 0;
  err = chop_init (state);

  if (state->is_attachment) {
    set_disposition_attachment (state, path_translated);
  }

  if (state->end == -1.0) {
    duration = ((double)oggz_get_duration (state->oggz)/1000.0) - state->start;
  } else {
    duration = state->end - state->start;
  }
  header_content_duration (duration);

  size = oggz_get_length (state->oggz);

  if (range != NULL) {
    parse_range (state, range, size);

    if (state->byte_range_start > state->byte_range_end ||
        state->byte_range_end >= size) {
      header_status_416();
      header_content_range_star (size);
      header_end();
      return 1;
    } else {
      header_status_206();
      if (state->start > 0.0 || state->end != -1.0) {
        header_content_range_bytes (state->byte_range_start, state->byte_range_end, -1);
      } else {
        header_content_range_bytes (state->byte_range_start, state->byte_range_end, size);
      }
      header_content_length (state->byte_range_end - state->byte_range_start + 1);

      /* Now that the headers are done, increment byte_range_end so that it
       * can be used as a counter of remaining bytes for fwrite */
      state->byte_range_end++;
    }
  } else if (state->start == 0.0 && state->end == -1.0) {
    header_content_length (size);
  }

  header_end();

  chop_run (state);

  chop_close (state);

  if (built_path_translated && path_translated != NULL)
    free (path_translated);
  
  return err;
}
