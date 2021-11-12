#include "comms.h"

int send_s_u_frame(int fd, Source src, int ctrl)
{
  char buf[5];

  buf[0] = FLAG;
  buf[1] = src == SENDER ? A_SND : A_RCV;
  buf[2] = ctrl;
  buf[3] = BCC(buf[1], buf[2]);
  buf[4] = FLAG;

  return write(fd, buf, 5);
}