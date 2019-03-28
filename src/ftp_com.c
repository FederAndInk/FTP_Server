#include "ftp_com.h"
#include "format.h"
#include "utils.h"
#include <math.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_SIZET_LEN 21

bool sha512_equal(sha512_sum* s1, sha512_sum* s2)
{
  size_t i = 0;

  while (i < sizeof(s1->sum) && s1->sum[i] == s2->sum[i])
  {
    ++i;
  }
  return i == sizeof(s1->sum);
}

ssize_t receive_line(rio_t* rio, char* str, size_t maxlen)
{
  ssize_t n = rio_readlineb(rio, str, maxlen - 1);
  if (n > 0)
  {
    str[n - 1] = '\0';
  }
  else if (n == 0)
  {
    str[0] = '\0';
  }
  else
  {
    str[0] = '\0';
    // error
    printf("error receive_live");
  }

  return n;
}

size_t receive_size_t(rio_t* rio)
{
  char buf[MAX_SIZET_LEN] = {0};

  receive_line(rio, buf, MAX_SIZET_LEN);
  return strtoul(buf, NULL, 10);
}

void send_size_t(rio_t* rio, size_t s)
{
  char buf[MAX_SIZET_LEN] = {0};
  snprintf(buf, MAX_SIZET_LEN - 1, "%zu", s);
  send_line(rio, buf);
}

long receive_long(rio_t* rio)
{
  char buf[MAX_SIZET_LEN] = {0};
  receive_line(rio, buf, MAX_SIZET_LEN);
  return strtol(buf, NULL, 10);
}

void send_long(rio_t* rio, long l)
{
  char buf[MAX_SIZET_LEN] = {0};
  snprintf(buf, MAX_SIZET_LEN - 1, "%ld", l);
  send_line(rio, buf);
}

void send_line(rio_t* rio, char const* str)
{
  size_t strl = strlen(str);
  if (rio_writen(rio->rio_fd, str, strl) == strl)
  {
    rio_writen(rio->rio_fd, "\n", 1);
  }
  else
  {
    printf("error send line '%s'\n", str);
  }
}

int sf_init(Seg_File* sf, char const* file_name, size_t req_size, Seg_File_Mode sfm,
            size_t blk_size)
{
  struct stat file_info;
  errno = 0;

  if (stat(file_name, &file_info) == 0 || (errno == ENOENT && req_size != 0))
  {
    if (errno == ENOENT)
    {
      errno = 0;
      sf->size = 0;
    }
    else
    {
      sf->size = file_info.st_size;
    }
    sf->req_size = req_size == 0 ? sf->size : req_size;
    sf->blk_size = blk_size;

    switch (sfm)
    {
    case SF_READ:
      sf->fd = open(file_name, O_RDONLY);
      if (sf->fd >= 0)
      {
        sf->data =
            (unsigned char*)mmap(NULL, sf->req_size, PROT_READ, MAP_SHARED, sf->fd, 0);
      }
      break;
    case SF_READ_WRITE:
      sf->fd = open(file_name, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IROTH | S_IRGRP);
      if (sf->fd >= 0)
      {
        sf->data = (unsigned char*)mmap(NULL, sf->req_size, PROT_READ | PROT_WRITE,
                                        MAP_SHARED, sf->fd, 0);
      }
      break;
    default:
      fprintf(stderr, "error: unknown seg_file_mode: %d", sfm);
      exit(2);
    }
    return errno == 0 ? 0 : -1;
  }
  else
  {
    return -1;
  }
}

size_t sf_nb_blk(Seg_File* sf)
{
  return ceil((double)sf->size / (double)sf->blk_size);
}

size_t sf_nb_blk_req(Seg_File* sf)
{
  return ceil((double)sf->req_size / (double)sf->blk_size);
}

Block sf_get_blk(Seg_File* sf, size_t no)
{
  Block b;

  size_t off = no * (sf->blk_size);

  if (sf_nb_blk_req(sf) - 1 == no)
  {
    b.blk_size = (sf->req_size) % (sf->blk_size);
    if (b.blk_size == 0)
    {
      b.blk_size = sf->blk_size;
    }
  }
  else
  {
    b.blk_size = sf->blk_size;
  }

  b.data = sf->data + off;

  if (off + b.blk_size > sf->size)
  {
    sf->size = off + b.blk_size;
    ftruncate(sf->fd, sf->size);
  }

  return b;
}

void sf_blk_sum(Block blk, sha512_sum* sum)
{
  SHA512(blk.data, blk.blk_size, sum->sum);
}

void sf_send_blk(Seg_File* sf, rio_t* rio, size_t no_blk)
{
  Block   b = sf_get_blk(sf, no_blk);
  ssize_t wrt = rio_writen(rio->rio_fd, b.data, b.blk_size);
  if (wrt != b.blk_size)
  {
    printf("error sending blk no %zu (", no_blk);
    printf_bytes(wrt);
    printf(")\n");
  }
}

void sf_send_blk_sum(Seg_File* sf, rio_t* rio, size_t no_blk)
{
  Block b = sf_get_blk(sf, no_blk);

  sha512_sum s;
  sf_blk_sum(b, &s);
  rio_writen(rio->rio_fd, s.sum, sizeof(s.sum));
}

bool sf_receive_blk(Seg_File* sf, rio_t* rio, size_t no_blk)
{
  Block b = sf_get_blk(sf, no_blk);

  send_line(rio, GET_BLK);
  send_size_t(rio, no_blk);

  return rio_readnb(rio, b.data, b.blk_size) == (ssize_t)b.blk_size;
}

bool sf_receive_blk_sum(Seg_File* sf, rio_t* rio, size_t no_blk, sha512_sum* sum)
{
  send_line(rio, GET_BLK_SUM);
  send_size_t(rio, no_blk);

  return rio_readnb(rio, sum->sum, sizeof(sum->sum)) == sizeof(sum->sum);
}

void sf_destroy(Seg_File* sf)
{
  close(sf->fd);
  munmap(sf->data, sf->req_size);
}
