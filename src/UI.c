#include "UI.h"
#include <stdio.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

char const FILL = '=';
char const EMPTY = ' ';

void get_term_size(struct winsize* w)
{
  ioctl(STDOUT_FILENO, TIOCGWINSZ, w);
}

bool progress_bar(float percent)
{
  static clock_t prev_time = 0;
  static int     prev_nb = 0;
  if (prev_time == 0)
  {
    prev_time = clock();
  }
  struct winsize w;
  get_term_size(&w);
  int size = w.ws_col * 0.7;
  int nb = percent * size;

  double passed = (clock() - prev_time) / (double)CLOCKS_PER_SEC;
  if (passed >= 0.2 || percent >= 0.95) // || prev_nb == 0 || prev_nb != nb
  {
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
    prev_time = clock();
    prev_nb = nb;
    return true;
  }
  prev_nb = nb;
  return false;
}
