#include "ll.h"

struct termios oldtio;
int numTransmissions;
Source role;

int llconfig(linkLayer ll)
{
    struct termios newtio;

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

    numTransmissions = ll.numTransmissions;

    tcflush(fd, TCIOFLUSH);

    if (tcsetattr(fd, TCSANOW, &newtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    printf("New termios structure set\n");
}

int llopen(int port, Source newRole)
{
    char[20] serial_port;

    if(port == 0 || port == 1 || port == 10 || port == 11)
        snprintf(serial_port, "/dev/ttyS%d", port);
    
    int fd = open(serial_port, O_RDWR | O_NOCTTY);
    if (fd < 0)
    {
        perror(serial_port);
        return -1;
    }

    role = newRole;

    if(newRole == SENDER)
    {
        while(timeout_no < ll.numTransmissions)
        {
            send_s_u_frame(fd, src, SET);

            if(receive_s_u_frame(src, UA))
                break;
            else timeout_no++;
        }

        if(timeout_no == ll.numTransmissions)
            return -2;
    }

    else
    {
        while(!receive_s_u_frame(SENDER, SET));

        send_s_u_frame(fd, SENDER, UA);
    }

    return fd;
}

int llclose(int fd)
{

}