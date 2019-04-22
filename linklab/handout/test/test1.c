#include <stdlib.h>

int main(void)
{
  void *a, *b, *c;

  a = malloc(1024);
  a = malloc(32);
  b = malloc(23);
  c = calloc(10, 4);
  b = calloc(2,3);  
    c = realloc(b,24);
    c= realloc(c, 12);
    c = realloc(c,3);
    

  free(malloc(1));
  free(a);
  return 0;
}
