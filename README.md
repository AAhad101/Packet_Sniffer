[![Review Assignment Due Date](https://classroom.github.com/assets/deadline-readme-button-22041afd0340ce965d47ae6ef1cefeee28c7c493a6346c4f15d667ab976d596c.svg)](https://classroom.github.com/a/MXQf8Ibj)

Overview
Implements:

Demand paging with lazy mapping and page-fault handler.
FIFO page replacement with per-process queue and sequence numbers.
Per-process swap file /pgswp<PID> with swap-in/out and strict failure handling.
memstat syscall that reports per-page state to user space.
Precise event logging with VERBOSE_PAGING toggles.
Part 1: Demand Paging
Changes in 
kernel/vm.c
:
demand_resolve()
 handles faults and validates accesses for exec/text/data, heap, stack.
On first access:
Exec/text/data: LOADEXEC then RESIDENT.
Heap/stack: ALLOC then RESIDENT.
Logs PAGEFAULT with va, access, cause.
Lazy exec mapping: page-in from inode on demand.
Added a small guard so if eviction marks the process killed (e.g., SWAPFULL → KILL swap-exhausted), the fault returns immediately without continuing.
Part 2: FIFO Replacement
Files: 
kernel/vm.c
, 
kernel/proc.h
, 
kernel/proc.c
.
Per-process FIFO queue:
Tracks resident pages with monotonically increasing seq (p->next_fifo_seq).
Logs RESIDENT va=… seq=N.
On memory pressure: MEMFULL → VICTIM (seq, FIFO) → EVICT.
Clean, backed pages are DISCARD; others go to swap.
Part 3: Per-Process Swap
Files: 
kernel/exec.c
, 
kernel/vm.c
, 
kernel/proc.c
, 
kernel/proc.h
, 
kernel/defs.h
.
Swap file lifecycle:
Created on exec() as /pgswp<PID> (
kernel/exec.c
).
Deleted on exit() with SWAPCLEANUP freed_slots=K (
kernel/proc.c
).
Eviction:
Dirty or non-backed pages → find free slot (bitmap) → write → EVICT state=dirty then SWAPOUT slot=N.
If no free slot for a dirty page → SWAPFULL then KILL swap-exhausted.
Swap-in:
On fault to a swapped VA → read from slot, free slot, SWAPIN then RESIDENT.
Logging strictly follows required sequences; all logs via printf under VERBOSE_PAGING.
Part 4: memstat Syscall
Files: 
kernel/syscall.h
, 
kernel/syscall.c
, 
kernel/sysproc.c
, 
kernel/memstat.h
, 
user/user.h
, 
user/usys.pl
.
memstat() fills struct proc_mem_stat with per-page entries (UNMAPPED/RESIDENT/SWAPPED, is_dirty, seq, swap_slot) up to MAX_PAGES_INFO.
Useful for verifying resident/swapped sets and FIFO sequencing.
Tests Created
user/t_swap.c
: Creates pressure, triggers FIFO evictions with SWAPOUT, and re-accesses an old heap page to exercise SWAPIN. Under extreme pressure, swap may exhaust before a visible SWAPIN; to reliably see SWAPIN, increase frames a bit or lighten allocations (see run conditions).
user/t_memstat.c
: Calls memstat() at stages to print per-page state, dirty, seq, and slot.
evicttest.c
: Stress allocation, verify re-touch data correctness, and print summarized mem stats.
How to Run (Conditions)
Toggle logs: 
kernel/vm.c
 has VERBOSE_PAGING (default 1). Leave at 1 for visibility.
Constrain RAM to trigger eviction: Set in 
kernel/memlayout.h
:
For heavy pressure: e.g., #define PHYSTOP (KERNBASE + 12*1024*1024)
For normal runs/usertests: #define PHYSTOP (KERNBASE + 128*1024*1024)
Single CPU for ordered logs: run QEMU with CPUS=1.
Commands:
sh
# Build and run with logs ordered
make clean qemu CPUS=1

# Run targeted tests from the shell
t_swap
t_memstat
# Or run all
usertests
Expected Logs (sampling)
Heap/stack: PAGEFAULT → ALLOC → RESIDENT seq=N
Exec: PAGEFAULT → LOADEXEC → RESIDENT seq=N
Replacement path: PAGEFAULT → MEMFULL → VICTIM (seq, FIFO) → EVICT → (DISCARD | SWAPOUT slot=N) → (ALLOC|LOADEXEC|SWAPIN) → RESIDENT
Swap full: … → EVICT state=dirty → SWAPFULL → KILL swap-exhausted
Exit cleanup: SWAPCLEANUP freed_slots=K

