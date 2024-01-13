// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

uint8 ref_count[2^15] = {0};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);

  clear_ref_count((uint64)pa);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk

  set_ref_count((uint64)r, 1);

  return (void*)r;
}

int kcount(void)
{
  struct run *r = 0;
  int free_mem = 0;

  acquire(&kmem.lock);
  r = kmem.freelist;
  while(r)
  {
    r = r->next;
    free_mem += PGSIZE;
  }
  release(&kmem.lock);

  return free_mem;
}

void set_ref_count(uint64 pa, int is_incre)
{
  if (pa < (uint64)end)  // must be trampoline
    return;
  
  if (pa % PGSIZE != 0)
    panic("set_ref_count(): pa must be aligned");
  
  uint64 end_bound = PGROUNDUP((uint64)end);

  if (is_incre)
  {
    ref_count[pa - end_bound] += 1;
  }
  else
  {
    ref_count[pa - end_bound] -= 1;
    if (ref_count[pa - end_bound] < 0)
      panic("ref_count has negative element");
  }
}

void clear_ref_count(uint64 pa)
{
  if (pa < (uint64)end)
    return;

  if (pa % PGSIZE != 0)
    panic("set_ref_count(): pa must be aligned");

  uint64 end_bound = PGROUNDUP((uint64)end);

  ref_count[pa - end_bound] = 0;
}

int get_ref_count(uint64 pa)
{
  if (pa < (uint64)end)
    panic("get_ref_count(): pa <(uint64)end");

  if (pa % PGSIZE != 0)
    panic("get_ref_count(): pa must be aligned");

  uint64 end_bound = PGROUNDUP((uint64)end);

  return ref_count[pa - end_bound];
}
