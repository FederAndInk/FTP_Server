
#include "format.h"
#include <stdio.h>

void printf_bytes(size_t b)
{
  double const d2p20 = 1l << 20;
  double const d2p30 = 1l << 30;
  if (b < 1024)
  {
    printf("%zu B", b);
  }
  else if (b < d2p20)
  {
    printf("%.4g Ki", b / 1024.0);
  }
  else if (b < d2p30)
  {
    printf("%.4g Mi", b / d2p20);
  }
  else
  {
    printf("%.4g Gi", b / d2p30);
  }
}

void printf_second(size_t sc)
{
  size_t h = sc / 3600;
  size_t m = (sc % 3600) / 60;
  size_t s = (sc % 60);

  printf("%.2zu:%.2zu:%.2zu", h, m, s);
}
