#include "ll.h"

#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <fcntl.h>

#define MAX_NPORT_SIZE 5

struct termios oldtio;
Source role;
linkLayer ll;

int llopen(int port, Source newRole)
{
    llconfig_reset(&ll);

    char nport_str[MAX_NPORT_SIZE];
    snprintf(nport_str, MAX_NPORT_SIZE, "%d", port); //port to str
    
    strcat(ll.port, nport_str);
    
    int fd = open(ll.port, O_RDWR | O_NOCTTY);
    if (fd < 0)
    {
        perror(ll.port);
        return -1;
    }

    llconfig(fd, &ll, &oldtio);

    role = newRole;

    int timeout_no = 0;

    if(newRole == SENDER)
    {
        while(timeout_no < ll.numTransmissions)
        {
            send_s_u_frame(fd, SENDER, SET);

            if(receive_s_u_frame(fd, SENDER, UA) == 0)
                break;
            else timeout_no++;
        }

        if(timeout_no == ll.numTransmissions)
            return -2;
    }

    else
    {
        while(receive_s_u_frame(fd, SENDER, SET) != 0);

        send_s_u_frame(fd, SENDER, UA);
    }

    return fd;
}

int llwrite(int fd, char *buffer, int length)
{
    if(length <= 0)
        return -1;
    
    send_i_frame(fd, buffer, length, ll.sequenceNumber);
    //receive_i_ack(fd);
}

int llread(int fd, char *buffer)
{
    
}

int llclose(int fd)
{
    if(role == SENDER)
    {
        send_s_u_frame(fd, SENDER, DISC);
        receive_s_u_frame(fd, SENDER, DISC);
        send_s_u_frame(fd, SENDER, UA);
    }

    else
    {
        receive_s_u_frame(fd, SENDER, DISC);
        send_s_u_frame(fd, SENDER, DISC);
        receive_s_u_frame(fd, SENDER, UA);
    }
    
    tcsetattr(fd, TCSANOW, &oldtio);
    close(fd);

    return 1;
}