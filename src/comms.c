#include "comms.h"
#include "utils.h"
#include "vector.h"
#include <stdio.h>
#include <stdlib.h>

int stuff(vector *v)
{
    u_int8_t cur_byte;

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
    u_int8_t cur_byte, escaped_byte;

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
    u_int8_t buf[5];

    buf[0] = FLAG;
    buf[1] = src == SENDER ? A_SND : A_RCV;
    buf[2] = ctrl;
    buf[3] = BCC(buf[1], buf[2]);
    buf[4] = FLAG;

    int res = write(fd, buf, 5);

    return res != 5;
}

u_int8_t receive_s_u_frame(int fd, Source src)
{
    u_int8_t byte;
    u_int8_t ctrl = 0;
    int res;

    State cur_state = START;
    u_int8_t address = src == SENDER ? A_SND : A_RCV;

    while (cur_state != STOP) { 
        res = read(fd, &byte, 1);
        if (res == -1)
        {
            printf("An error occurred while reading S/U frame.\n;");
            continue;
        }
        if(res == 0)
            return 0;

        switch (cur_state) {
            case START:
                if (byte == FLAG)
                    cur_state = FLAG_RCV;
                break;
            
            case FLAG_RCV:
                if (byte == address)
                    cur_state = A_REC;
                else if (byte != FLAG)
                    cur_state = START;
                break;
            
            case A_REC:
                if (byte == FLAG) cur_state = FLAG_RCV;
                else
                {
                    ctrl = byte;
                    cur_state = C_REC;
                }
                break;
            
            case C_REC:
                if (BCC(address,ctrl) == byte)
                    cur_state = BCC_OK;
                else if (byte == FLAG)
                    cur_state = FLAG_RCV;
                else
                    cur_state = START;
                break;
            
            case BCC_OK:
                if (byte == FLAG)
                    cur_state = STOP;
                else
                    cur_state = START;
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

    u_int8_t address = A_SND;
    u_int8_t ctrl = seqNum << 6;
    vector_push_front(&v, BCC(address, ctrl));
    vector_push_front(&v, ctrl);
    vector_push_front(&v, address);
    
    u_int8_t data_bcc;

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
    u_int8_t byte;
    int res, index = 0;

    vector v;
    vector_init(&v);

    u_int8_t prev_byte = 0;
    u_int8_t cur_bcc = 1;
    bool first_data = true, second_data = false;

    const u_int8_t address = A_SND;
    u_int8_t ctrl = seqNum << 6;
    u_int8_t inv_ctrl = invSN(seqNum) << 6;

    State cur_state = START;
    
    while(cur_state != STOP) {

        res = read(fd, &byte, 1);    

        if (res == -1)
        {
            printf("An error occurred while reading I frame.\n;");
            continue;
        }

        if(res == 0)
        {
            printf("Got nothing on I frame.\n");
            vector_free(&v);
            return -3;
        }

        vector_push_back(&v, byte);

        switch(cur_state) {
            case START:
                if(byte == FLAG) cur_state = DATA;
                break;

            case DATA:
                if(byte == FLAG) cur_state = STOP;
                break;

            default:
                break;
        }
    }

    destuff(&v);

    cur_state = START;

    while (cur_state < STOP && index < v.size) {
        byte = vector_get(&v, index);
        switch (cur_state) {
            case START:
                if (byte == FLAG) cur_state = FLAG_RCV;
                else cur_state = HEADER_ERR;
                break;
            
            case FLAG_RCV:
                if (byte == address)
                    cur_state = A_REC;
                else cur_state = HEADER_ERR;
                break;
            
            case A_REC:
                if (byte == ctrl || byte == inv_ctrl)
                    cur_state = C_REC;
                else cur_state = HEADER_ERR;
                break;
            
            case C_REC:
                if (BCC(address,ctrl) == byte)
                    cur_state = DATA;
                else if (BCC(address, inv_ctrl) == byte)
                    cur_state = SEQNUM_ERR;
                else
                    cur_state = HEADER_ERR;
                break;
            
            case DATA:
                if (index == v.size - 1)
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
                        cur_bcc = byte;
                        first_data = false;
                        second_data = true;
                    }

                    else if(second_data) {
                        second_data = false;
                    }

                    else
                        cur_bcc = BCC(cur_bcc, prev_byte);
                    
                    prev_byte = byte;

                }
                break;

            default:
                break;
        }
        index++;
    }

    if(cur_state == STOP)
    {
        //remove header
        for (int i = 0; i < 4; i++)
            vector_delete(&v, 0);
        
        //remove tail
        vector_delete(&v, v.size-1);
        vector_delete(&v, v.size-1);

        int size = v.size;

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
