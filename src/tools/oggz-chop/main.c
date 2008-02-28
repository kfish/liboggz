#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "oggz-chop.h"
/*#include "cgi.h"*/
#include "cmd.h"
 
int
main (int argc, char * argv[])
{
  int err = 0; 

#if 0
  if (cgi_test ()) {
    err = cgi_main (&params);
  } else {
    err = cmd_main (&params, argc, argv);
  }
#else
  err = cmd_main (argc, argv);
#endif
  
  if (err) return 1;
  else return 0;
}
