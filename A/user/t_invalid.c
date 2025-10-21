#include "kernel/types.h"
#include "kernel/param.h"
#include "kernel/memlayout.h"
#include "kernel/riscv.h"
#include "user.h"

static void do_invalid_below_stack(void) {
  // read one byte below the guard page relative to current sp
  char *sp = (char*) r_sp();
  // emulate usertests style: access below stack guard to trigger invalid
  volatile char x = *(sp - (USERSTACK*PGSIZE) - 1);
  (void)x;
}

static void do_write_to_exec(void) {
  // Map a code address (main's page) as exec-only, writing should fault.
  // We simulate by writing through a function pointer's address.
  // This will either segfault or trigger our kernel's invalid-access path.
  void (*fn)(void) = (void(*)(void))do_write_to_exec; // any text addr
  volatile char *pc = (volatile char*)((uint64)fn & ~4095ULL); // page base
  pc[0] = pc[0]; // read ok (exec treated as exec/read in our resolver),
  pc[1] = 0x7f;  // attempt write -> should trigger KILL invalid-access
}

int main(void) {
  printf("=== t_invalid start ===\n");
  int pid = fork();
  if (pid == 0) {
    do_invalid_below_stack();
    printf("t_invalid: below-stack did not fault as expected\n");
    exit(1);
  } else if (pid > 0) {
    wait(0);
  }

  pid = fork();
  if (pid == 0) {
    do_write_to_exec();
    printf("t_invalid: write-to-exec did not fault as expected\n");
    exit(1);
  } else if (pid > 0) {
    wait(0);
  }

  printf("=== t_invalid done ===\n");
  exit(0);
}
