#include "csapp.h"
#include "ftp_com.h"
#include <sys/stat.h>

#define MAXINTLEN 20
#define MAX_NAME_LEN 256
#define NB_CHILDREN 2

void get_file(int connfd);

pid_t children[NB_CHILDREN] = {0};

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

/* 
 * Note that this code only works with IPv4 addresses
 * (IPv6 is not supported)
 */
int main(int argc, char** argv)
{
  int                listenfd;
  int                connfd;
  int                port;
  socklen_t          clientlen;
  struct sockaddr_in clientaddr;
  char               client_ip_string[INET_ADDRSTRLEN];
  char               client_hostname[MAX_NAME_LEN];

  if (argc != 2)
  {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(0);
  }
  port = atoi(argv[1]);

  clientlen = (socklen_t)sizeof(clientaddr);

  listenfd = Open_listenfd(port);

  for (int i = 0; i < NB_CHILDREN - 1; i++)
  {
    children[i] = Fork();
    if (children[i] == 0)
    {
      break;
    }
  }

  if (children[0] != 0)
  {
    signal(SIGINT, ctrlc);
  }

  while (1)
  {
    connfd = Accept(listenfd, (SA*)&clientaddr, &clientlen);

    /* determine the name of the client */
    Getnameinfo((SA*)&clientaddr, clientlen, client_hostname, MAX_NAME_LEN, 0, 0, 0);

    /* determine the textual representation of the client's IP address */
    Inet_ntop(AF_INET, &clientaddr.sin_addr, client_ip_string, INET_ADDRSTRLEN);

    printf("server connected to %s (%s)\n", client_hostname, client_ip_string);

    get_file(connfd);
    Close(connfd);
  }

  exit(0);
}

void Rio_write_str(int fd, char* str)
{
  Rio_writen(fd, str, (strlen(str)) * sizeof(char));
}

void get_file(int connfd)
{
  size_t      n;
  char        buf[MAXLINE];
  rio_t       rio;
  struct stat st;
  char        file_size[MAXINTLEN] = {'\0'};

  Rio_readinitb(&rio, connfd);
  if ((n = Rio_readlineb(&rio, buf, MAXLINE - 1)) != 0)
  {
    // buf has a new line at the end
    buf[n - 1] = '\0';
    printf("Asked for: '%s'\n", buf);
    errno = 0;
    int fd = open(buf, O_RDONLY);

    if (fd > 0)
    {
      printf("file found\n");
      Rio_write_str(connfd, FTP_OK);
      fstat(fd, &st);
      snprintf(file_size, MAXINTLEN - 1, "%ld\n", st.st_size);
      Rio_write_str(connfd, file_size);
      printf("size of file: %ld\n", st.st_size);

      char* file = (char*)mmap(0, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
      Rio_writen(connfd, file, st.st_size);
      munmap(file, st.st_size);
      close(fd);
    }
    else
    {
      printf("file not found\n");
      Rio_write_str(connfd, FTP_NO_FILE);
    }
  }
}
