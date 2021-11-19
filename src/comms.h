#define MAX_SIZE 500

typedef enum {SENDER, RECEIVER} Source;

int send_s_u_frame(int fd, Source src, int ctrl);