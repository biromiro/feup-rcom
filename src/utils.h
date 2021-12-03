#ifndef UTILS_H
#define UTILS_H

#define FLAG 0x7e
#define ESC 0x7d
#define ESCAPED(b) (0x20^b)
#define A_SND 0x03
#define A_RCV 0x01
#define SET 0x03
#define UA 0x07
#define DISC 0x0B
#define RR(r) ((r << 7) | 0x05)
#define REJ(r) ((r << 7) | 0x01)
#define BCC(a,b) (a^b)
#define invSN(sn) (sn ? 0 : 1)

#define MAX_PORT_SIZE 20
#define BAUDRATE B38400
#define TIMEOUT 30
#define NUM_TRANSM 4

#include <termios.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef enum {SENDER, RECEIVER} Source;

typedef enum {START, FLAG_RCV, A_REC, C_REC, BCC_OK, DATA, STOP, HEADER_ERR, DATA_ERR, SEQNUM_ERR} State;

typedef struct {
    char port[MAX_PORT_SIZE];
    int baudRate;
    unsigned sequenceNumber;
    unsigned timeout;
    unsigned numTransmissions;
} linkLayer;

int llconfig_reset(linkLayer* ll);

int llconfig(int fd, linkLayer* ll, struct termios *oldtio);

#endif