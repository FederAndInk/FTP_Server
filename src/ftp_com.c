#include "ftp_com.h"
#include "UI.h"
#include "format.h"
#include "utils.h"
#include <math.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_SIZET_LEN 21

void default_disp(char const* format, ...)
{
  va_list args;
  va_start(args, format);
  vprintf(format, args);
  va_end(args);
}

Disp_Fn disp = default_disp;

bool check_sum_equal(Check_Sum* s1, Check_Sum* s2)
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

bool receive_file(rio_t* rio, char const* file_name)
{
  // 2. receive ok/err
  int err = receive_long(rio);

  if (err == 0) // no error
  {

    // 3. nb bytes of the file
    size_t size = receive_size_t(rio);
    // 4. nb bytes of a block
    size_t blk_size = receive_size_t(rio);

    printf("writting %s (", file_name);
    printf_bytes(size);
    printf(")\n");

    // file to write
    Seg_File sf;
    if (sf_init(&sf, file_name, size, SF_READ_WRITE, blk_size) == 0)
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
        remaining -= b.blk_size;
        ++no;
      }

      // fetch the remaining blocks
      while (no < nb_blocks_req && sf_receive_blk(&sf, rio, no))
      {
        b = sf_get_blk(&sf, no);

        download_bar(&bDownload, size - remaining);
        remaining -= b.blk_size;
        ++no;
      }
      sf_destroy(&sf);
      if (no == nb_blocks_req)
      {
        progress_bar(1.f);
        printf("\n");
        return true;
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

  return false;
}

bool receive_exec_command(rio_t* rio, char* res, size_t len)
{
  receive_line(rio, res, len);
  return res[0] != '\x04';
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

bool send_file(rio_t* rio, char const* fn, size_t blk_size)
{
  size_t n;

  Seg_File sf;
  int      err = sf_init(&sf, fn, 0, SF_READ, blk_size);

  if (err == 0)
  {
    char   buf[FTP_MAX_LINE_SIZE];
    size_t nb_blk = sf_nb_blk_req(&sf);

    disp("file found\n");
    // 2. ok
    send_long(rio, 0);

    // handle size

    // 3. file size
    send_size_t(rio, sf.size);
    // 4. blk size
    send_size_t(rio, sf.blk_size);

    disp("size of file: %ld bytes (", sf.size);
    printf_bytes(sf.size);
    printf(")\n");
    disp("%zu blocks of %zu bytes\n", nb_blk, sf.blk_size);

    // file is ready
    // 5. waiting for client commands

    while ((n = receive_line(rio, buf, FTP_MAX_LINE_SIZE)) != 0 &&
           strcmp(buf, GET_END) != 0)
    {
      size_t no = receive_size_t(rio);
      disp("get subcommand '%s %zu' received\n", buf, no);

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
    return true;
  }
  else
  {
    // 2. err
    perror("get error");
    send_long(rio, errno);
    errno = 0;
    return false;
  }
}

void send_exec_command(rio_t* rio, char const* com)
{
  char  buf[FTP_MAX_LINE_SIZE];
  FILE* res = popen(com, "r");
  while (fgets(buf, FTP_MAX_LINE_SIZE, res) != NULL)
  {
    send_line(rio, buf);
  }

  send_line(rio, "\x04");
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

void sf_blk_sum(Block blk, Check_Sum* sum)
{
  SHA256(blk.data, blk.blk_size, sum->sum);
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

  Check_Sum s;
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

bool sf_receive_blk_sum(Seg_File* sf, rio_t* rio, size_t no_blk, Check_Sum* sum)
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
