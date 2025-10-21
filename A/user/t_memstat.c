#include "kernel/types.h"
#include "kernel/riscv.h"
#include "kernel/memlayout.h"
#include "kernel/memstat.h"
#include "user.h"

static void print_stat(const char *title) {
  struct proc_mem_stat ps;
  if (memstat(&ps) < 0) {
    printf("memstat failed\n");
    return;
  }
  printf("%s: num=%d\n", title, ps.num);
  for (int i = 0; i < ps.num; i++) {
    struct page_info *pi = &ps.info[i];
    const char *st = (pi->state == P_RESIDENT) ? "RES" : (pi->state == P_SWAPPED) ? "SWP" : "UNM";
    printf("  va=%p state=%s dirty=%d seq=%d slot=%d\n", (void*)pi->va, st, pi->is_dirty, pi->seq, pi->swap_slot);
  }
}

int main(void) {
  printf("=== t_memstat start ===\n");
  print_stat("initial");

  // Create some heap pages
  int n = 64;
  char *h = sbrk(n * PGSIZE);
  for (int i = 0; i < n * PGSIZE; i += PGSIZE) h[i] = (char)i;

  print_stat("after-heap");

  // Touch one exec page to bring it in
  volatile char *p0 = (volatile char*)0x0;
  (void)*p0;

  print_stat("after-exec");

  printf("=== t_memstat done ===\n");
  exit(0);
}
