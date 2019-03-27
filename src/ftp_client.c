#include "UI.h"
#include "csapp.h"
#include "format.h"
#include "ftp_com.h"
#include "utils.h"
#include <time.h>

/**
 * @brief receive file
 * Protocol:
 * 1. send file name
 * 2. receive ok/err (if file is ready)
 * 3. receive file size
 * 4. receive blk size
 * 5. send commands 
 *   (depending on what we already have and what the user want):
 *   - get_blk \n no
 *   - get_blk_sum \n no
 * 6. send "get_end" end receiving file
 * 
 * @param rio 
 * @param file_name 
 */
void get_file(rio_t* rio, char* file_name)
{
  char buf[FTP_MAX_LINE_SIZE];

  // 1. send file name
  send_line(rio, file_name);

  // 2. receive ok/err
  receive_line(rio, buf, sizeof(buf));

  if (strcmp(buf, FTP_OK) == 0)
  {
    // 3. nb bytes of the file
    size_t size = receive_size_t(rio);
    // 4. nb bytes of a block
    size_t blk_size = receive_size_t(rio);

    printf("getting %s (", file_name);
    printf_bytes(size);
    printf(")\n");

    // file to write
    Seg_File sf;
    if (sf_init(&sf, file_name, size, SF_READ_WRITE, blk_size) == 0)
    {
      size_t nb_blocks = sf_nb_blk(&sf);
      printf("%zu blocks of ", nb_blocks);
      printf_bytes(sf.blk_size);
      printf("\n");

      long remaining = size;
      Bar  bDownload;
      init_bar(&bDownload, size);

      size_t no = 0;
      // 5. command sequence to get the file
      while (no < nb_blocks && sf_receive_blk(&sf, rio, no))
      {
        download_bar(&bDownload, size - remaining);
        remaining -= sf.blk_size;
        ++no;
      }
      progress_bar(1.f);
      printf("\n");
      // 6. end get
    }
    else
    {
      perror("error opening file");
    }
    send_line(rio, GET_END);
  }
  else
  {
    errno = receive_long(rio);
    perror("Can't download file");
    errno = 0;
  }
}

void help()
{
  printf("Commands:\n");
  printf("get <file_name>          get file_name from the server\n");
  printf("bye                      quit\n\n");
}

void command(rio_t* rio, char* cmd)
{
  char* args = cut_first(cmd, ' ');

  if (strcmp(cmd, "get") == 0)
  {
    send_line(rio, cmd);
    get_file(rio, args);
  }
  else if (strcmp(cmd, "help") == 0)
  {
    help();
  }
  else if (strcmp(cmd, "bye") == 0)
  {
    send_line(rio, "bye");
    exit(0);
  }
  else
  {
    printf("command '%s' does not exists, type help\n", cmd);
  }
}

int main(int argc, char** argv)
{
  int   clientfd, port;
  char* host;
  char  cmd[FTP_MAX_CMD_LEN];
  rio_t rio;

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
  clientfd = Open_clientfd(host, port);

  Rio_readinitb(&rio, clientfd);
  // printf("client connected to server OS\n");

  printf("ftp > ");
  while (Fgets(cmd, FTP_MAX_CMD_LEN, stdin) != NULL)
  {
    cmd[strlen(cmd) - 1] = '\0';
    command(&rio, cmd);
    printf("ftp > ");
  }
  Close(clientfd);
  exit(0);
}
