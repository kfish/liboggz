#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <getopt.h>

#include "oggz-chop.h"

static char * progname;

static void
usage (char * progname)
{
  printf ("Usage: %s [options] filename\n", progname);
  printf ("Chop an Ogg file.\n");
  printf ("\nOutput options\n");
  printf ("  -o filename, --output filename\n");
  printf ("                         Specify output filename\n");
  printf ("  -s start_time, --start start_time\n");
  printf ("                         Specify start time\n");
  printf ("  -e end_time, --end end_time\n");
  printf ("                         Specify end time\n");
  printf ("\nMiscellaneous options\n");
  printf ("  -h, --help             Display this help and exit\n");
  printf ("  -v, --version          Output version information and exit\n");
  printf ("\n");
  printf ("Please report bugs to <ogg-dev@xiph.org>\n");
}

int
cmd_main (OCState * state, int argc, char * argv[])
{
  int show_version = 0;
  int show_help = 0;
  int i;

  progname = argv[0];

  if (argc < 2) {
    usage (progname);
    return (1);
  }

  state->start = 0.0;
  state->end = -1.0;
  state->infilename = NULL;
  state->outfilename = NULL;

  while (1) {
    char * optstring = "s:e:o:hv";

#ifdef HAVE_GETOPT_LONG
    static struct option long_options[] = {
      {"start",   required_argument, 0, 's'},
      {"end",   required_argument, 0, 'e'},
      {"output",   required_argument, 0, 'o'},
      {"help",     no_argument, 0, 'h'},
      {"version",  no_argument, 0, 'v'},
      {0,0,0,0}
    };

    i = getopt_long(argc, argv, optstring, long_options, NULL);
#else
    i = getopt (argc, argv, optstring);
#endif
    if (i == -1) break;
    if (i == ':') {
      usage (progname);
      goto exit_err;
    }

    switch (i) {
    case 's': /* start */
      state->start = atof (optarg);
      break;
    case 'e': /* end */
      state->end = atof (optarg);
      break;
    case 'h': /* help */
      show_help = 1;
      break;
    case 'v': /* version */
      show_version = 1;
      break;
    case 'o': /* output */
      state->outfilename = optarg;
      break;
    default:
      break;
    }
  }

  if (show_version) {
    printf ("%s version " VERSION "\n", progname);
  }

  if (show_help) {
    usage (progname);
  }

  if (show_version || show_help) {
    goto exit_ok;
  }

  if (optind >= argc) {
    usage (progname);
    goto exit_err;
  }

  state->infilename = argv[optind++];

  return chop (state);

exit_ok:
  return 0;

exit_err:
  return 1;
}