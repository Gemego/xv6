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

#ifdef LAB_COW
uint8 ref_count[0x8000] = {0};
#endif

#ifdef LAB_LOCK
char lk_name[32 * NCPU] = {0};
uint64 all_mem = 0;
#endif

struct {
  #ifndef LAB_LOCK
  struct spinlock lock;
  struct run *freelist;
  #else
  struct spinlock frls_lk[NCPU];
  struct run *freelist[NCPU];
  #endif
} kmem;

void
kinit()
{
  #ifndef LAB_LOCK
  initlock(&kmem.lock, "kmem");
  #else
  for (int i = 0; i < NCPU; i++)
  {
    kmem.freelist[i] = 0;

    snprintf(lk_name + i * 32, 32, "kmem_frls_lk%d", i);
    initlock(&kmem.frls_lk[i], lk_name + i * 32);
  }
  #endif

  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
  {
    kfree_init(p);
    #ifdef LAB_LOCK
    all_mem += 1;
    #endif
  }
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)

void
kfree_init(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  #ifndef LAB_LOCK
  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
  #else
  push_off();
  acquire(&kmem.frls_lk[cpuid()]);
  r->next = kmem.freelist[cpuid()];
  kmem.freelist[cpuid()] = r;
  release(&kmem.frls_lk[cpuid()]);
  pop_off();
  #endif
}

void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  #ifdef LAB_COW
  set_ref_count((uint64)pa, 0);

  if (get_ref_count((uint64)pa) == 0)
  {
  #endif
    // Fill with junk to catch dangling refs.
    memset(pa, 1, PGSIZE);

    r = (struct run*)pa;

    #ifndef LAB_LOCK
    acquire(&kmem.lock);
    r->next = kmem.freelist;
    kmem.freelist = r;
    release(&kmem.lock);
    #else
    push_off();
    acquire(&kmem.frls_lk[cpuid()]);
    r->next = kmem.freelist[cpuid()];
    kmem.freelist[cpuid()] = r;
    release(&kmem.frls_lk[cpuid()]);
    pop_off();
    #endif
  #ifdef LAB_COW
  }
  #endif
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  #ifndef LAB_LOCK
  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  #else
  push_off();
  acquire(&kmem.frls_lk[cpuid()]);
  r = kmem.freelist[cpuid()];

  uint mem_wanted = all_mem / NCPU;
  struct run *tmp = 0;

  if(r)
  {
    kmem.freelist[cpuid()] = r->next;
  }
  else
  {
    for (int j = 0; j < NCPU; j++)
    {
      if (j == cpuid())
        continue;
      acquire(&kmem.frls_lk[j]);
      r = kmem.freelist[j];
      if (r)
      {
        mem_wanted -= 1;
        while(r && mem_wanted > 0)
        {
          kmem.freelist[j] = r->next;
          tmp = r->next;
          r->next = kmem.freelist[cpuid()];
          kmem.freelist[cpuid()] = r;
          r = tmp;
          mem_wanted -= 1;
        }
        r = kmem.freelist[cpuid()];
        release(&kmem.frls_lk[j]);
        break;
      }
      release(&kmem.frls_lk[j]);
    }

    if(r)
      kmem.freelist[cpuid()] = r->next;
  }

  release(&kmem.frls_lk[cpuid()]);

  pop_off();
  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  #endif

  #ifdef LAB_COW
  set_ref_count((uint64)r, 1);
  #endif

  return (void*)r;
}

uint64 kcount(void)
{
  struct run *r = 0;
  uint64 free_mem = 0;

  #ifndef LAB_LOCK
  acquire(&kmem.lock);
  r = kmem.freelist;
  while(r)
  {
    r = r->next;
    free_mem += PGSIZE;
  }
  release(&kmem.lock);
  #else
  push_off();
  acquire(&kmem.frls_lk[cpuid()]);
  pop_off();
  r = kmem.freelist[cpuid()];
  while(r)
  {
    r = r->next;
    free_mem += PGSIZE;
  }
  release(&kmem.frls_lk[cpuid()]);
  #endif

  return free_mem;
}

#ifdef LAB_COW
void set_ref_count(uint64 pa, int is_incre)
{
  uint64 end_bound = PGROUNDUP((uint64)end);
  if (pa < end_bound)  // pa < end_bound must be trampoline
    return;

  if (pa % PGSIZE != 0)
    panic("set_ref_count(): pa must be aligned");

  if (is_incre)
  {
    if (((pa - end_bound) >> 12) < 0x8000)
    {
      acquire(&kmem.lock);
      ref_count[((pa - end_bound) >> 12)] += 1;
      release(&kmem.lock);
    }
    else
      panic("ref_count[pa - end_bound] index out of boundary");
  }
  else
  {
    acquire(&kmem.lock);
    if (ref_count[((pa - end_bound) >> 12)] == 0)
      panic("ref_count[pa - end_bound] has already been zero");
    else
      ref_count[((pa - end_bound) >> 12)] -= 1;
    release(&kmem.lock);
  }
}

void clear_ref_count(uint64 pa)
{
  uint64 end_bound = PGROUNDUP((uint64)end);
  if (pa < end_bound)
    return;

  if (pa % PGSIZE != 0)
    panic("set_ref_count(): pa must be aligned");

  acquire(&kmem.lock);
  ref_count[((pa - end_bound) >> 12)] = 0;
  release(&kmem.lock);
}

uint8 get_ref_count(uint64 pa)
{
  uint64 end_bound = PGROUNDUP((uint64)end);
  if (pa < end_bound)
    panic("get_ref_count(): pa <(uint64)end");

  if (pa % PGSIZE != 0)
    panic("get_ref_count(): pa must be aligned");

  return ref_count[((uint64)pa - end_bound) >> 12];
}
#endif
