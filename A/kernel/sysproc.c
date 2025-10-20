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

static int is_va_swapped(struct proc *p, uint64 va, int *slot_out){
  for(int i=0;i<1024;i++){
    if(p->swap_slot_used[i] && p->swap_slot_va[i]==va){ if(slot_out) *slot_out=i; return 1; }
  }
  return 0;
}

uint64
sys_memstat(void)
{
  uint64 uaddr;
  if(argaddr(0, &uaddr), 0){}
  // Fetch user pointer
  argaddr(0, &uaddr);
  struct proc *p = myproc();
  struct proc_mem_stat kinfo = {0};
  kinfo.pid = p->pid;
  kinfo.next_fifo_seq = p->next_fifo_seq;

  // Determine reporting range: from text start (text_lo) to p->sz
  uint64 start = p->text_lo;
  uint64 end = p->sz;
  int idx = 0;
  for(uint64 va = PGROUNDDOWN(start); va < end; va += PGSIZE){
    int state = UNMAPPED;
    int is_dirty = 0;
    int seq = 0;
    int slot = -1;
    uint64 pa = walkaddr(p->pagetable, va);
    if(pa){
      state = RESIDENT;
      // lookup in FIFO to extract seq/dirty
      int n = p->res_count; int i = p->res_head;
      for(int k=0;k<n;k++){
        struct respage *e = &p->res_pages[i];
        if(e->va == va){ seq = e->seq; is_dirty = e->dirty; break; }
        i = (i+1) % MAX_RES_PAGES;
      }
      kinfo.num_resident_pages++;
    } else if(is_va_swapped(p, va, &slot)){
      state = SWAPPED;
      kinfo.num_swapped_pages++;
    } else {
      state = UNMAPPED;
    }
    kinfo.num_pages_total++;
    if(idx < MAX_PAGES_INFO){
      kinfo.pages[idx].va = (uint)va;
      kinfo.pages[idx].state = state;
      kinfo.pages[idx].is_dirty = is_dirty;
      kinfo.pages[idx].seq = seq;
      kinfo.pages[idx].swap_slot = slot;
      idx++;
    }
  }

  if(either_copyout(1, uaddr, &kinfo, sizeof(kinfo)) < 0)
    return -1;
  return 0;
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
