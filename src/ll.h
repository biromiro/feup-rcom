#ifndef LL_H
#define LL_H

#include "comms.h"
#include "utils.h"

int llopen(int port, Source src);

int llwrite(int fd, char *buffer, int length);

int llread(int fd, char *buffer);

int llclose(int fd);

#endif