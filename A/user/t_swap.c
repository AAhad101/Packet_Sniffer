#include "kernel/types.h"
#include "kernel/param.h"
#include "kernel/memlayout.h"
#include "kernel/riscv.h"
#include "user.h"

static void touch_exec_pages(int n) {
  volatile int sink = 0;
  for (int i = 0; i < n; i++) {
    uint64 va = (uint64)(i * PGSIZE);
    volatile char *p = (volatile char *)va;
    sink ^= *p;
  }
  if (sink == 123) printf("%d\n", sink);
}

int main(void) {
  printf("=== t_swap start ===\n");

  // Step 1: Create a bunch of dirty heap pages in the parent so they are FIFO-oldest.
  int heap_pages = 512; // 2MB
  char *h = sbrk(heap_pages * PGSIZE);
  if (h != (char*)-1) {
    for (int j = 0; j < heap_pages * PGSIZE; j += PGSIZE) h[j] = (char)(j >> 12);
  }

  // Step 2: Spawn a child that fills memory and holds it, pushing parent into MEMFULL.
  int pid = fork();
  if (pid == 0) {
    // Child
    int pages = 768; // 3MB
    char *c = sbrk(pages * PGSIZE);
    if (c != (char*)-1) {
      for (int j = 0; j < pages * PGSIZE; j += PGSIZE) c[j] = (char)(j >> 12);
    }
    pause(800);
    exit(0);
  }

  // Give child a head start so parent's next faults happen under pressure.
  pause(200);

  // Step 3: Parent triggers additional exec page faults, forcing eviction of its oldest pages.
  // Expect: PAGEFAULT -> MEMFULL -> VICTIM -> EVICT state=dirty -> SWAPOUT slot=N -> LOADEXEC -> RESIDENT
  touch_exec_pages(4);

  // Step 4: Access an old heap page to force SWAPIN for that VA.
  if (h != (char*)-1) {
    volatile char x = h[0];
    (void)x;
  }

  wait(0);
  printf("=== t_swap done ===\n");
  exit(0);
}
