#include "csapp.h"
#include "ftp_com.h"

int main(int argc, char** argv)
{
  int        clientfd, port;
  char*      host;
  long const BUF_SIZE = 8192l;
  char       buf[BUF_SIZE];
  char       file_name[MAXLINE];
  rio_t      rio;

  if (argc != 2)
  {
    fprintf(stderr, "usage: %s <host>\n", argv[0]);
    exit(0);
  }
  host = argv[1];
  port = 2121;

  /*
   * Note that the 'host' can be a name or an IP address.
   * If necessary, Open_clientfd will perform the name resolution
   * to obtain the IP address.
   */
  clientfd = Open_clientfd(host, port);

  /*
   * At this stage, the connection is established between the client
   * and the server OS ... but it is possible that the server application
   * has not yet called "Accept" for this connection
   */
  printf("client connected to server OS\n");
  printf("Enter file name to get: ");

  Rio_readinitb(&rio, clientfd);

  if (Fgets(file_name, MAXLINE, stdin) != NULL)
  {
    size_t file_name_s = strlen(file_name);
    Rio_writen(clientfd, file_name, file_name_s);
    ssize_t nb = Rio_readlineb(&rio, buf, BUF_SIZE);
    buf[nb] = '\0';
    if (strcmp(buf, FTP_OK) == 0)
    {
      // nb bytes of the file
      nb = Rio_readlineb(&rio, buf, BUF_SIZE);
      buf[nb - 1] = '\0';
      // convert nb bytes to integer
      long size = atoi(buf);
      // file to write
      file_name[--file_name_s] = '\0';
      FILE* f = Fopen(file_name, "w");
      printf("getting %s (%ldB '%s')\n", file_name, size, buf);
      while (size > BUF_SIZE)
      {
        // get the file
        Rio_readnb(&rio, buf, sizeof(buf));
        // write the file
        Fwrite(buf, sizeof(*buf), BUF_SIZE, f);
        size -= BUF_SIZE;
      }
      // get the file
      Rio_readnb(&rio, buf, size);
      // write the file
      Fwrite(buf, sizeof(*buf), size, f);
    }
    else
    {
      printf("Can't download file\n");
    }
  }
  Close(clientfd);
  exit(0);
}
