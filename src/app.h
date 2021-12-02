#ifndef APP_H
#define APP_H

#include "ll.h"
#include "utils.h"
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <libgen.h>

#define MAX_PACK_SIZE 512

#define MAX_FILENAME_SIZE 128

typedef struct {
    u_int8_t T;
    u_int8_t L;
    u_int8_t *V;
} TLV;

int app_start(int port, Source status);

int app_end();

int send_file(const char *filepath);

int receive_file();

#endif