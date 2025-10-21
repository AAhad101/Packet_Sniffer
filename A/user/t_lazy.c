#include "kernel/types.h"
#include "user.h"

int main(void) {
  printf("=== t_lazy start ===\n");
  int n = 3 * 4096;
  char *p = sbrk(n);
  if (p == (char*)-1) {
    printf("t_lazy: sbrk failed\n");
    exit(1);
  }
  for (int i = 0; i < n; i += 4096) {
    p[i] = (char)(i / 4096);
  }
  for (int i = 0; i < n; i += 4096) {
    if (p[i] != (char)(i / 4096)) {
      printf("t_lazy: mismatch at %d\n", i);
      exit(1);
    }
  }
  printf("=== t_lazy done ===\n");
  exit(0);
}
