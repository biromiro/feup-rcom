#include "ll.h"
#include "comms.h"
#include "utils.h"

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
    
    unsigned char response, timeout_no = 0;

    do
    {
        send_i_frame(fd, buffer, length, ll.sequenceNumber);
        
        response = receive_s_u_frame(fd, role);
        
        if (response == 0 && timeout_no < ll.numTransmissions)
        {
            timeout_no++;
            printf("Timed out waiting on RR/REJ: %d\n", timeout_no); 
        }

        if(response == REJ(ll.sequenceNumber))
            timeout_no = 0;

    } while (response != RR(invSN(ll.sequenceNumber)) && timeout_no < ll.numTransmissions);

    if(timeout_no == ll.numTransmissions)
    {
        printf("Timed out llwrite.\n");
        return -1;
    }

    ll.sequenceNumber = invSN(ll.sequenceNumber);

    return length;
}

int llread(int fd, char *buffer)
{
    int result;
    
    do
    {
        result = receive_i_frame(fd, buffer, ll.sequenceNumber);

        if (result == -1) 
            send_s_u_frame(fd, SENDER, REJ(ll.sequenceNumber));

        if (result == 0)
            send_s_u_frame(fd, SENDER, RR(ll.sequenceNumber));

    } while (result <= 0);

    ll.sequenceNumber = invSN(ll.sequenceNumber);

    send_s_u_frame(fd, SENDER, RR(ll.sequenceNumber));

    return result;
}

int llclose(int fd)
{
    int timeout_no = 0;

    if(role == SENDER)
    {
        while (timeout_no < ll.numTransmissions)
            if (send_s_u_frame(fd, SENDER, DISC) != 0 || receive_s_u_frame(fd, SENDER) != DISC || send_s_u_frame(fd, SENDER, UA) != 0)
                timeout_no++;
            else break;
    }

    else
    {
        while (timeout_no < ll.numTransmissions)
            if (receive_s_u_frame(fd, SENDER) != DISC || send_s_u_frame(fd, SENDER, DISC) != 0 || receive_s_u_frame(fd, SENDER) != UA)
                timeout_no++;
            else break;
    }

    if(timeout_no == ll.numTransmissions)
        printf("Timed out trying to llclose gracefully.\n");
    
    tcsetattr(fd, TCSANOW, &oldtio);
    close(fd);

    return 1;
}
