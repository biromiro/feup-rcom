#define MAX_SIZE 500

typedef enum {SENDER, RECEIVER} Source;

typedef struct linkLayer {
    char port[20];
    int baudRate;
    unsigned sequenceNumber;
    unsigned timeout;
    unsigned numTransmissions;

    char frame[MAX_SIZE];
};

int send_s_u_frame(int fd, Source src, int ctrl);