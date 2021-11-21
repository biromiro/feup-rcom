#ifndef COMMS_H
#define COMMS_H

#include "utils.h"
#include "vector.h"

#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>

int stuff(vector *v);

int send_s_u_frame(int fd, Source src, int ctrl);

int receive_s_u_frame(int fd, Source src, int ctrl);

int send_i_frame(int fd, char *buffer, int length, bool seqNum);

#endif