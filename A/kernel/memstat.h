#pragma once
#include "types.h"

// Maximum number of page info entries returned by memstat.
#define MAX_PAGES_INFO 128

// Page state values
#define P_UNMAPPED 0
#define P_RESIDENT 1
#define P_SWAPPED  2

struct page_info {
  uint64 va;      // page-aligned VA
  int state;      // P_UNMAPPED | P_RESIDENT | P_SWAPPED
  int is_dirty;   // 0 | 1 (heap/stack considered dirty; exec clean)
  int seq;        // FIFO sequence if RESIDENT, else -1
  int swap_slot;  // slot if SWAPPED, else -1
};

struct proc_mem_stat {
  int num;                    // number of valid entries in info[]
  struct page_info info[MAX_PAGES_INFO];
};
