#include "kernel/types.h"
#include "user.h"

int x = 7;

static int f(int a) {
  return a + x;
}

int main(int argc, char **argv) {
  int y = f(5);
  printf("lazyexec y=%d\n", y);
  exit(0);
}
