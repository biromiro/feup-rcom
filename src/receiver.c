/** 
 * Non-Canonical Input Processing
 * From https://tldp.org/HOWTO/Serial-Programming-HOWTO/x115.html by Gary Frerking and Peter Baumann
**/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>

#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "utils.h"
#include "comms.h"

#define BAUDRATE B38400
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

typedef enum State {START, FLAG_RCV, A_REC, C_REC, BCC_OK, STOP};

int main(int argc, char **argv)
{
  int fd, res;
  struct termios oldtio, newtio;
  char buf[5], resp[5];

  struct linkLayer ll;

  int timeout_no = 0;
  

  if ((argc < 2) ||
      ((strcmp("/dev/ttyS0", argv[1]) != 0) &&
       (strcmp("/dev/ttyS1", argv[1]) != 0) &&
       (strcmp("/dev/ttyS10", argv[1]) != 0) &&
       (strcmp("/dev/ttyS11", argv[1]) != 0)))
  {
    printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
    exit(1);
  }

  strncpy(ll.port, argv[1], 20);
  ll.baudRate = BAUDRATE;
  
  ll.timeout = 30;
  ll.numTransmissions = 3;

  /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
  */

  fd = open(argv[1], O_RDWR | O_NOCTTY);
  if (fd < 0)
  {
    perror(argv[1]);
    exit(-1);
  }

  if (tcgetattr(fd, &oldtio) == -1)
  { /* save current port settings */
    perror("tcgetattr");
    exit(-1);
  }

  bzero(&newtio, sizeof(newtio));
  newtio.c_cflag = ll.baudRate | CS8 | CLOCAL | CREAD;
  newtio.c_iflag = IGNPAR;
  newtio.c_oflag = 0;

  /* set input mode (non-canonical, no echo,...) */
  newtio.c_lflag = 0;

  newtio.c_cc[VTIME] = ll.timeout;
  newtio.c_cc[VMIN] = 0;

  /* 
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a 
    leitura do(s) proximo(s) caracter(es)
  */

  tcflush(fd, TCIOFLUSH);

  if (tcsetattr(fd, TCSANOW, &newtio) == -1)
  {
    perror("tcsetattr");
    exit(-1);
  }

  printf("New termios structure set\n");
    
  enum State cur_state = START;
  char a_val, c_val;    

  while (cur_state != STOP && timeout_no < ll.numTransmissions) { 
    res = read(fd, buf, 1);

    if (!buf[0])
      timeout_no++;
    
    switch (cur_state) {
        case START:
            if (buf[0] == FLAG) cur_state = FLAG_RCV;
            else printf("Unknown message byte: %x\n", buf[0]);
            break;
        
        case FLAG_RCV:
            printf("rcv\n");
            if (buf[0] == A_SND){
                cur_state = A_REC;
                a_val = buf[0];
            }
            else if (buf[0] != FLAG) cur_state = START;
            break;
        
        case A_REC:
            printf("a\n");
            if (buf[0] == SET){
                cur_state = C_REC;
                c_val = buf[0];
            }
            else if (buf[0] == FLAG) cur_state = FLAG_RCV;
            else cur_state = START;
            break;
        
        case C_REC:
            printf("c\n");
            if (BCC(a_val,c_val) == buf[0])
                cur_state = BCC_OK;
            else if (buf[0] = FLAG)
                cur_state = FLAG_RCV;
            else
                cur_state = START;
            break;
        
        case BCC_OK:
            printf("bcc\n");
            if (buf[0] == FLAG)
                cur_state = STOP;
            else cur_state = START;
            break;
    }
  }

  if(timeout_no != ll.numTransmissions)
  {
    printf("a = %x, c = %x\n", a_val, c_val);

    printf("Got packet. Sending response...\n");

    resp[0] = FLAG;
    resp[1] = A_SND;
    resp[2] = UA;
    resp[3] = BCC(resp[1], resp[2]);
    resp[4] = FLAG;


    write(fd, resp, 5);

    printf("Response sent.\n");
  }
  else printf("Timed out.\n");

  tcsetattr(fd, TCSANOW, &oldtio);
  close(fd);
  return 0;
}
