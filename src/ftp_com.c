#include "ftp_com.h"
#include "utils.h"
#include <math.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

ssize_t receive_line(rio_t* rp, char* str, size_t maxlen)
{
  ssize_t n = Rio_readlineb(rp, str, maxlen - 1);
  if (n > 0)
  {
    str[n - 1] = '\0';
  }
  else
  {
    str[n] = '\0';
  }

  return n;
}

void send_line(int fd, char const* str)
{
  Rio_writen(fd, str, strlen(str));
  Rio_writen(fd, "\n", 1);
}

void sf_init(Seg_File* sf, char const* file_name, size_t req_size, Seg_File_Mode sfm,
             size_t blk_size)
{
  struct stat file_info;
  stat(file_name, &file_info);
  sf->size = file_info.st_size;
  sf->req_size = req_size == 0 ? sf->size : req_size;
  sf->blk_size = blk_size;

  switch (sfm)
  {
  case SF_READ:
    sf->fd = open(file_name, O_RDONLY);
    sf->data = (unsigned char*)mmap(NULL, sf->req_size, PROT_READ, MAP_SHARED, sf->fd, 0);
    break;
  case SF_READ_WRITE:
    sf->fd = open(file_name, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IROTH | S_IRGRP);
    sf->data = (unsigned char*)mmap(NULL, sf->req_size, PROT_READ | PROT_WRITE,
                                    MAP_SHARED, sf->fd, 0);
    break;
  default:
    fprintf(stderr, "error: unknown seg_file_mode: %d", sfm);
    exit(2);
  }
}

size_t sf_nb_blk(Seg_File* sf)
{
  return ceil((double)sf->req_size / (double)sf->blk_size);
}

Block sf_get_blk(Seg_File* sf, size_t no)
{
  Block b;

  size_t off = no * (sf->blk_size);

  if (sf_nb_blk(sf) - 1 == no)
  {
    b.blk_size = (sf->req_size) % sf->blk_size;
    //handle the case where we have to increase file size
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

void sf_destroy(Seg_File* sf)
{
  close(sf->fd);
  munmap(sf->data, sf->req_size);
}
