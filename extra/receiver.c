/** 
 * Non-Canonical Input Processing
 * From https://tldp.org/HOWTO/Serial-Programming-HOWTO/x115.html by Gary Frerking and Peter Baumann
**/

#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <stdio.h>

#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "comms.h"
#include "app.h"
#include "utils.h"

#define _POSIX_SOURCE 1 /* POSIX compliant source */

int main(int argc, char **argv)
{
  if ((argc < 2) ||
      ((strcmp("/dev/ttyS0", argv[1]) != 0) &&
       (strcmp("/dev/ttyS1", argv[1]) != 0) &&
       (strcmp("/dev/ttyS10", argv[1]) != 0) &&
       (strcmp("/dev/ttyS11", argv[1]) != 0)))
  {
    printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1 [msg]\n");
    exit(1);
  }

  int port = atoi(argv[1] + 9);

  printf("Starting...\n");

  app_start(port, RECEIVER);

  printf("Waiting to receive start packet.\n");

  int result = receive_file();

  if (result != 0) {
    printf("Error receiving data.");
    return -1;
  }

  printf("Closing...\n");
  
  app_end();

  return 0;
}