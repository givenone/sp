#include <stdlib.h>


int main(void)
{
  void *a;
  a = realloc(a, 10000);
  a = realloc(a, 100000);
  
  a = malloc(10);
  a = realloc(a, 100);
  a = realloc(a, 1000);
  free(a);


  return 0;
}
