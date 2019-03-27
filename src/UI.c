#include "UI.h"
#include "format.h"
#include <stdint.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>

char const FILL = '=';
char const EMPTY = ' ';

struct timeval ptime()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv;
}

void get_term_size(struct winsize* w)
{
  ioctl(STDOUT_FILENO, TIOCGWINSZ, w);
}

void init_bar(Bar* b, size_t size)
{
  b->size = size;
  b->adv = 0;
  b->delta = 0;
  b->t_delta = ptime();
  b->rate = 0.0;
  b->lastUp.tv_sec = 0;
  b->lastUp.tv_usec = 0;
  b->up = 0.6;
}

void progress_bar(float percent)
{
  struct winsize w;
  get_term_size(&w);
  int size = w.ws_col * 0.7;
  int nb = percent * size;

  printf("\e[2K\e[G");
  printf("[");
  int i;
  for (i = 0; i < nb; i++)
  {
    putchar(FILL);
  }
  if (i < size)
  {
    putchar('>');
  }

  for (; i < size - 1; i++)
  {
    putchar(EMPTY);
  }
  printf("] %.3g%%", percent * 100.0);

  fflush(stdout);
}

void download_bar(Bar* b, size_t downloaded)
{
  b->delta += downloaded - b->adv;
  struct timeval now = ptime();
  struct timeval ts;
  timersub(&now, &(b->t_delta), &ts);
  double time_spend = ts.tv_sec + ts.tv_usec / 1000000.0;
  if (b->lastUp.tv_sec == 0)
  {
    progress_bar((float)(downloaded) / (float)b->size);
    printf("ts: %g", time_spend);
    // b->lastUp = now;
  }

  if (time_spend >= b->up)
  {
    progress_bar((float)(downloaded) / (float)b->size);
    if (time_spend >= 1.0)
    {
      b->rate = b->delta / time_spend;
      b->delta = 0;
      b->t_delta = ptime();
    }

    printf(" ");
    printf_bytes(b->rate);
    printf("/s ");
    printf_second((b->size - downloaded) / b->rate);
    fflush(stdout);
    b->lastUp = ptime();
  }
  b->adv = downloaded;
}
