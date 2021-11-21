#include "comms.h"

int stuff(vector *v)
{
    char cur_byte;

    for(int i = 0; i < v->size; i++)
    {
        cur_byte = vector_get(v, i);

        if(cur_byte == FLAG || cur_byte == ESC)
        {
            vector_set(v, i, ESC);
            vector_push_at(v, i+1, ESCAPED(cur_byte));
        }
    }

    return 0;
}

int send_s_u_frame(int fd, Source src, int ctrl)
{
    printf("Sending A=%d, C=%d.\n", src == SENDER ? A_SND : A_RCV, ctrl);

    char buf[5];

    buf[0] = FLAG;
    buf[1] = src == SENDER ? A_SND : A_RCV;
    buf[2] = ctrl;
    buf[3] = BCC(buf[1], buf[2]);
    buf[4] = FLAG;

    return write(fd, buf, 5);
}

int receive_s_u_frame(int fd, Source src, int ctrl)
{
    printf("Expecting A=%d, C=%d.\n", src == SENDER ? A_SND : A_RCV, ctrl);

    char buf[5];
    int res;

    State cur_state = START;
    char address = src == SENDER ? A_SND : A_RCV;

    while (cur_state != STOP) { 
        res = read(fd, buf, 1);
        if (res == -1)
        {
            printf("An error occurred while reading S/U frame.\n;");
            continue;
        }
        if(res == 0)
        {
            printf("Timed out.\n");
            return 1;
        }
        printf("got: %x\n", buf[0]);
        switch (cur_state) {
            case START:
                if (buf[0] == FLAG) cur_state = FLAG_RCV;
                else printf("Unknown message byte\n");
                break;
            
            case FLAG_RCV:
                printf("rcv\n");
                if (buf[0] == address)
                    cur_state = A_REC;
                else if (buf[0] != FLAG) cur_state = START;
                break;
            
            case A_REC:
                printf("a\n");
                if (buf[0] == ctrl)
                    cur_state = C_REC;
                else if (buf[0] == FLAG) cur_state = FLAG_RCV;
                else cur_state = START;
                break;
            
            case C_REC:
                printf("c\n");
                if (BCC(address,ctrl) == buf[0])
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

    return 0;
}

int send_i_frame(int fd, char *buffer, int length, bool seqNum)
{
    vector *v = malloc(sizeof (vector));
    vector_from_arr(v, buffer, length);
    char address = A_SND;
    char ctrl = seqNum << 7;
    vector_push_front(v, BCC(address, ctrl));
    vector_push_front(v, ctrl);
    vector_push_front(v, address);
    
    char data_bcc;

    data_bcc = buffer[0];

    for(int i = 1; i < length; i++)
        data_bcc = BCC(data_bcc, buffer[i]);
    
    vector_push_back(v, data_bcc);
    
    stuff(v);

    vector_push_front(v, FLAG);
    vector_push_back(v, FLAG);

    int res = write(fd, v->items, v->size);

    vector_free(v);
    free(v);

    return res;
}

int receive_i_frame(int fd, char *buffer, bool seqNum)
{
    char buf[5];
    int res;

    char prev_byte = 0;
    char cur_bcc = 1;
    bool first_data = true;

    const char address = A_SND;
    char ctrl = seqNum << 7;

    State cur_state = START;

    while (cur_state != STOP) { 
        res = read(fd, buf, 1);
        if (res == -1)
        {
            printf("An error occurred while reading I frame.\n;");
            continue;
        }
        if(res == 0)
        {
            printf("Timed out.\n");
            return 1;
        }
        printf("got: %x\n", buf[0]);
        switch (cur_state) {
            case START:
                if (buf[0] == FLAG) cur_state = FLAG_RCV;
                else printf("Unknown message byte\n");
                break;
            
            case FLAG_RCV:
                printf("rcv\n");
                if (buf[0] == address)
                    cur_state = A_REC;
                else if (buf[0] != FLAG) cur_state = START;
                break;
            
            case A_REC:
                printf("a\n");
                if (buf[0] == ctrl)
                    cur_state = C_REC;
                else if (buf[0] == FLAG) cur_state = FLAG_RCV;
                else cur_state = START;
                break;
            
            case C_REC:
                printf("c\n");
                if (BCC(address,ctrl) == buf[0])
                    cur_state = DATA;
                else if (buf[0] = FLAG)
                    cur_state = FLAG_RCV;
                else
                    cur_state = START;
                break;
            
            case DATA:
                printf("bcc1\n");
                
                if (buf[0] == FLAG)
                {
                    if(cur_bcc == prev_byte)
                        cur_state = STOP;
                    else
                        cur_state = START;
                }
                else
                {
                    if(first_data)
                        cur_bcc = buf[0];
                    if(buf[0] == ESC)
                        continue;
                }
                break;
        }
    }
}