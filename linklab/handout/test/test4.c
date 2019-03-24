#include <stdlib.h>

int main(void)
{
  void *a;

  a = malloc(1024);
  free(a);
  a = realloc(a, 10);
  free(a);
  free(a);
  free((void*)0x1706e90);

  return 0;
}
