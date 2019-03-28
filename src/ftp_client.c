#include "UI.h"
#include "csapp.h"
#include "format.h"
#include "ftp_com.h"
#include "utils.h"
#include <time.h>

void fexist_act(char const* file_name, char opt)
{
  char opts[] = " \n";
  opts[0] = opt;
  char const* act;
  if (opt == 'r')
  {
    act = "rename";
  }
  else if (opt == 'c')
  {
    act = "complete";
  }
  else
  {
    printf("fexist_act unsupported opt %c\n", opt);
    exit(3);
  }

  if (access(file_name, F_OK) == 0)
  {
    printf("The file : '%s' already exists, do you want to %s(%c) or to replace(x) "
           "it?\n",
           file_name, act, opt);

    bool correct = true;
    do
    {
      size_t sz = 0;
      char*  buf = NULL;
      getline(&buf, &sz, stdin);
      if (strcmp(buf, "x\n") == 0)
      {
        remove(file_name);
        free(buf);
      }
      else if (opt == 'r' && strcmp(buf, "r\n") == 0)
      {
        printf("Please enter a file name:\n");
        free(buf);
        sz = 0;
        buf = NULL;
        sz = getline(&buf, &sz, stdin);
        buf[sz - 1] = '\0';
        rename(file_name, buf);
        printf("renamed '%s' to '%s'\n", file_name, buf);
        free(buf);
      }
      else if (opt == 'c' && strcmp(buf, "c\n") == 0)
      {
        free(buf);
        // nothing to do
      }
      else
      {
        correct = false;
        printf("please enter %c or x: ", opt);
        free(buf);
      }
    } while (!correct);
  }
}

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
  // 1. send file name
  send_line(rio, file_name);

  // 2. receive ok/err
  int err = receive_long(rio);

  if (err == 0) // no error
  {
    fexist_act(file_name, 'r');

    // 3. nb bytes of the file
    size_t size = receive_size_t(rio);
    // 4. nb bytes of a block
    size_t blk_size = receive_size_t(rio);

    size_t fnpsz = strlen(file_name) + sizeof(".part");
    char   file_name_part[fnpsz];
    snprintf(file_name_part, fnpsz, "%s.part", file_name);
    fexist_act(file_name_part, 'c');

    printf("getting %s (", file_name);
    printf_bytes(size);
    printf(")\n");

    // file to write
    Seg_File sf;
    if (sf_init(&sf, file_name_part, size, SF_READ_WRITE, blk_size) == 0)
    {
      size_t nb_blocks_req = sf_nb_blk_req(&sf);
      size_t nb_blocks = sf_nb_blk(&sf);
      printf("%zu blocks of ", nb_blocks_req);
      printf_bytes(sf.blk_size);
      printf("\n");

      long remaining = size;
      Bar  bDownload;
      init_bar(&bDownload, size);

      size_t no = 0;
      // 5. command sequence to get the file
      // check the available blocks
      Check_Sum s;
      Check_Sum s_dist;
      Block     b;
      while (no < nb_blocks && sf_receive_blk_sum(&sf, rio, no, &s_dist))
      {
        b = sf_get_blk(&sf, no);
        sf_blk_sum(b, &s);
        if (!check_sum_equal(&s_dist, &s))
        {
          sf_receive_blk(&sf, rio, no);
        }
        download_bar(&bDownload, size - remaining);
        remaining -= sf.blk_size;
        ++no;
      }

      // fetch the remaining blocks
      while (no < nb_blocks_req && sf_receive_blk(&sf, rio, no))
      {
        download_bar(&bDownload, size - remaining);
        remaining -= sf.blk_size;
        ++no;
      }
      sf_destroy(&sf);
      if (no == nb_blocks_req)
      {
        progress_bar(1.f);
        rename(file_name_part, file_name);
      }
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
    errno = err;
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
