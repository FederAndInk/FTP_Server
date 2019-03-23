#include "UI.h"
#include "format.h"
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>

char const FILL = '=';
char const EMPTY = ' ';

void get_term_size(struct winsize* w)
{
  ioctl(STDOUT_FILENO, TIOCGWINSZ, w);
}

void init_bar(Bar* b, size_t size)
{
  b->size = size;
  b->adv = 0;
  b->delta = 0;
  b->t_delta = clock() - 1;
  b->rate = 0.0;
  b->lastUp = 0;
  b->up = 0.4;
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

  double time_spend = (double)(clock() - b->t_delta) / (double)CLOCKS_PER_SEC;
  if (time_spend >= b->up)
  {
    progress_bar((float)(downloaded) / (float)b->size);
    if (time_spend >= 1.0)
    {
      b->rate = b->delta / time_spend;
      b->delta = 0;
      b->t_delta = clock();
    }

    printf(" ");
    printf_bytes(b->rate);
    printf("/s ");
    printf_second((b->size - downloaded) / b->rate);
    fflush(stdout);
    b->lastUp = clock();
  }
  b->adv = downloaded;
}
