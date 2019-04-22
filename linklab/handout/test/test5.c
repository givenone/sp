#include <stdlib.h>


int main(void)
{
  void *a, *b, *c;

  
  a = malloc(10);
  b = realloc(a, 100);
  c = realloc(a, 1000);
  c = calloc(10, 4);
  a = realloc(a, 10000);
  a = realloc(a, 100000);
  free(a);


  return 0;
}
