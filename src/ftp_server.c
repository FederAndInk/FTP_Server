#include "csapp.h"
#include "format.h"
#include "ftp_com.h"
#include <openssl/sha.h>
#include <stdarg.h>
#include <sys/stat.h>

#define MAXLONGLEN 21
#define MAX_NAME_LEN 256
#define NB_CHILDREN 8
#define CMD_SIZE 8192

#define BLK_SIZE (1 << 20)

void get_file(int connfd, rio_t* rio);
void put_file(int connfd, rio_t* rio);
void fpt_ls(int connfd, rio_t* rio);
void fpt_pwd(int connfd, rio_t* rio);
void fpt_cd(int connfd, rio_t* rio);
void fpt_mkdir(int connfd, rio_t* rio);
void ftp_rm(int connfd, rio_t* rio);

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

void command(int connfd)
{
  rio_t rio;
  char  cmd[MAX_CMD_LEN];

  // init rio read
  Rio_readinitb(&rio, connfd);

  // reads the client's command
  receive_line(&rio, cmd, MAX_CMD_LEN);
  disp_serv("command '%s' received\n", cmd);
  if (strcmp(cmd, "get") == 0)
  {
    get_file(connfd, &rio);
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
    char cmd[CMD_SIZE];
    signal(SIGINT, ctrlc);
    disp_serv("OK\n");
    while (Fgets(cmd, CMD_SIZE, stdin) != NULL)
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

    while (1)
    {
      connfd = Accept(listenfd, (SA*)&clientaddr, &clientlen);

      /* determine the name of the client */
      Getnameinfo((SA*)&clientaddr, clientlen, client_hostname, MAX_NAME_LEN, 0, 0, 0);

      /* determine the textual representation of the client's IP address */
      Inet_ntop(AF_INET, &clientaddr.sin_addr, client_ip_string, INET_ADDRSTRLEN);

      disp_serv("server connected to %s (%s)\n", client_hostname, client_ip_string);

      command(connfd);
      Close(connfd);
      disp_serv("actif\n");
    }
  }
}

void get_file(int connfd, rio_t* rio)
{
  size_t n;
  char   buf[MAXLINE];

  if ((n = receive_line(rio, buf, MAXLINE)) != 0)
  {
    disp_serv("Asked for: '%s'\n", buf);
    errno = 0;
    int fd = open(buf, O_RDONLY);

    if (fd > 0)
    {
      char file_size[MAXLONGLEN] = {'\0'};

      disp_serv("file found\n");
      send_line(connfd, FTP_OK);

      // handle size
      struct stat st;
      fstat(fd, &st);

      char* file = (char*)mmap(0, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);

      receive_line(rio, file_size, MAXLONGLEN);
      long request_check = atol(file_size);
      if (request_check != 0)
      {
        disp_serv("checking %d first bytes (", request_check);
        printf_bytes(request_check);
        printf(")\n");

        unsigned char sha[SHA512_DIGEST_LENGTH + 1];

        SHA256((unsigned char const*)file, request_check, sha);
        sha[SHA512_DIGEST_LENGTH] = '\0';

        disp_serv("sending checksum\n");
        send_line(connfd, sha);
      }

      long size = st.st_size - request_check;

      snprintf(file_size, MAXLONGLEN - 1, "%ld", size);
      send_line(connfd, file_size);
      disp_serv("size of file: %ld bytes (", st.st_size);
      printf_bytes(st.st_size);
      printf(")\n");
      if (request_check != 0)
      {
        disp_serv("Only sending: %ld last bytes (", size);
        printf_bytes(size);
        printf(")\n");
      }

      Rio_writen(connfd, file + request_check, size);
      munmap(file, st.st_size);
      close(fd);
    }
    else
    {
      perror("get error");
      n = snprintf(buf, MAXLINE, "%d", errno);
      buf[n] = '\0';
      send_line(connfd, buf);
    }
  }
}
