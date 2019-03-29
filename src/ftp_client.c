#include "UI.h"
#include "csapp.h"
#include "format.h"
#include "ftp_com.h"
#include "utils.h"
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

void fexist_act(char const* file_name, char opt)
{
  // char opts[] = " \n";
  // opts[0] = opt;
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
  // handle existing file/file.part
  fexist_act(file_name, 'r');

  size_t fnpsz = strlen(file_name) + sizeof(".part");
  char   file_name_part[fnpsz];
  snprintf(file_name_part, fnpsz, "%s.part", file_name);
  fexist_act(file_name_part, 'c');

  send_line(rio, "get");
  // 1. send file name
  send_line(rio, file_name);

  printf("getting %s\n", file_name);
  if (receive_file(rio, file_name_part))
  {
    rename(file_name_part, file_name);
  }
}

void put_file(rio_t* rio, char* file_name)
{
  if (access(file_name, F_OK) == 0)
  {
    send_line(rio, "put");

    // 1. send file name
    send_line(rio, file_name);

    send_file(rio, file_name, BLK_SIZE);
  }
  else
  {
    printf("%s doesn't exist", file_name);
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
    get_file(rio, args);
  }
  else if (strcmp(cmd, "put") == 0)
  {
    put_file(rio, args);
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

void disp_client(Log_Level ll, char const* format, ...)
{
  if (ll > LOG_LV_LOG)
  {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
  }
}

int main(int argc, char** argv)
{
  fc_disp = disp_client;
  fc_show_progress_bar = true;

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
