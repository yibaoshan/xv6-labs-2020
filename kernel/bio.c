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

#define NBUCKET 13  // 哈希桶数量，一般用质数
#define NBUFFER CEIL(NBUF, NBUCKET)  // NBUF 默认为 30，这里向上取整后结果应该是 3，即每个桶里面有 3 个 buf

#define CEIL(num, divisor) (((num) + (divisor) - 1) / (divisor)) // 向上取整的宏

struct {
  struct spinlock lock;
  struct buf buf[NBUFFER];
} bcache[NBUCKET];

static uint global_timestamp;  // 全局时间戳计数器

void
binit(void)
{
  for (int i = 0; i < NBUCKET; i++) {
      initlock(&bcache[i].lock, "bcache");
      for (int j = 0; j < NBUFFER; j++) {
          bcache[i].buf[j].timestamp = 0;
          initsleeplock(&bcache[i].buf[j].lock, "buffer");
      }
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  uint bucket_id = hash(blockno);

  // 优先在当前哈希桶中，查找已缓存的块
  acquire(&bcache[bucket_id].lock);
  for(int i = 0; i < NBUFFER; i++) {
    b = &bcache[bucket_id].buf[i];
    if(b->dev == dev && b->blockno == blockno) {
      b->refcnt++;
      // 增加引用的同时，更新时间戳
      update_timestamp(b);
      release(&bcache[bucket_id].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }
  release(&bcache[bucket_id].lock);

  uint min_timestamp = 0xffffffff;
  struct buf *least_used_buf = 0;

  // 没有找到缓存块，和上一小节的 kalloc 一样，环形遍历所有桶，查找其他桶中未使用的空闲块
  // 遍历所有桶的过程中，记录最少使用并且最久未使用的块
  // 如果没有空闲块，那么就直接使用 最少使用并且最久未使用的块
  for(int i = 0, cur_bucket = bucket_id; i < NBUCKET; i++, cur_bucket++) {
    if(cur_bucket == NBUCKET)
      cur_bucket = 0;  // 环形遍历

    acquire(&bcache[cur_bucket].lock);
    for(int j = 0; j < NBUFFER; j++) {
      b = &bcache[cur_bucket].buf[j];
      if(b->refcnt == 0) {  // 找到未使用的块
        b->dev = dev;
        b->blockno = blockno;
        b->valid = 0;
        b->refcnt = 1;
        update_timestamp(b);

        release(&bcache[cur_bucket].lock);
        acquiresleep(&b->lock);
        return b;
      }

      //更新 最久未使用的 buffer
      if(b->timestamp < min_timestamp) {
        min_timestamp = b->timestamp;
        least_used_buf = b;
      }
    }
    release(&bcache[cur_bucket].lock);
  }

  // 其他桶里面也没有空闲块，那就只能使用 最久未使用的 buffer
  if(least_used_buf) {
    acquire(&bcache[bucket_id].lock);
    // 一大堆赋值和初始化操作
    least_used_buf->dev = dev;
    least_used_buf->blockno = blockno;
    least_used_buf->valid = 0;
    least_used_buf->refcnt = 1;
    update_timestamp(least_used_buf);
    release(&bcache[bucket_id].lock);
    acquiresleep(&least_used_buf->lock);
    return least_used_buf;
  }

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

  int bucket_id = hash(b->blockno);
  acquire(&bcache[bucket_id].lock);
  b->refcnt--;
  release(&bcache[bucket_id].lock);
  releasesleep(&b->lock);
}

void
bpin(struct buf *b) {
  int bucket_id = hash(b->blockno);
  acquire(&bcache[bucket_id].lock);
  b->refcnt++;
  release(&bcache[bucket_id].lock);
}

void
bunpin(struct buf *b) {
  int bucket_id = hash(b->blockno);
  acquire(&bcache[bucket_id].lock);
  b->refcnt--;
  release(&bcache[bucket_id].lock);
}

uint
hash(uint blockno)
{
    return blockno % NBUCKET;
}

void
update_timestamp(struct buf *b)
{
    // global_timestamp 会有溢出问题，暂不考虑
    b->timestamp = __sync_fetch_and_add(&global_timestamp, 1);
//    printf("time %d\n",b->timestamp);
}


