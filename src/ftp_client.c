#include "csapp.h"
#include "ftp_com.h"

void get_file(rio_t* rio, int clientfd, char* file_name)
{
  long const BUF_SIZE = 8192;
  char       buf[BUF_SIZE];

  send_line(clientfd, file_name);

  size_t file_name_s = strlen(file_name);
  file_name[--file_name_s] = '\0';

  receive_line(rio, buf, BUF_SIZE);
  if (strcmp(buf, FTP_OK) == 0)
  {
    // nb bytes of the file
    receive_line(rio, buf, BUF_SIZE);
    // convert nb bytes to integer
    long size = atoi(buf);
    // file to write
    FILE* f = Fopen(file_name, "w");
    printf("getting %s (%ldB '%s')\n", file_name, size, buf);
    while (size > BUF_SIZE)
    {
      // get the file
      Rio_readnb(rio, buf, sizeof(buf));
      // write the file
      Fwrite(buf, sizeof(*buf), BUF_SIZE, f);
      size -= BUF_SIZE;
    }
    // get the file
    Rio_readnb(rio, buf, size);
    // write the file
    Fwrite(buf, sizeof(*buf), size, f);
  }
  else
  {
    printf("Can't download file\n");
  }
}

char* cut_first(char* str, char c)
{
  char* tmp = str;
  while (*tmp != '\0' && *tmp != c)
  {
    tmp++;
  }
  if (*tmp == c)
  {
    *tmp = '\0';
    return tmp + 1;
  }
  else
  {
    return str;
  }
}

void help()
{
  printf("Commands:\n");
  printf("get <file_name>          get file_name from the server\n");
  printf("bye                      quit\n\n");
}

void command(rio_t* rio, int clientfd, char* cmd)
{
  char* args = cut_first(cmd, ' ');

  if (strcmp(cmd, "get") == 0)
  {
    send_line(clientfd, cmd);
    get_file(rio, clientfd, args);
  }
  else if (strcmp(cmd, "help") == 0)
  {
    help();
  }
  else if (strcmp(cmd, "bye") == 0)
  {
    exit(0);
  }
  else
  {
    printf("command '%s' does not exists, type help\n", cmd);
  }
}

int main(int argc, char** argv)
{
  int        clientfd, port;
  char*      host;
  long const CMD_SIZE = 8192l;
  char       cmd[CMD_SIZE];
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

  /*
   * At this stage, the connection is established between the client
   * and the server OS ... but it is possible that the server application
   * has not yet called "Accept" for this connection
   */
  // printf("client connected to server OS\n");

  printf("ftp > ");
  while (Fgets(cmd, CMD_SIZE, stdin) != NULL)
  {
    clientfd = Open_clientfd(host, port);
    Rio_readinitb(&rio, clientfd);

    cmd[strlen(cmd) - 1] = '\0';
    command(&rio, clientfd, cmd);
    Close(clientfd);
    printf("ftp > ");
  }
  exit(0);
}