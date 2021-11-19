#include "comms.h"
#include "utils.h"

typedef struct linkLayer {
    char port[20];
    int baudRate;
    unsigned sequenceNumber;
    unsigned timeout;
    unsigned numTransmissions;

    char frame[MAX_SIZE];
};

int llconfig(linkLayer ll);

int llopen(int port, Source src);

int llclose(int fd);