#include "utils.h"

/**
 * @brief cut str to the first occurence of c (write a '\0' over c)
 * 
 * @param str 
 * @param c 
 * @return char* the second part after first c
 */
char* cut_first(char* str, char c)
{
  char* tmp = str;
  while (*tmp != '\0' && *tmp != c)
  {
    tmp++;
  }

  if (*tmp == c)
  {
    *tmp = '\0';
    return tmp + 1;
  }
  else
  {
    return str;
  }
}
