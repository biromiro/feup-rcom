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

#include "app.h"
#include "comms.h"

#define _POSIX_SOURCE 1 /* POSIX compliant source */

int main(int argc, char **argv)
{
  if ((argc < 3) ||
      ((strcmp("/dev/ttyS0", argv[1]) != 0) &&
       (strcmp("/dev/ttyS1", argv[1]) != 0) &&
       (strcmp("/dev/ttyS10", argv[1]) != 0) &&
       (strcmp("/dev/ttyS11", argv[1]) != 0)))
  {
    printf("Usage:\tapp SerialPort Filepath\n\tex: app /dev/ttyS1 ./pinguim.gif\n");
    exit(1);
  }

  int port = atoi(argv[1] + 9);

  printf("Starting...\n");

  app_start(port, SENDER);

  printf("Assembling and sending...\n");

  char *filepath = argv[2];

  int result = send_file(filepath);

  if (result != 0) {
    printf("Error sending data.");
    return -1;
  }

  printf("Closing...\n");

  app_end();

  return 0;
}