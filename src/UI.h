/**
 * @brief 
 * 
 * @param percent between 0.0 and 1.0
 */
#include <stdbool.h>
#include <stddef.h>
#include <sys/time.h>
#include <time.h>

typedef struct
{
  size_t size;
  size_t adv;

  size_t         delta;
  struct timeval t_delta;
  double         rate;

  struct timeval lastUp;
  double         up;
} Bar;

void init_bar(Bar* b, size_t size);

void progress_bar(float percent);
void download_bar(Bar* b, size_t downloaded);
