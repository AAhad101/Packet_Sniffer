#include "param.h"
#include "types.h"
#include "memlayout.h"
#include "elf.h"
#include "riscv.h"
#include "defs.h"
#include "spinlock.h"
#include "proc.h"
#include "fs.h"

// Set to 1 for detailed page fault logging; 0 to reduce verbosity
#ifndef VERBOSE_PAGING
#define VERBOSE_PAGING 0 
#endif

/*
 * the kernel's page table.
 */
pagetable_t kernel_pagetable;

extern char etext[];  // kernel.ld sets this to end of kernel code.

extern char trampoline[]; // trampoline.S

// Make a direct-map page table for the kernel.
pagetable_t
kvmmake(void)
{
  pagetable_t kpgtbl;

  kpgtbl = (pagetable_t) kalloc();
  memset(kpgtbl, 0, PGSIZE);

  // uart registers
  kvmmap(kpgtbl, UART0, UART0, PGSIZE, PTE_R | PTE_W);

  // virtio mmio disk interface
  kvmmap(kpgtbl, VIRTIO0, VIRTIO0, PGSIZE, PTE_R | PTE_W);

  // PLIC
  kvmmap(kpgtbl, PLIC, PLIC, 0x4000000, PTE_R | PTE_W);

  // map kernel text executable and read-only.
  kvmmap(kpgtbl, KERNBASE, KERNBASE, (uint64)etext-KERNBASE, PTE_R | PTE_X);

  // map kernel data and the physical RAM we'll make use of.
  kvmmap(kpgtbl, (uint64)etext, (uint64)etext, PHYSTOP-(uint64)etext, PTE_R | PTE_W);

  // map the trampoline for trap entry/exit to
  // the highest virtual address in the kernel.
  kvmmap(kpgtbl, TRAMPOLINE, (uint64)trampoline, PGSIZE, PTE_R | PTE_X);

  // allocate and map a kernel stack for each process.
  proc_mapstacks(kpgtbl);
  
  return kpgtbl;
}

// add a mapping to the kernel page table.
// only used when booting.
// does not flush TLB or enable paging.
void
kvmmap(pagetable_t kpgtbl, uint64 va, uint64 pa, uint64 sz, int perm)
{
  if(mappages(kpgtbl, va, sz, pa, perm) != 0)
    panic("kvmmap");
}

// Initialize the kernel_pagetable, shared by all CPUs.
void
kvminit(void)
{
  kernel_pagetable = kvmmake();
}

// Switch the current CPU's h/w page table register to
// the kernel's page table, and enable paging.
void
kvminithart()
{
  // wait for any previous writes to the page table memory to finish.
  sfence_vma();

  w_satp(MAKE_SATP(kernel_pagetable));

  // flush stale entries from the TLB.
  sfence_vma();
}

// Return the address of the PTE in page table pagetable
// that corresponds to virtual address va.  If alloc!=0,
// create any required page-table pages.
//
// The risc-v Sv39 scheme has three levels of page-table
// pages. A page-table page contains 512 64-bit PTEs.
// A 64-bit virtual address is split into five fields:
//   39..63 -- must be zero.
//   30..38 -- 9 bits of level-2 index.
//   21..29 -- 9 bits of level-1 index.
//   12..20 -- 9 bits of level-0 index.
//    0..11 -- 12 bits of byte offset within the page.
pte_t *
walk(pagetable_t pagetable, uint64 va, int alloc)
{
  if(va >= MAXVA)
    panic("walk");

  for(int level = 2; level > 0; level--) {
    pte_t *pte = &pagetable[PX(level, va)];
    if(*pte & PTE_V) {
      pagetable = (pagetable_t)PTE2PA(*pte);
    } else {
      if(!alloc || (pagetable = (pde_t*)kalloc()) == 0)
        return 0;
      memset(pagetable, 0, PGSIZE);
      *pte = PA2PTE(pagetable) | PTE_V;
    }
  }
  return &pagetable[PX(0, va)];
}

// Look up a virtual address, return the physical address,
// or 0 if not mapped.
// Can only be used to look up user pages.
uint64
walkaddr(pagetable_t pagetable, uint64 va)
{
  pte_t *pte;
  uint64 pa;

  if(va >= MAXVA)
    return 0;

  pte = walk(pagetable, va, 0);
  if(pte == 0)
    return 0;
  if((*pte & PTE_V) == 0)
    return 0;
  if((*pte & PTE_U) == 0)
    return 0;
  pa = PTE2PA(*pte);
  return pa;
}

// Create PTEs for virtual addresses starting at va that refer to
// physical addresses starting at pa.
// va and size MUST be page-aligned.
// Returns 0 on success, -1 if walk() couldn't
// allocate a needed page-table page.
int
mappages(pagetable_t pagetable, uint64 va, uint64 size, uint64 pa, int perm)
{
  uint64 a, last;
  pte_t *pte;

  if((va % PGSIZE) != 0)
    panic("mappages: va not aligned");

  if((size % PGSIZE) != 0)
    panic("mappages: size not aligned");

  if(size == 0)
    panic("mappages: size");
  
  a = va;
  last = va + size - PGSIZE;
  for(;;){
    if((pte = walk(pagetable, a, 1)) == 0)
      return -1;
    if(*pte & PTE_V)
      panic("mappages: remap");
    *pte = PA2PTE(pa) | perm | PTE_V;
    if(a == last)
      break;
    a += PGSIZE;
    pa += PGSIZE;
  }
  return 0;
}

// create an empty user page table.
// returns 0 if out of memory.
pagetable_t
uvmcreate()
{
  pagetable_t pagetable;
  pagetable = (pagetable_t) kalloc();
  if(pagetable == 0)
    return 0;
  memset(pagetable, 0, PGSIZE);
  return pagetable;
}

// Remove npages of mappings starting from va. va must be
// page-aligned. It's OK if the mappings don't exist.
// Optionally free the physical memory.
void
uvmunmap(pagetable_t pagetable, uint64 va, uint64 npages, int do_free)
{
  uint64 a;
  pte_t *pte;

  if((va % PGSIZE) != 0)
    panic("uvmunmap: not aligned");

  for(a = va; a < va + npages*PGSIZE; a += PGSIZE){
    if((pte = walk(pagetable, a, 0)) == 0) // leaf page table entry allocated?
      continue;   
    if((*pte & PTE_V) == 0)  // has physical page been allocated?
      continue;
    if(do_free){
      uint64 pa = PTE2PA(*pte);
      kfree((void*)pa);
    }
    *pte = 0;
  }
}

// Allocate PTEs and physical memory to grow a process from oldsz to
// newsz, which need not be page aligned.  Returns new size or 0 on error.
uint64
uvmalloc(pagetable_t pagetable, uint64 oldsz, uint64 newsz, int xperm)
{
  char *mem;
  uint64 a;

  if(newsz < oldsz)
    return oldsz;

  oldsz = PGROUNDUP(oldsz);
  for(a = oldsz; a < newsz; a += PGSIZE){
    mem = kalloc();
    if(mem == 0){
      uvmdealloc(pagetable, a, oldsz);
      return 0;
    }
    memset(mem, 0, PGSIZE);
    if(mappages(pagetable, a, PGSIZE, (uint64)mem, PTE_R|PTE_U|xperm) != 0){
      kfree(mem);
      uvmdealloc(pagetable, a, oldsz);
      return 0;
    }
  }
  return newsz;
}

// Deallocate user pages to bring the process size from oldsz to
// newsz.  oldsz and newsz need not be page-aligned, nor does newsz
// need to be less than oldsz.  oldsz can be larger than the actual
// process size.  Returns the new process size.
uint64
uvmdealloc(pagetable_t pagetable, uint64 oldsz, uint64 newsz)
{
  if(newsz >= oldsz)
    return oldsz;

  if(PGROUNDUP(newsz) < PGROUNDUP(oldsz)){
    int npages = (PGROUNDUP(oldsz) - PGROUNDUP(newsz)) / PGSIZE;
    uvmunmap(pagetable, PGROUNDUP(newsz), npages, 1);
  }

  return newsz;
}

// Recursively free page-table pages.
// All leaf mappings must already have been removed.
void
freewalk(pagetable_t pagetable)
{
  // there are 2^9 = 512 PTEs in a page table.
  for(int i = 0; i < 512; i++){
    pte_t pte = pagetable[i];
    if((pte & PTE_V) && (pte & (PTE_R|PTE_W|PTE_X)) == 0){
      // this PTE points to a lower-level page table.
      uint64 child = PTE2PA(pte);
      freewalk((pagetable_t)child);
      pagetable[i] = 0;
    } else if(pte & PTE_V){
      // Leaf mapping left over; free it defensively.
      uint64 pa = PTE2PA(pte);
      kfree((void*)pa);
      pagetable[i] = 0;
    }
  }
  kfree((void*)pagetable);
}

// Free user memory pages,
// then free page-table pages.
void
uvmfree(pagetable_t pagetable, uint64 sz)
{
  if(sz > 0)
    uvmunmap(pagetable, 0, PGROUNDUP(sz)/PGSIZE, 1);
  freewalk(pagetable);
}

// Given a parent process's page table, copy
// its memory into a child's page table.
// Copies both the page table and the
// physical memory.
// returns 0 on success, -1 on failure.
// frees any allocated pages on failure.
int
uvmcopy(pagetable_t old, pagetable_t new, uint64 sz)
{
  pte_t *pte;
  uint64 pa, i;
  uint flags;
  char *mem;

  for(i = 0; i < sz; i += PGSIZE){
    if((pte = walk(old, i, 0)) == 0)
      continue;   // page table entry hasn't been allocated
    if((*pte & PTE_V) == 0)
      continue;   // physical page hasn't been allocated
    pa = PTE2PA(*pte);
    flags = PTE_FLAGS(*pte);
    if((mem = kalloc()) == 0)
      goto err;
    memmove(mem, (char*)pa, PGSIZE);
    if(mappages(new, i, PGSIZE, (uint64)mem, flags) != 0){
      kfree(mem);
      goto err;
    }
  }
  return 0;

 err:
  uvmunmap(new, 0, i / PGSIZE, 1);
  return -1;
}

// mark a PTE invalid for user access.
// used by exec for the user stack guard page.
void
uvmclear(pagetable_t pagetable, uint64 va)
{
  pte_t *pte;
  
  pte = walk(pagetable, va, 0);
  if(pte == 0)
    panic("uvmclear");
  *pte &= ~PTE_U;
}

// Copy from kernel to user.
// Copy len bytes from src to virtual address dstva in a given page table.
// Return 0 on success, -1 on error.
int
copyout(pagetable_t pagetable, uint64 dstva, char *src, uint64 len)
{
  uint64 n, va0, pa0;
  pte_t *pte;

  while(len > 0){
    va0 = PGROUNDDOWN(dstva);
    if(va0 >= MAXVA)
      return -1;
  
    pa0 = walkaddr(pagetable, va0);
    if(pa0 == 0){
      // Only resolve if the VA is within a valid segment; otherwise, report error.
      if (classify_fault_cause(myproc(), va0) == 0)
        return -1;
      if((pa0 = demand_resolve(myproc(), pagetable, va0, "write")) == 0)
        return -1;
    }

    pte = walk(pagetable, va0, 0);
    // forbid copyout over read-only user text pages.
    if((*pte & PTE_W) == 0)
      return -1;
      
    n = PGSIZE - (dstva - va0);
    if(n > len)
      n = len;
    memmove((void *)(pa0 + (dstva - va0)), src, n);

    len -= n;
    src += n;
    dstva = va0 + PGSIZE;
  }
  return 0;
}

// Copy from user to kernel.
// Copy len bytes to dst from virtual address srcva in a given page table.
// Return 0 on success, -1 on error.
int
copyin(pagetable_t pagetable, char *dst, uint64 srcva, uint64 len)
{
  uint64 n, va0, pa0;

  while(len > 0){
    va0 = PGROUNDDOWN(srcva);
    pa0 = walkaddr(pagetable, va0);
    if(pa0 == 0) {
      // Only resolve if the VA is within a valid segment; otherwise, report error.
      if (classify_fault_cause(myproc(), va0) == 0)
        return -1;
      if((pa0 = demand_resolve(myproc(), pagetable, va0, "read")) == 0)
        return -1;
    }

    n = PGSIZE - (srcva - va0);
    if(n > len)
      n = len;
    memmove(dst, (void *)(pa0 + (srcva - va0)), n);

    len -= n;
    dst += n;
    srcva = va0 + PGSIZE;
  }
  return 0;
}

// Copy a null-terminated string from user to kernel.
// Copy bytes to dst from virtual address srcva in a given page table,
// until a '\0', or max.
// Return 0 on success, -1 on error.
int
copyinstr(pagetable_t pagetable, char *dst, uint64 srcva, uint64 max)
{
  uint64 n, va0, pa0;
  int got_null = 0;

  while(got_null == 0 && max > 0){
    va0 = PGROUNDDOWN(srcva);

    pa0 = walkaddr(pagetable, va0);
    if(pa0 == 0) {
      // Only resolve if the VA is within a valid segment; otherwise, report error.
      if (classify_fault_cause(myproc(), va0) == 0)
        return -1;
      if ((pa0 = demand_resolve(myproc(), pagetable, va0, "read")) == 0)
        return -1;
    }

    n = PGSIZE - (srcva - va0);
    if(n > max)
      n = max;

    char *p = (char *) (pa0 + (srcva - va0));
    while(n > 0){
      if(*p == '\0'){
        *dst = '\0';
        got_null = 1;
        break;
      } else {
        *dst = *p;
      }
      --n;
      --max;
      p++;
      dst++;
    }

    srcva = va0 + PGSIZE;
  }
  if(got_null){
    return 0;
  } else {
    return -1;
  }
}

// allocate and map user memory if process is referencing a page
// that was lazily allocated in sys_sbrk().
// returns 0 if va is invalid or already mapped, or if
// out of physical memory, and physical address if successful.
uint64
vmfault(pagetable_t pagetable, uint64 va, int read)
{
  uint64 mem;
  struct proc *p = myproc();

  if (va >= p->sz)
    return 0;
  va = PGROUNDDOWN(va);
  if(ismapped(pagetable, va)) {
    return 0;
  }
  mem = (uint64) kalloc();
  if(mem == 0)
    return 0;
  memset((void *) mem, 0, PGSIZE);
  if (mappages(p->pagetable, va, PGSIZE, mem, PTE_W|PTE_U|PTE_R) != 0) {
    kfree((void *)mem);
    return 0;
  }
  return mem;
}

int
ismapped(pagetable_t pagetable, uint64 va)
{
  pte_t *pte = walk(pagetable, va, 0);
  if (pte == 0) {
    return 0;
  }
  if (*pte & PTE_V){
    return 1;
  }
  return 0;
}

// Classify a user VA into "exec", "heap", or "stack".
// Returns a const string or 0 if invalid.
const char*
classify_fault_cause(struct proc *p, uint64 va)
{
  va = PGROUNDDOWN(va);
  // Exec/data segment?
  for(int i = 0; i < p->vmaps_len; i++){
    uint64 lo = p->vmaps[i].vaddr;
    uint64 hi = lo + p->vmaps[i].memsz;
    if(va >= lo && va < hi){
      if(p->vmaps[i].perms & PTE_X) return "exec";
      else return "data"; // file-backed, writable only if segment has PTE_W
    }
  }
  
  // Stack: check first so it doesn't get swallowed by heap range.
  if (p->stack_top != 0 || p->sz != 0) {
    uint64 top = p->stack_top ? p->stack_top : p->sz;
    uint64 stack_lo = top - USERSTACK*PGSIZE;
    // Guard page immediately below the stack must be invalid
    uint64 guard_lo = stack_lo - PGSIZE;
    if (va >= guard_lo && va < stack_lo)
      return 0;
    if (va >= stack_lo && va < top)
      return "stack";
  }

  // Heap?
  if (va >= p->heap_start && va < p->sz)
    return "heap";


  return 0;
}

// Return 1 if va lies in an exec/data segment; fill *out if not null
int
is_exec_vaddr(struct proc *p, uint64 va, struct vmap *out)
{
  va = PGROUNDDOWN(va);
  for(int i = 0; i < p->vmaps_len; i++){
    uint64 lo = p->vmaps[i].vaddr;
    uint64 hi = lo + p->vmaps[i].memsz;
    if(va >= lo && va < hi){
      if(out) *out = p->vmaps[i];
      return 1;
    }
  }
  return 0;
}

// Map classify string to kind code used by FIFO: 0=exec,1=heap,2=stack
static inline int
page_kind_for_va(struct proc *p, uint64 va)
{
  const char *c = classify_fault_cause(p, va);
  if (!c) return -1;
  if (strncmp(c, "exec", 4) == 0) return 0;
  if (strncmp(c, "heap", 4) == 0) return 1;
  if (strncmp(c, "stack", 5) == 0) return 2;
  return -1;
}

void
fifo_enqueue(struct proc *p, uint64 va, int kind, int seq)
{
  if (p->res_count >= MAX_RES_PAGES)
    return;
  int idx = p->res_tail;
  p->res_pages[idx].va = va;
  p->res_pages[idx].seq = seq;
  p->res_pages[idx].kind = kind;
  p->res_tail = (p->res_tail + 1) % MAX_RES_PAGES;
  p->res_count++;
}

int
fifo_remove_va(struct proc *p, uint64 va)
{
  int n = p->res_count;
  int i = p->res_head;
  for (int k = 0; k < n; k++) {
    int idx = (p->res_head + k) % MAX_RES_PAGES;
    if (p->res_pages[idx].va == va) {
      // shift following elements one step towards head
      int j = idx;
      for (int m = 0; m < n - k - 1; m++) {
        int nj = (j + 1) % MAX_RES_PAGES;
        p->res_pages[j] = p->res_pages[nj];
        j = nj;
      }
      p->res_tail = (p->res_tail + MAX_RES_PAGES - 1) % MAX_RES_PAGES;
      p->res_count--;
      return 1;
    }
    i = (i + 1) % MAX_RES_PAGES;
  }
  return 0;
}

int
try_evict_one(struct proc *p)
{
  if (p->res_count == 0)
    return 0;

  // Pass 1: prefer heap pages (kind==1) to reduce exec thrash.
  // Pass 2: consider any non-stack page (kind!=2).
  // Pass 3: consider any page.
  for (int pass = 0; pass < 3; pass++) {
    int n = p->res_count;
    int i = p->res_head;
    while (n-- > 0) {
      struct respage *e = &p->res_pages[i];
      uint64 va = e->va;
      int k = e->kind;

      if ((pass == 0 && k != 1) || (pass == 1 && k == 2)) {
        i = (i + 1) % MAX_RES_PAGES;
        continue;
      }

      // Check mapping still present and user-accessible
      pte_t *pte = walk(p->pagetable, va, 0);
      if (pte && (*pte & PTE_V) && (*pte & PTE_U)) {
        uint64 pa = PTE2PA(*pte);

        if (VERBOSE_PAGING)
          printf("[pid %d] VICTIM va=%p seq=%d\n", p->pid, (void*)va, e->seq);

        *pte = 0;
        sfence_vma();
        kfree((void*)pa);

        if (VERBOSE_PAGING) {
          printf("[pid %d] EVICT va=%p\n", p->pid, (void*)va);
          printf("[pid %d] DISCARD va=%p\n", p->pid, (void*)va);
        }

        // Properly remove this VA from the FIFO to maintain order
        if (!fifo_remove_va(p, va)) {
          // Fallback: if not found (shouldn't happen), advance head safely
          p->res_head = (p->res_head + 1) % MAX_RES_PAGES;
          if (p->res_count > 0)
            p->res_count--;
        }
        return 1;
      }
      // advance to next FIFO element
      i = (i + 1) % MAX_RES_PAGES;
    }
  }
  return 0;
}

// Demand-paging resolver: map a page at va based on segment or zero-fill.
// Returns the physical address (pa) of the mapped page, or 0 on failure.
// access is "read" | "write" | "exec" (for logging).
uint64
demand_resolve(struct proc *p, pagetable_t pt, uint64 va, const char *access)
{
  va = PGROUNDDOWN(va);

  // Never map outside the reserved user range
  if (va >= p->sz) {
    printf("[pid %d] PAGEFAULT va=%p access=%s cause=invalid\n", p->pid, (void*)va, access);
    printf("[pid %d] KILL invalid-access va=%p access=%s\n", p->pid, (void*)va, access);
    setkilled(p);
    return 0;
  }

  // If already mapped, just return pa.
  uint64 pa0 = walkaddr(pt, va);
  if(pa0 != 0)
    return pa0;

  const char *cause = classify_fault_cause(p, va);
  if(cause == 0){
    // invalid access
    printf("[pid %d] PAGEFAULT va=%p access=%s cause=invalid\n", p->pid, (void*)va, access);
    printf("[pid %d] KILL invalid-access va=%p access=%s\n", p->pid, (void*)va, access);
    setkilled(p);
    return 0;
  }

  // Always log PAGEFAULT first
  if (VERBOSE_PAGING)
    printf("[pid %d] PAGEFAULT va=%p access=%s cause=%s\n", p->pid, (void*)va, access, cause);

  // Forbid writes to exec/text segments: kill on direct page-faulting writes
  if (strncmp(cause, "exec", 4) == 0 && access && access[0] == 'w') {
    printf("[pid %d] KILL write-to-exec va=%p access=%s\n", p->pid, (void*)va, access);
    setkilled(p);
    return 0;
  }

  char *mem = kalloc();
  if(mem == 0){
    // For heap growth, do not evict to satisfy OOM: make sbrkfail semantics match xv6 tests
    if (strncmp(cause, "heap", 4) == 0) {
      printf("[pid %d] MEMFULL\n", p->pid);
      printf("[pid %d] KILL out-of-memory va=%p access=%s\n", p->pid, (void*)va, access);
      setkilled(p);
      return 0;
    }
    // For stack/exec/data, allow eviction and retry
    printf("[pid %d] MEMFULL\n", p->pid);
    while (mem == 0) {
      int evicted = 0;
      for(int e = 0; e < 8; e++){
        if(try_evict_one(p)) evicted++; else break;
      }
      if (evicted == 0) {
        printf("[pid %d] KILL out-of-memory va=%p access=%s\n", p->pid, (void*)va, access);
        setkilled(p);
        return 0;
      }
      yield();
      mem = kalloc();
    }
  }
  memset(mem, 0, PGSIZE);

  if(strncmp(cause, "heap", 4) == 0 || strncmp(cause, "stack", 5) == 0){
    // Zero-fill for heap/stack
    // mappages can itself require kalloc for page-table pages; retry with eviction.
    if (strncmp(cause, "heap", 4) == 0) {
      // For heap, do not evict to create page-table pages; fail fast to satisfy sbrk semantics
      if(mappages(pt, va, PGSIZE, (uint64)mem, PTE_U|PTE_R|PTE_W) != 0){
        printf("[pid %d] KILL out-of-memory map va=%p access=%s\n", p->pid, (void*)va, access);
        setkilled(p);
        kfree(mem);
        return 0;
      }
    } else {
      for(;;){
        if(mappages(pt, va, PGSIZE, (uint64)mem, PTE_U|PTE_R|PTE_W) == 0)
          break;
        // mapping failed; try to evict and retry (stack)
        int evicted = 0;
        for(int e = 0; e < 8; e++){
          if(try_evict_one(p)) evicted++; else break;
        }
        if(evicted == 0){
          printf("[pid %d] KILL out-of-memory map va=%p access=%s\n", p->pid, (void*)va, access);
          setkilled(p);
          kfree(mem);
          return 0;
        }
        yield();
      }
    }
    if (VERBOSE_PAGING)
      printf("[pid %d] ALLOC va=%p\n", p->pid, (void*)va);
    if (VERBOSE_PAGING)
      printf("[pid %d] RESIDENT va=%p seq=%d\n", p->pid, (void*)va, p->next_fifo_seq++);
    else
      p->next_fifo_seq++;
    // enqueue resident page into FIFO
    {
      int seq = p->next_fifo_seq - 1;
      int kind = page_kind_for_va(p, va);
      fifo_enqueue(p, va, kind, seq);
    }
    return walkaddr(pt, va);
  }

  // Exec/data: load from executable file
  struct vmap seg;
  if(!is_exec_vaddr(p, va, &seg)){
    // Shouldn't happen; double-check
    kfree(mem);
    printf("[pid %d] KILL invalid-access va=%p access=%s\n", p->pid, (void*)va, access);
    setkilled(p);
    return 0;
  }

  uint64 file_off = 0, n = 0;
  int perms = PTE_U | PTE_R | seg.perms;

  // Compute file_off and n bytes to read
  {
    uint64 seg_off = va - seg.vaddr;
    file_off = seg.off + seg_off;
    uint64 file_last = seg.off + seg.filesz - 1;
    if(seg.filesz > 0 && file_off <= file_last){
      uint64 maxn = file_last - file_off + 1;
      n = (maxn > PGSIZE) ? PGSIZE : maxn;
    } 
    else{
      n = 0;
    }
  }

  if(p->exec_ip == 0){
    kfree(mem);
    printf("[pid %d] KILL invalid-access va=%p access=%s\n", p->pid, (void*)va, access);
    setkilled(p);
    return 0;
  }

  ilock(p->exec_ip);
  if(n > 0){
    if(readi(p->exec_ip, 0, (uint64)mem, file_off, n) != n){
      iunlock(p->exec_ip);
      kfree(mem);
      return 0;
    }
  }
  iunlock(p->exec_ip);

  // Map the exec/data page. mappages may need page-table pages; retry with eviction.
  for(;;){
    if(mappages(pt, va, PGSIZE, (uint64)mem, perms) == 0)
      break;
    int evicted = 0;
    for(int e = 0; e < 8; e++){
      if(try_evict_one(p)) evicted++; else break;
    }
    if(evicted == 0){
      printf("[pid %d] KILL out-of-memory map va=%p access=%s\n", p->pid, (void*)va, access);
      setkilled(p);
      kfree(mem);
      return 0;
    }
    yield();
  }

  if (VERBOSE_PAGING)
    printf("[pid %d] LOADEXEC va=%p\n", p->pid, (void*)va);
  if (VERBOSE_PAGING)
    printf("[pid %d] RESIDENT va=%p seq=%d\n", p->pid, (void*)va, p->next_fifo_seq++);
  else
    p->next_fifo_seq++;
  // enqueue resident page into FIFO
  {
    int seq = p->next_fifo_seq - 1;
    int kind = page_kind_for_va(p, va);
    fifo_enqueue(p, va, kind, seq);
  }

  return walkaddr(pt, va);
}