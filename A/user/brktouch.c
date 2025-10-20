#include "kernel/types.h"
#include "user.h"

// Usage: brktouch N [notouch]
// N = number of pages to sbrk. If "notouch" is given, do not touch pages.
int main(int argc, char **argv) {
  if (argc < 2) {
    printf("usage: brktouch N [notouch]\n");
    exit(1);
  }
  int n = atoi(argv[1]);
  int touch = 1;
  if (argc >= 3 && strcmp(argv[2], "notouch") == 0) touch = 0;

  int bytes = n * 4096;
  char *base = sbrk(bytes);
  if (base == (char*)-1) {
    printf("brktouch: sbrk failed\n");
    exit(1);
  }
  printf("brktouch: grew by %d bytes at %p\n", bytes, base);

  if (touch) {
    for (int i = 0; i < n; i++) {
      volatile char *p = (volatile char*) (base + i * 4096);
      char v = *p; // read fault (allowed to be zero)
      (void)v;
      *p = (char)i; // write fault -> marks dirty
    }
    printf("brktouch: touched %d pages\n", n);
  } else {
    printf("brktouch: not touching newly mapped pages\n");
  }
  exit(0);
}
