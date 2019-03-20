#include "UI.h"
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>

char const FILL = '=';
char const EMPTY = ' ';

void get_term_size(struct winsize* w)
{
  ioctl(STDOUT_FILENO, TIOCGWINSZ, w);
}

void progress_bar(float percent)
{
  static int prev_nb = 0;

  struct winsize w;
  get_term_size(&w);
  int size = w.ws_col * 0.75;
  int nb = percent * size;
  if (prev_nb == 0 || prev_nb != nb)
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
    printf("]");
    fflush(stdout);
  }
  prev_nb = nb;
}
