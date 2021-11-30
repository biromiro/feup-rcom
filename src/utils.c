#include "utils.h"

int llconfig_reset(linkLayer* ll)
{
    strcpy(ll->port, "/dev/ttyS");
    ll->baudRate = BAUDRATE;
    ll->sequenceNumber = 0;
    ll->timeout = TIMEOUT;
    ll->numTransmissions = NUM_TRANSM;

    return 0;
}

int llconfig(int fd, linkLayer* ll, struct termios *oldtio)
{
    struct termios newtio;

    if (tcgetattr(fd, oldtio) == -1)
    { /* save current port settings */
        perror("tcgetattr");
        exit(-1);
    }

    memset(&newtio, 0, sizeof(newtio));
    newtio.c_cflag = ll->baudRate | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME] = ll->timeout;
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

    return 0;
}