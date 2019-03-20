#include "ftp_com.h"

ssize_t receive_line(rio_t* rp, char* str, size_t maxlen)
{
  ssize_t n = Rio_readlineb(rp, str, maxlen - 1);
  if (n > 0)
  {
    str[n - 1] = '\0';
  }
  else
  {
    str[n] = '\0';
  }

  return n;
}

void send_line(int fd, char const* str)
{
  Rio_writen(fd, str, strlen(str));
  Rio_writen(fd, "\n", 1);
}
