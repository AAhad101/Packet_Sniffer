#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

static void touch_pages(char *base, int pages) {
  for (int i = 0; i < pages; i++) {
    volatile char *p = base + i * 4096;
    *p = (char)i; // write to dirty the page
  }
}

int
main(int argc, char *argv[])
{
  if (argc < 2) {
    fprintf(2, "usage: swaptwopass <pages>\n");
    exit(1);
  }
  int pages = atoi(argv[1]);
  if (pages <= 0)
    pages = 1;

  // allocate pages
  char *base = sbrk(pages * 4096);
  if (base == (char*)-1) {
    fprintf(2, "swaptwopass: sbrk failed\n");
    exit(1);
  }

  // pass 1: touch sequentially to cause allocations and (under pressure) SWAPOUT
  touch_pages(base, pages);

  // small pause to encourage eviction by others if needed
  pause(1);

  // pass 2: touch again to trigger SWAPIN on previously swapped pages
  touch_pages(base, pages);

  printf("swaptwopass: done %d pages\n", pages);
  exit(0);
}
