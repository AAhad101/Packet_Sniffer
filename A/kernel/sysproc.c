#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "vm.h"
#include "memstat.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  kexit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return kfork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return kwait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr = myproc()->sz;
  int n;

  argint(0, &n);

  if (n < 0) {
    if (growproc(n) < 0)
      return -1;
  } else if (n > 0) {
    if (addr + n < addr)
      return -1;
    myproc()->sz += n;   // lazy grow: no allocation
  }
  return addr;
}

uint64
sys_pause(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  if(n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kkill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

static int fifo_seq_for_va(struct proc *p, uint64 va)
{
  for (int k = 0; k < p->res_count; k++) {
    int idx = (p->res_head + k) % MAX_RES_PAGES;
    if (p->res_pages[idx].va == va)
      return p->res_pages[idx].seq;
  }
  return -1;
}

uint64
sys_memstat(void)
{
  struct proc *p = myproc();
  uint64 uptr;
  argaddr(0, &uptr);

  struct proc_mem_stat k = {0};

  // Enumerate resident pages by scanning mapped user PTEs up to
  // the max of sz and stack_top.
  uint64 limit = p->sz;
  if (p->stack_top > limit) limit = p->stack_top;

  for (uint64 va = 0; va < limit && k.num < MAX_PAGES_INFO; va += PGSIZE) {
    pte_t *pte = walk(p->pagetable, va, 0);
    if (pte && (*pte & PTE_V) && (*pte & PTE_U)) {
      struct page_info *pi = &k.info[k.num++];
      pi->va = va;
      pi->state = P_RESIDENT;
      pi->seq = fifo_seq_for_va(p, va);
      // Dirty heuristic: heap/stack or writable => dirty; exec RO => clean
      const char *c = classify_fault_cause(p, va);
      if (c && (strncmp(c, "heap", 4) == 0 || strncmp(c, "stack", 5) == 0))
        pi->is_dirty = 1;
      else
        pi->is_dirty = ((*pte & PTE_W) != 0);
      pi->swap_slot = -1;
    }
  }

  // Enumerate swapped pages from per-proc swap table
  for (int s = 0; s < MAX_SWAP_SLOTS && k.num < MAX_PAGES_INFO; s++) {
    if (p->swap_pages[s].valid) {
      struct page_info *pi = &k.info[k.num++];
      pi->va = p->swap_pages[s].va;
      pi->state = P_SWAPPED;
      pi->is_dirty = 1; // only dirty/non-backed pages are swapped out
      pi->seq = -1;
      pi->swap_slot = s;
    }
  }

  if (copyout(p->pagetable, uptr, (char*)&k, sizeof(k)) < 0)
    return -1;
  return 0;
}
