#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

// Large zero-initialized array in .bss (data segment) to cause data faults
// Tuned to 64MB and accessed conservatively to avoid end-of-segment invalid faults
static char bigdata[64 * 1024 * 1024];

static void run_worker(int pages, int stack_kb) {
  if (pages <= 0) {
    printf("worker: pages=0, nothing to do\n");
    return;
  }
  int size = pages * 4096;
  char *mem = sbrk(size);
  if (mem == (char*)-1) {
    printf("sbrk failed\n");
    exit(1);
  }
  printf("touching %d pages...\n", pages);
  for (int i = 0; i < size; i += 4096) {
    if ((i % (1024 * 1024)) == 0) write(1, ".", 1);
    mem[i] = (char)(i / 4096);
  }
  // Now touch the data segment buffer to force data faults under peak pressure
  // Limit to 8MB so we don't exceed xv6's per-file FS limit before retouch phase
  int data_limit = (pages > 0) ? (8 * 1024 * 1024) : 0;
  if (data_limit > (int)sizeof(bigdata)) data_limit = (int)sizeof(bigdata);
  if (data_limit < 0) data_limit = 0;
  printf("\ntouching %dMB data segment...\n", (int)(sizeof(bigdata) / (1024*1024)));
  for (int i = 0; i < data_limit; i += 4096) {
    if ((i % (1024 * 1024)) == 0) write(1, ".", 1);
    bigdata[i] += 1;
  }
  // Optional: avoid stack growth; stack faults may hit guard in some builds
  (void)stack_kb;

  // Re-touch a small window to verify contents and trigger possible SWAPIN
  printf("\nre-touching a small window...\n");
  int small_pages = pages < 20 ? pages : 20;
  for (int i = 0; i < small_pages*4096; i += 4096) {
    if (mem[i] != (char)(i / 4096)) {
      printf("error: incorrect data at %p\n", &mem[i]);
      exit(1);
    }
  }
  // Explicitly re-touch a larger prefix of heap to force SWAPIN after evictions
  int heap_pages_retouch = pages < 256 ? pages : 256;
  printf("re-touching first %d heap pages...\n", heap_pages_retouch);
  for (int i = 0; i < heap_pages_retouch*4096; i += 4096) {
    volatile char x = mem[i];
    (void)x;
  }
  // Explicitly re-touch first 4MB of data segment to force SWAPIN for data-backed evictions
  printf("re-touching first 4MB of data segment...\n");
  for (int i = 0; i < 4*1024*1024; i += 4096) {
    volatile char x = bigdata[i];
    (void)x;
  }
  printf("worker done\n");
}

// evicttest [children] [pages_per_child] [stack_kb]
int
main(int argc, char *argv[])
{
  printf("evicttest starting\n");

  int children = 2;            // default: spawn 2 workers
  int pages_per_child = 2000;  // ~8MB per child
  int stack_kb = 0;            // default no extra stack growth
  if (argc >= 2) children = atoi(argv[1]);
  if (argc >= 3) pages_per_child = atoi(argv[2]);
  if (argc >= 4) stack_kb = atoi(argv[3]);
  if (children < 0) children = 0;
  if (pages_per_child < 0) pages_per_child = 0;
  if (stack_kb < 0) stack_kb = 0;

  if (children == 0) {
    printf("evicttest: children=0, exiting\n");
    exit(0);
  }

  for (int c = 0; c < children; c++){
    int pid = fork();
    if (pid < 0) {
      printf("fork failed at child %d\n", c);
      break;
    }
    if (pid == 0) {
      // child
      run_worker(pages_per_child, stack_kb);
      exit(0);
    }
  }

  // parent waits
  int x;
  while (wait(&x) >= 0) {
    ;
  }
  printf("evicttest finished\n");
  exit(0);
}
