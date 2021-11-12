typedef enum Source {SENDER, RECEIVER};

int send_s_u_frame(int fd, Source src, int ctrl);