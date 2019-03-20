#include "csapp.h"
#include "ftp_com.h"
#include <sys/stat.h>

#define MAXINTLEN 20
#define MAX_NAME_LEN 256
#define NB_CHILDREN 8

void get_file(int connfd, rio_t* rio);
void put_file(int connfd, rio_t* rio);
void fpt_ls(int connfd, rio_t* rio);
void fpt_pwd(int connfd, rio_t* rio);
void fpt_cd(int connfd, rio_t* rio);
void fpt_mkdir(int connfd, rio_t* rio);
void ftp_rm(int connfd, rio_t* rio);

pid_t children[NB_CHILDREN] = {0};
int   serv_no = 0;

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

void disp_serv()
{
  printf("[Server no: %d] ", serv_no);
}

void command(int connfd)
{
  rio_t rio;
  char  cmd[MAX_CMD_LEN];
  Rio_readinitb(&rio, connfd);

  receive_line(&rio, cmd, MAX_CMD_LEN);
  disp_serv();
  printf("command '%s' received\n", cmd);
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

  for (int i = 1; i < NB_CHILDREN; i++)
  {
    children[i] = Fork();
    if (children[i] == 0)
    {
      serv_no = i;
      break;
    }
  }

  if (children[NB_CHILDREN - 2] != 0)
  {
    signal(SIGINT, ctrlc);
  }

  disp_serv();
  printf("actif\n");

  while (1)
  {
    connfd = Accept(listenfd, (SA*)&clientaddr, &clientlen);

    /* determine the name of the client */
    Getnameinfo((SA*)&clientaddr, clientlen, client_hostname, MAX_NAME_LEN, 0, 0, 0);

    /* determine the textual representation of the client's IP address */
    Inet_ntop(AF_INET, &clientaddr.sin_addr, client_ip_string, INET_ADDRSTRLEN);

    disp_serv();
    printf("server connected to %s (%s)\n", client_hostname, client_ip_string);

    command(connfd);
    Close(connfd);
  }

  exit(0);
}

void get_file(int connfd, rio_t* rio)
{
  size_t      n;
  char        buf[MAXLINE];
  struct stat st;
  char        file_size[MAXINTLEN] = {'\0'};

  if ((n = receive_line(rio, buf, MAXLINE)) != 0)
  {
    printf("Asked for: '%s'\n", buf);
    errno = 0;
    int fd = open(buf, O_RDONLY);

    if (fd > 0)
    {
      printf("file found\n");
      send_line(connfd, FTP_OK);
      fstat(fd, &st);
      snprintf(file_size, MAXINTLEN - 1, "%ld", st.st_size);
      send_line(connfd, file_size);
      printf("size of file: %ld\n", st.st_size);

      char* file = (char*)mmap(0, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
      Rio_writen(connfd, file, st.st_size);
      munmap(file, st.st_size);
      close(fd);
    }
    else
    {
      printf("file not found\n");
      send_line(connfd, FTP_NO_FILE);
    }
  }
}
