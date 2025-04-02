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

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

// 物理地址转 index
#define PA2IDX(p) (((uint64)(p)) / PGSIZE)

struct {
  struct spinlock lock; // 保证并发安全
  int ref_arr[PHYSTOP / PGSIZE]; // 每个物理页面的引用次数
} page_ref; // 模仿 kmem 新建页面引用结构

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  initlock(&page_ref.lock, "pageref"); // 初始化 page_ref.lock
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

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  acquire(&page_ref.lock);

  // 只有页面的引用计数为 0，没有进程映射到该物理页了，才真正释放页面
  if(--page_ref.ref_arr[PA2IDX(pa)] <= 0) {

    // Fill with junk to catch dangling refs.
    memset(pa, 1, PGSIZE);

    r = (struct run*)pa;

    acquire(&kmem.lock);
    r->next = kmem.freelist;
    kmem.freelist = r;
    release(&kmem.lock);
  }

  release(&page_ref.lock);
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

  if(r){
    memset((char*)r, 5, PGSIZE); // fill with junk
    // 如果是新分配的有效物理页，引用计数为 1
    page_ref.ref_arr[PA2IDX(r)] = 1;
  }
  return (void*)r;
}

// 如果有必要，克隆一页物理页
void *ktry_pgclone(void *pa) {

    acquire(&page_ref.lock);

    // 这个物理页本来就只有一个地方引用，直接返回
    if(page_ref.ref_arr[PA2IDX(pa)] <= 1) {
        release(&page_ref.lock);
        return pa;
    }

    // 申请一页物理页
    uint64 newpa = (uint64)kalloc();

    if(newpa == 0) {
        release(&page_ref.lock);
        return 0;
    }

    // 复制老物理页内容到新页
    memmove((void*)newpa, (void*)pa, PGSIZE);

    // 老物理页引用减一
    page_ref.ref_arr[PA2IDX(pa)]--;

    release(&page_ref.lock);
    return (void*)newpa;
}

// 增加物理页面的引用次数
void kparef_inc(void *pa) {
    acquire(&page_ref.lock);
    page_ref.ref_arr[PA2IDX(pa)]++;
    release(&page_ref.lock);
}