#include "kernel/types.h"
#include "user.h"

// Read a large, ELF-backed global to cause LOADEXEC faults without making pages dirty.
// Under memory pressure (e.g., reduce QEMU -m or run many instances), evictions should occur
// and be DISCARD (clean) with FIFO order.

// 64 KB: 16 pages of 4096 bytes (keep binary small to avoid MAXFILE)
static const unsigned char big[64 * 1024] = {1};

int main(int argc, char **argv) {
  int pages = 16; // default equals sizeof(big)/4096
  if (argc >= 2) pages = atoi(argv[1]);
  if (pages <= 0 || pages > (int)(sizeof(big)/4096)) pages = sizeof(big)/4096;

  // Sequentially read one byte from the start of each page to fault in pages in FIFO order.
  volatile unsigned long sum = 0;
  for (int i = 0; i < pages; i++) {
    int off = i * 4096;
    sum += big[off];
  }

  // Optional: create children to increase memory pressure.
  if (argc >= 3) {
    int kids = atoi(argv[2]);
    for (int k = 0; k < kids; k++) {
      int pid = fork();
      if (pid == 0) {
        volatile unsigned long s2 = 0;
        for (int i = 0; i < pages; i++) {
          int off = i * 4096;
          s2 += big[off];
        }
        printf("evictfifo child %d done sum=%lu\n", getpid(), s2);
        exit(0);
      }
    }
    for (int k = 0; k < kids; k++) wait(0);
  }

  printf("evictfifo done sum=%lu pages=%d\n", sum, pages);
  exit(0);
}
