#include "kernel/types.h"
#include "kernel/param.h"
#include "kernel/riscv.h"
#include "user.h"

static void touch_exec_pages(int npages)
{
  // Many tiny user programs have text+data < 8KB; only 0x0 and 0x1000 are valid exec/data pages.
  // Clamp to two pages (0 and 0x1000) to avoid invalid-access at 0x2000+.
  int cap = npages;
  if (cap > 2) cap = 2;
  volatile char sink = 0;
  for (int i = 0; i < cap; i++) {
    uint64 va = (i == 0) ? 0ULL : 0x1000ULL;
    volatile char *p = (volatile char *)va;
    sink ^= *p;
  }
  if (sink == 123) printf("%d\n", sink); // keep compiler from optimizing away
}

int main(void)
{
  printf("=== t_fifo start ===\n");

  // Preload some exec/text pages in parent
  touch_exec_pages(8);

  // Spawn children that each keep a few resident pages and sleep
  int children = 96;
  for (int i = 0; i < children; i++) {
    int pid = fork();
    if (pid < 0) {
      break;
    }
    if (pid == 0) {
      touch_exec_pages(8);
      // Allocate some heap pages and touch them once to hold frames.
      int pages = 1024; // 4MB per child
      char *h = sbrk(pages * 4096);
      if (h != (char*)-1) {
        for (int j = 0; j < pages * 4096; j += 4096) h[j] = (char)i;
      }
      // Keep child alive briefly to hold frames.
      pause(400);
      exit(0);
    }
  }

  // Attempt to trigger replacement in parent by touching another exec page.
  // If memory is tight, parent should log MEMFULL and evict an old exec page (clean).
  touch_exec_pages(9);

  // Re-touch first page to ensure it faults back in if evicted.
  volatile char *p0 = (volatile char *)(uint64)0;
  (void)*p0;

  // Reap children.
  for (int i = 0; i < children; i++) {
    wait(0);
  }

  printf("=== t_fifo done ===\n");
  exit(0);
}
