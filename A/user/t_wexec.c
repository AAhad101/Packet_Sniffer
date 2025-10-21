#include "kernel/types.h"
#include "kernel/memlayout.h"
#include "kernel/riscv.h"
#include "user.h"

int main(void) {
  printf("=== t_wexec start ===\n");
  // Attempt to write into the first text page (0x0). This should trigger:
  // PAGEFAULT access=write cause=exec -> KILL invalid-access
  volatile char *pc = (volatile char*)0x0;
  pc[0] = 0x7f; // should fault and kill this process
  // If we get here, it didn't fault.
  printf("t_wexec: write-to-exec did not fault as expected\n");
  exit(1);
}
