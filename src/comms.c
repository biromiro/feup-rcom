#include "comms.h"
#include "utils.h"
#include "vector.h"
#include <stdio.h>

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

int destuff(vector *v) 
{
    char cur_byte, escaped_byte;

    for(int i = 0; i < v->size; i++)
    {
        cur_byte = vector_get(v, i);

        if(cur_byte == ESC)
        {
            escaped_byte = vector_get(v, i+1);
            vector_delete(v, i);
            vector_set(v, i, ESCAPED(escaped_byte));
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

char receive_s_u_frame(int fd, Source src)
{
    printf("Expecting S/U frame.\n");

    char buf[5];
    char ctrl = 0;
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
            return -1;
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
                if (buf[0] == FLAG) cur_state = FLAG_RCV;
                else
                {
                    ctrl = buf[0];
                    cur_state = C_REC;
                }
                break;
            
            case C_REC:
                printf("c\n");
                if (BCC(address,ctrl) == buf[0])
                    cur_state = BCC_OK;
                else if (buf[0] == FLAG)
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
            
            default:
                break;
        }
    }

    return ctrl;
}

int send_i_frame(int fd, char *buffer, int length, bool seqNum)
{
    vector v;
    vector_from_arr(&v, buffer, length);

    char address = A_SND;
    char ctrl = seqNum << 6;
    vector_push_front(&v, BCC(address, ctrl));
    vector_push_front(&v, ctrl);
    vector_push_front(&v, address);
    
    char data_bcc;

    data_bcc = buffer[0];

    for(int i = 1; i < length; i++)
        data_bcc = BCC(data_bcc, buffer[i]);
    
    vector_push_back(&v, data_bcc);
    
    stuff(&v);

    vector_push_front(&v, FLAG);
    vector_push_back(&v, FLAG);

    int res = write(fd, v.items, v.size);

    vector_free(&v);

    return res;
}

int receive_i_frame(int fd, char *buffer, bool seqNum)
{
    char buf[5];
    int res, index = 0;

    vector v;
    vector_init(&v);

    char prev_byte = 0;
    char cur_bcc = 1;
    bool first_data = true;

    const char address = A_SND;
    char ctrl = seqNum << 6;
    char inv_ctrl = invSN(seqNum) << 6;

    State cur_state = START;

    int i = 0;
    
    while(cur_state != STOP) {
        char info;
        res = read(fd, buf, 1);

        if (res == -1)
        {
            printf("An error occurred while reading I frame.\n;");
            continue;
        }
        if(res == 0)
        {
            printf("Timed out.\n");
            vector_free(&v);
            return 1;
        }

        vector_push_back(&v, buf[0]);

        switch(cur_state) {
            case START:
                if(buf[0] == FLAG) cur_state = DATA;
                break;

            case DATA:
                if(buf[0] == FLAG) cur_state = STOP;
                break;

            default:
                break;
        }
    }

    destuff(&v);

    cur_state = START;

    while (cur_state < STOP && index < v.size) {
        buf[0] = vector_get(&v, index);
        printf("got: %x\n", buf[0]);
        switch (cur_state) {
            case START:
                if (buf[0] == FLAG) cur_state = FLAG_RCV;
                else cur_state = HEADER_ERR;
                break;
            
            case FLAG_RCV:
                printf("rcv\n");
                if (buf[0] == address)
                    cur_state = A_REC;
                else cur_state = HEADER_ERR;
                break;
            
            case A_REC:
                printf("a\n");
                if (buf[0] == ctrl || buf[0] == inv_ctrl)
                    cur_state = C_REC;
                else cur_state = HEADER_ERR;
                break;
            
            case C_REC:
                printf("c\n");
                if (BCC(address,ctrl) == buf[0])
                    cur_state = DATA;
                else if (BCC(address, inv_ctrl) == buf[0])
                    cur_state = SEQNUM_ERR;
                else
                    cur_state = HEADER_ERR;
                break;
            
            case DATA:
                printf("data\n");
                if (buf[0] == FLAG)
                {
                    if(cur_bcc == prev_byte)
                        cur_state = STOP;
                    else
                        cur_state = DATA_ERR;
                }
                else
                {
                    if(first_data)
                    {
                        cur_bcc = buf[0];
                        first_data = false;
                    }
                    else cur_bcc = BCC(cur_bcc, buf[0]);
                }
                break;

            default:
                break;
        }
        index++;
    }

    int size;

    if(cur_state == STOP)
    {
        //remove header
        for (int i = 0; i < 4; i++)
            vector_delete(&v, 0);
        
        //remove tail
        vector_delete(&v, v.size-1);
        vector_delete(&v, v.size-1);

        size = v.size;
        memcpy(buffer, v.items, size);

        vector_free(&v);

        return size;
    }

    vector_free(&v);
    
    if(cur_state == HEADER_ERR)
        return -2;
    if (cur_state == DATA_ERR)
        return -1;
    if (cur_state == SEQNUM_ERR)
        return 0;
    
    return -3;
}
