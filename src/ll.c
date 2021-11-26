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

            if(receive_s_u_frame(fd, SENDER) == UA)
                break;
            else timeout_no++;
        }

        if(timeout_no == ll.numTransmissions)
            return -2;
    }

    else
    {
        while(receive_s_u_frame(fd, SENDER) != SET);

        send_s_u_frame(fd, SENDER, UA);
    }

    return fd;
}

int llwrite(int fd, char *buffer, int length)
{
    if(length <= 0)
        return -1;
    
    int response, timeout_no=0;

    while(response != RR(!ll.sequenceNumber) && timeout_no < ll.numTransmissions)
    {
        send_i_frame(fd, buffer, length, ll.sequenceNumber);
        response = receive_s_u_frame(fd, role);
        timeout_no++;
        if(response == REJ(ll.sequenceNumber))
            timeout_no = 0;
    }

    if(timeout_no == ll.numTransmissions) return -1;

    ll.sequenceNumber = ll.sequenceNumber ? 0 : 1;
    return length;
    
}

int llread(int fd, char *buffer)
{
    printf("seqnum: %d\n", ll.sequenceNumber);
    int result = receive_i_frame(fd, buffer, ll.sequenceNumber);
    ll.sequenceNumber = ll.sequenceNumber ? 0 : 1;

    send_s_u_frame(fd, SENDER, RR(ll.sequenceNumber));
    return result;
}

int llclose(int fd)
{
    if(role == SENDER)
    {
        send_s_u_frame(fd, SENDER, DISC);
        receive_s_u_frame(fd, SENDER); //DISC
        send_s_u_frame(fd, SENDER, UA);
    }

    else
    {
        receive_s_u_frame(fd, SENDER); //DISC
        send_s_u_frame(fd, SENDER, DISC);
        receive_s_u_frame(fd, SENDER); //UA
    }
    
    tcsetattr(fd, TCSANOW, &oldtio);
    close(fd);

    return 1;
}