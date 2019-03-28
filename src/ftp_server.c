#include "csapp.h"
#include "format.h"
#include "ftp_com.h"
#include <openssl/sha.h>
#include <stdarg.h>
#include <sys/stat.h>

#define MAX_NAME_LEN 256
#define NB_CHILDREN 8
#define BLK_SIZE (1 << 25)

void chld_handler(int ntm)
{
  (void)ntm;

  int   status;
  pid_t p = waitpid(0, &status, WNOHANG);
  if (p > 0)
  {
    printf("%d: exit: %d, exited: %d, signaled: %d, sig: %d\n", p, WEXITSTATUS(status),
           WIFEXITED(status), WIFSIGNALED(status), WTERMSIG(status));
  }
}

void get_file(rio_t* rio);
void put_file(rio_t* rio);
void fpt_ls(rio_t* rio);
void fpt_pwd(rio_t* rio);
void fpt_cd(rio_t* rio);
void fpt_mkdir(rio_t* rio);
void ftp_rm(rio_t* rio);

pid_t children[NB_CHILDREN] = {0};
int   serv_no = -1;

void ctrlc(int ntm)
{
  (void)ntm; // unused

  for (int i = 0; i < NB_CHILDREN; i++)
  {
    if (children[i])
    {
      kill(children[i], SIGINT);
    }
  }
  exit(0);
}

/**
 * @brief print header of server (no child)
 * 
 */
void disp_serv(char const* format, ...)
{
  if (serv_no == -1)
  {
    printf("[Server] ");
  }
  else
  {
    printf("[Server no: %d] ", serv_no);
  }

  va_list args;
  va_start(args, format);
  vprintf(format, args);
  va_end(args);
}

void command(rio_t* rio)
{
  char cmd[FTP_MAX_CMD_LEN];

  // reads the client's command
  while (receive_line(rio, cmd, FTP_MAX_CMD_LEN) > 0 && strcmp(cmd, "bye") != 0)
  {
    disp_serv("command '%s' received\n", cmd);
    if (strcmp(cmd, "get") == 0)
    {
      get_file(rio);
    }
  }
}

/* 
 * Note that this code only works with IPv4 addresses
 * (IPv6 is not supported)
 */
int main()
{
  int                listenfd;
  int                connfd;
  int                port;
  socklen_t          clientlen;
  struct sockaddr_in clientaddr;
  char               client_ip_string[INET_ADDRSTRLEN];
  char               client_hostname[MAX_NAME_LEN];

  port = 2121;

  clientlen = (socklen_t)sizeof(clientaddr);

  listenfd = Open_listenfd(port);

  for (int i = 0; i < NB_CHILDREN; i++)
  {
    children[i] = Fork();
    if (children[i] == 0)
    {
      serv_no = i;
      break;
    }
  }

  if (children[NB_CHILDREN - 1] != 0)
  {
    char cmd[FTP_MAX_CMD_LEN];
    signal(SIGINT, ctrlc);
    signal(SIGCHLD, chld_handler);
    disp_serv("OK\n");
    while (Fgets(cmd, FTP_MAX_CMD_LEN, stdin) != NULL)
    {
      cmd[strlen(cmd) - 1] = '\0';

      if (strcmp(cmd, "bye") == 0)
      {
        kill(getpid(), SIGINT);
      }
    }
    kill(getpid(), SIGINT);
  }
  else
  {
    disp_serv("actif\n");
    rio_t rio;
    while (1)
    {
      connfd = Accept(listenfd, (SA*)&clientaddr, &clientlen);
      Rio_readinitb(&rio, connfd);

      /* determine the name of the client */
      Getnameinfo((SA*)&clientaddr, clientlen, client_hostname, MAX_NAME_LEN, 0, 0, 0);

      /* determine the textual representation of the client's IP address */
      Inet_ntop(AF_INET, &clientaddr.sin_addr, client_ip_string, INET_ADDRSTRLEN);

      disp_serv("server connected to %s (%s)\n", client_hostname, client_ip_string);

      command(&rio);
      Close(connfd);
      disp_serv("actif\n");
    }
  }
}

/**
 * @brief send file
 * Protocol:
 * 1. receive file name
 * 2. send ok/err (if file is ready)
 * 3. send file size
 * 4. send blk size
 * 5. wait for commands:
 *   - get_blk \n no
 *   - get_blk_sum \n no
 * 6. receiving "get_end" end sending file
 * 
 * @param connfd 
 * @param rio 
 */
void get_file(rio_t* rio)
{
  size_t n;
  char   buf[FTP_MAX_LINE_SIZE];

  // 1. receive file name
  if ((n = receive_line(rio, buf, FTP_MAX_LINE_SIZE)) != 0)
  {
    disp_serv("Asked for: '%s'\n", buf);
    Seg_File sf;
    int      err = sf_init(&sf, buf, 0, SF_READ, BLK_SIZE);

    if (err == 0)
    {
      size_t nb_blk = sf_nb_blk_req(&sf);

      disp_serv("file found\n");
      // 2. ok
      send_long(rio, 0);

      // handle size

      // 3. file size
      send_size_t(rio, sf.size);
      // 4. blk size
      send_size_t(rio, sf.blk_size);

      disp_serv("size of file: %ld bytes (", sf.size);
      printf_bytes(sf.size);
      printf(")\n");
      disp_serv("%zu blocks of %zu bytes\n", nb_blk, sf.blk_size);

      // file is ready
      // 5. waiting for client commands

      while ((n = receive_line(rio, buf, FTP_MAX_LINE_SIZE)) != 0 &&
             strcmp(buf, GET_END) != 0)
      {
        size_t no = receive_size_t(rio);
        disp_serv("get subcommand '%s %zu' received\n", buf, no);

        if (no < nb_blk)
        {
          if (strcmp(buf, GET_BLK) == 0)
          {
            sf_send_blk(&sf, rio, no);
          }
          else if (strcmp(buf, GET_BLK_SUM) == 0)
          {
            sf_send_blk_sum(&sf, rio, no);
          }
          else
          {
            // Unknown command !
          }
        }
      }
      // 6. end

      sf_destroy(&sf);
    }
    else
    {
      // 2. err
      perror("get error");
      send_long(rio, errno);
      errno = 0;
    }
  }
}
