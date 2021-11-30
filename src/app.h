#ifndef APP_H
#define APP_H

#include "ll.h"
#include "utils.h"

#define MAX_PACK_SIZE 256

typedef struct {
    char T;
    char L;
    char *V;
} TLV;

int app_start(int port, Source status);

int app_end();

int send_control_packet(char ctrl, TLV* tlv_arr, unsigned n_tlv);

int receive_start_packet();

#endif