#include "kernel/types.h"
#include "kernel/param.h"
#include "kernel/memlayout.h"
#include "kernel/riscv.h"
#include "user.h"

static void start_late_fillers(int children, int pages_per_child) {
  for (int i = 0; i < children; i++) {
    int pid = fork();
    if (pid < 0) {
      break;
    }
    if (pid == 0) {
      // Delay to let parent exec and load some exec/text pages first
      pause(200);
      int bytes = pages_per_child * PGSIZE;
      char *h = sbrk(bytes);
      if (h != (char*)-1) {
        for (int off = 0; off < bytes; off += PGSIZE) h[off] = (char)i;
      }
      // Hold the memory to keep pressure high
      pause(800);
      exit(0);
    }
  }
}

int main(int argc, char **argv) {
  printf("=== t_eviction start ===\n");

  // Spawn a set of delayed memory fillers to push the system to MEMFULL
  // AFTER the parent has started, exec'd, and loaded a few clean exec pages.
  int children = 8;            // number of memory-filling children
  int pages_per_child = 256;   // 1MB per child
  start_late_fillers(children, pages_per_child);

  // Exec a larger binary so it faults many exec/data pages, and after
  // delayed fillers kick in, its subsequent exec faults should hit MEMFULL
  // and trigger per-process FIFO eviction of its own clean exec pages.
  char *args[] = { "grind", 0 };
  exec("grind", args);

  // If exec fails, report and exit.
  printf("t_eviction: exec grind failed\n");
  exit(1);
}
