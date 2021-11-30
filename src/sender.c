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

  app_start(port, SENDER);

  printf("Assembling and sending...\n");

  TLV filename;
  filename.T = 0;
  char strfilename[] = "pinguim.gif";
  filename.L = strlen(strfilename);
  filename.V = strfilename;

  TLV filesize;
  filesize.T = 1;
  char nfilesize = 55;
  filename.L = sizeof(size_t);
  filename.V = &nfilesize;

  printf("%s", filename.V);

  TLV tlvs[] = {filename, filesize};

  send_control_packet(2, tlvs, 2);

  printf("Closing...\n");
  
  app_end();

  return 0;
}