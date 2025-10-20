#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "elf.h"
#include "fs.h"
#include "stat.h"

//static int loadseg(pde_t *, uint64, struct inode *, uint, uint);

// map ELF permissions to PTE permission bits.
int flags2perm(int flags)
{
    int perm = 0;
    if(flags & 0x1)
      perm = PTE_X;
    if(flags & 0x2)
      perm |= PTE_W;
    return perm;
}

//
// the implementation of the exec() system call
//
int
kexec(char *path, char **argv)
{
  char *s, *last;
  int i, off;
  uint64 argc, sz = 0, sp, ustack[MAXARG], stackbase;
  struct elfhdr elf;
  struct inode *ip;
  struct proghdr ph;
  pagetable_t pagetable = 0, oldpagetable;
  struct proc *p = myproc();

  begin_op();

  // Open the executable file.
  if((ip = namei(path)) == 0){
    end_op();
    return -1;
  }
  ilock(ip);

  // Read the ELF header.
  if(readi(ip, 0, (uint64)&elf, 0, sizeof(elf)) != sizeof(elf))
    goto bad;

  // Is this really an ELF file?
  if(elf.magic != ELF_MAGIC)
    goto bad;

  if((pagetable = proc_pagetable(p)) == 0)
    goto bad;

  // Record loadable segments; do not map or load now (demand paging).
  p->vmaps_len = 0;
  p->text_lo = p->text_hi = 0;
  p->data_lo = p->data_hi = 0;
  sz = 0;

  for(i = 0, off = elf.phoff; i < elf.phnum; i++, off += sizeof(ph)){
    if(readi(ip, 0, (uint64)&ph, off, sizeof(ph)) != sizeof(ph))
      goto bad;
    if(ph.type != ELF_PROG_LOAD)
      continue;
    if(ph.memsz < ph.filesz)
      goto bad;
    if(ph.vaddr + ph.memsz < ph.vaddr)
      goto bad;
    if(ph.vaddr % PGSIZE != 0)
      goto bad;

    if(p->vmaps_len >= MAX_VMAPS)
      goto bad;

    // Record vmap
    p->vmaps[p->vmaps_len].vaddr  = ph.vaddr;
    p->vmaps[p->vmaps_len].memsz  = ph.memsz;
    p->vmaps[p->vmaps_len].filesz = ph.filesz;
    p->vmaps[p->vmaps_len].off    = ph.off;
    p->vmaps[p->vmaps_len].perms  = flags2perm(ph.flags);
    p->vmaps_len++;

    // Track text/data ranges for logging and heap_start
    uint64 seg_lo = ph.vaddr;
    uint64 seg_hi = ph.vaddr + ph.memsz;
    if(p->vmaps[p->vmaps_len-1].perms & PTE_X){
      if(p->text_lo == 0 || seg_lo < p->text_lo) p->text_lo = seg_lo;
      if(seg_hi > p->text_hi) p->text_hi = seg_hi;
    } 
    else{
      if(p->data_lo == 0 || seg_lo < p->data_lo) p->data_lo = seg_lo;
      if(seg_hi > p->data_hi) p->data_hi = seg_hi;
    }

    if(seg_hi > sz) sz = seg_hi;
  }

  // Drop previous exec image reference if any, then keep a referenced
  // copy of the new executable for demand loads
  if(p->exec_ip)
    iput(p->exec_ip);
  p->exec_ip = idup(ip);

  iunlockput(ip);
  end_op();
  ip = 0;

  p = myproc();
  uint64 oldsz = p->sz;

  // Reserve stack space only; allocate pages on demand
  sz = PGROUNDUP(sz);
  sz = sz + (USERSTACK+1)*PGSIZE;   // guard + USERSTACK pages
  sp = sz;
  stackbase = sp - USERSTACK*PGSIZE;
  p->stack_top = sp;
  
  // Make reserved layout visible to the fault resolver before copyouts
  // so stack faults are classified correctly.
  p->heap_start = p->data_hi;
  p->sz = sz;

  // Reset per-process resident set (pages will be repopulated lazily after exec)
  p->res_head = p->res_tail = p->res_count = 0;

  // Part 3: create per-process swap file /pgswpPID and reset swap slots
  {
    // Clean any previous reference
    if (p->swap_ip) { iput(p->swap_ip); p->swap_ip = 0; }
    p->swap_used_count = 0;
    for (int si = 0; si < 1024; si++) p->swap_used[si] = 0;
    // Build path
    char path[32]; int i = 0;
    path[i++] = '/'; path[i++] = 'p'; path[i++] = 'g'; path[i++] = 's'; path[i++] = 'w'; path[i++] = 'p';
    int pid = p->pid; char tmp[16]; int t = 0; do { tmp[t++] = '0' + (pid % 10); pid /= 10; } while(pid);
    for (int j = t - 1; j >= 0; j--) path[i++] = tmp[j];
    path[i] = 0;
    // Create file
    struct inode *sip = kcreate(path, T_FILE, 0, 0);
    if (sip)
      p->swap_ip = sip;
  }

  // Log INIT-LAZYMAP before any copyout that may trigger a page fault for this process
  printf("[pid %d] INIT-LAZYMAP text=[%p,%p) data=[%p,%p) heap_start=%p stack_top=%p\n",
         p->pid, (void*)p->text_lo, (void*)p->text_hi,
         (void*)p->data_lo, (void*)p->data_hi,
         (void*)p->data_hi, (void*)p->stack_top);

  // Copy argument strings into new stack, remember their
  // addresses in ustack[].
  for(argc = 0; argv[argc]; argc++) {
    if(argc >= MAXARG)
      goto bad;
    sp -= strlen(argv[argc]) + 1;
    sp -= sp % 16; // riscv sp must be 16-byte aligned
    if(sp < stackbase)
      goto bad;
    if(copyout(pagetable, sp, argv[argc], strlen(argv[argc]) + 1) < 0)
      goto bad;
    ustack[argc] = sp;
  }
  ustack[argc] = 0;

  // push a copy of ustack[], the array of argv[] pointers.
  sp -= (argc+1) * sizeof(uint64);
  sp -= sp % 16;
  if(sp < stackbase)
    goto bad;
  if(copyout(pagetable, sp, (char *)ustack, (argc+1)*sizeof(uint64)) < 0)
    goto bad;

  // a0 and a1 contain arguments to user main(argc, argv)
  // argc is returned via the system call return
  // value, which goes in a0.
  p->trapframe->a1 = sp;

  // Save program name for debugging.
  for(last=s=path; *s; s++)
    if(*s == '/')
      last = s+1;
  safestrcpy(p->name, last, sizeof(p->name));
    
  // Commit to the user image.
  oldpagetable = p->pagetable;
  p->pagetable = pagetable;
  p->sz = sz;
  p->trapframe->epc = elf.entry;  // initial program counter = main
  p->trapframe->sp = sp; // initial stack pointer
  proc_freepagetable(oldpagetable, oldsz);

  return argc; // this ends up in a0, the first argument to main(argc, argv)

 bad:
  if(pagetable)
    proc_freepagetable(pagetable, sz);
  if(ip){
    iunlockput(ip);
    end_op();
  }
  return -1;
}

// Load an ELF program segment into pagetable at virtual address va.
// va must be page-aligned
// and the pages from va to va+sz must already be mapped.
// Returns 0 on success, -1 on failure.

// Not used anywhere
#if 0
static int
loadseg(pagetable_t pagetable, uint64 va, struct inode *ip, uint offset, uint sz)
{
  uint i, n;
  uint64 pa;

  for(i = 0; i < sz; i += PGSIZE){
    pa = walkaddr(pagetable, va + i);
    if(pa == 0)
      panic("loadseg: address should exist");
    if(sz - i < PGSIZE)
      n = sz - i;
    else
      n = PGSIZE;
    if(readi(ip, 0, (uint64)pa, offset+i, n) != n)
      return -1;
  }
  
  return 0;
}
#endif
