// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#ifdef LAB_LOCK
#define HASH_LEN 13
struct hash_bucket
{
  struct buf head;
  struct spinlock bkt_lk;
};
char blk_name[32 * HASH_LEN] = {0};
#endif

struct {
  struct spinlock lock;
#ifdef LAB_LOCK
  struct hash_bucket bhash[HASH_LEN];
#endif

  struct buf buf[NBUF];

#ifndef LAB_LOCK
  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head;
#endif
} bcache;

void
binit(void)
{
  struct buf *b;

  #ifndef LAB_LOCK
  initlock(&bcache.lock, "bcache");

  // Create linked list of buffers
  bcache.head.prev = &bcache.head;
  bcache.head.next = &bcache.head;
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->next = bcache.head.next;
    b->prev = &bcache.head;
    initsleeplock(&b->lock, "buffer");
    bcache.head.next->prev = b;
    bcache.head.next = b;
  }
  #else
  for (int i = 0; i < HASH_LEN; i++)
  {
    snprintf(blk_name + i * 32, 32, "bcache_bhash_lk%d", i);
    initlock(&bcache.bhash[i].bkt_lk, blk_name + i * 32);

    bcache.bhash[i].head.prev = &bcache.bhash[i].head;
    bcache.bhash[i].head.next = &bcache.bhash[i].head;
  }
  for(b = bcache.buf; b < bcache.buf + NBUF; b++)
  {
    uint hash_idx = (uint64)b % HASH_LEN;
    b->next = bcache.bhash[hash_idx].head.next;
    b->prev = &bcache.bhash[hash_idx].head;
    initsleeplock(&b->lock, "buffer");
    bcache.bhash[hash_idx].head.next->prev = b;
    bcache.bhash[hash_idx].head.next = b;
  }
  #endif
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  // Is the block already cached?
  #ifndef LAB_LOCK
  acquire(&bcache.lock);

  for(b = bcache.head.next; b != &bcache.head; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock);
      acquiresleep(&b->lock);
      return b;
    }
  }
  #else
  uint hash_idx = blockno % HASH_LEN;
  acquire(&bcache.bhash[hash_idx].bkt_lk);
  b = bcache.bhash[hash_idx].head.next;
  while (b != &bcache.bhash[hash_idx].head)
  {
    if (b->dev == dev && b->blockno == blockno)
    {
      b->refcnt++;
      release(&bcache.bhash[hash_idx].bkt_lk);
      acquiresleep(&b->lock);
      return b;
    }
    b = b->next;
  }
  #endif

  // Not cached.
  #ifndef LAB_LOCK
  // Recycle the least recently used (LRU) unused buffer.
  for(b = bcache.head.prev; b != &bcache.head; b = b->prev){
    if(b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache.lock);
      acquiresleep(&b->lock);
      return b;
    }
  }
  #else
  for(b = bcache.bhash[hash_idx].head.prev; 
      b != &bcache.bhash[hash_idx].head; b = b->prev)
  {
    if(b->refcnt == 0)
    {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache.bhash[hash_idx].bkt_lk);
      acquiresleep(&b->lock);
      return b;
    }
  }

  int i = (hash_idx + 1) % HASH_LEN;
  while (i != hash_idx)
  { 
    acquire(&bcache.bhash[i].bkt_lk);
    for(b = bcache.bhash[i].head.prev; 
      b != &bcache.bhash[i].head; b = b->prev)
    {
      if(b->refcnt == 0)
      {
        b->dev = dev;
        b->blockno = blockno;
        b->valid = 0;
        b->refcnt = 1;

        b->next->prev = b->prev;
        b->prev->next = b->next;
        b->next = bcache.bhash[hash_idx].head.next;
        b->prev = &bcache.bhash[hash_idx].head;
        bcache.bhash[hash_idx].head.next->prev = b;
        bcache.bhash[hash_idx].head.next = b;

        release(&bcache.bhash[i].bkt_lk);
        release(&bcache.bhash[hash_idx].bkt_lk);
        acquiresleep(&b->lock);
        return b;
      }
    }
    release(&bcache.bhash[i].bkt_lk);
    i = (i + 1) % HASH_LEN;
  }
  #endif
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  #ifndef LAB_LOCK
  acquire(&bcache.lock);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.head.next;
    b->prev = &bcache.head;
    bcache.head.next->prev = b;
    bcache.head.next = b;
  }
  
  release(&bcache.lock);
  #else
  uint hash_idx = b->blockno % HASH_LEN;
  acquire(&bcache.bhash[hash_idx].bkt_lk);
  b->refcnt--;
  if (b->refcnt == 0)
  {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.bhash[hash_idx].head.next;
    b->prev = &bcache.bhash[hash_idx].head;
    bcache.bhash[hash_idx].head.next->prev = b;
    bcache.bhash[hash_idx].head.next = b;
  }
  release(&bcache.bhash[hash_idx].bkt_lk);
  #endif
}

void
bpin(struct buf *b) {
  #ifdef LAB_LOCK
  uint hash_idx = b->blockno % HASH_LEN;
  acquire(&bcache.bhash[hash_idx].bkt_lk);
  #endif
  b->refcnt++;
  #ifdef LAB_LOCK
  release(&bcache.bhash[hash_idx].bkt_lk);
  #endif
}

void
bunpin(struct buf *b) {
  #ifdef LAB_LOCK
  uint hash_idx = b->blockno % HASH_LEN;
  acquire(&bcache.bhash[hash_idx].bkt_lk);
  #endif
  b->refcnt--;
  #ifdef LAB_LOCK
  release(&bcache.bhash[hash_idx].bkt_lk);
  #endif
}
