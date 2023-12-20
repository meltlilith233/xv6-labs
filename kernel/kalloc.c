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

struct pageref {
  // 自旋锁，保护临界资源，因为显然可能会有多个进程使用
  struct spinlock lock;
  // 记录页面被引用次数：只有KERNBASE~PHYSTOP的物理内存会被映射到
  int count[(PHYSTOP-KERNBASE)/PGSIZE];
} kref;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  initlock(&kref.lock, "kref");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE){
    kfree(p);
  }
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

  // kref为临界资源，写时要加锁：
  acquire(&kref.lock);
  // 被引用数为正数时才减1
  int *count=&kref.count[COW_INDEX(pa)];
  if(*count>0){
    (*count)--;
  }
  if(*count==0){
    release(&kref.lock);
    // 当页面被引用次数为0时才释放物理内存
    // memset()填充垃圾数据
    memset(pa, 1, PGSIZE);
    // pa为新的可用物理页，使其作为链头
    r=(struct run*)pa;
    acquire(&kmem.lock);
    r->next=kmem.freelist;
    kmem.freelist=r;
    release(&kmem.lock);
  }else{
    release(&kref.lock);
  }
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  // 获取一个空闲的物理页中，并将页引用数初始化为1
  r = kmem.freelist;
  if(r){
    kmem.freelist = r->next;
    acquire(&kref.lock);
    kref.count[COW_INDEX((uint64)r)]=1;
    release(&kref.lock);
  }
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}

// kref对应索引加1
void krefplus(uint64 pa){
  acquire(&kref.lock);
  kref.count[COW_INDEX(pa)]++;
  release(&kref.lock);
}

// 为映射了COW页的虚拟地址va分配实际的物理页
// 返回新分配的物理页地址，
// 返回值为0表示va映射的物理页不是COW页或分配失败
void* cow_alloc(pagetable_t pagetable, uint64 va){
  // 页对齐
  va=PGROUNDDOWN(va);
  // 获取va对应的pte
  pte_t *pte=walk(pagetable, va, 0);
  if(pte==0 || (*pte & PTE_V)==0 || (*pte & PTE_U)==0){
    return 0;
  }
  // 获取va对应的物理地址
  uint64 pa=PTE2PA(*pte);
  uint64 flags=PTE_FLAGS(*pte);
  if(flags & PTE_COW){
    acquire(&kref.lock);
    if(kref.count[COW_INDEX(pa)]==1){
      release(&kref.lock);
      // 如果被引用次数为1，修改PTE标志位即可
      *pte |= PTE_W;
      *pte &= ~PTE_COW;
      return (void*)pa;
    }else{
      release(&kref.lock);
      // 获取一个新的物理页
      uint64 mem=(uint64)kalloc();
      if(mem==0)return 0;
      memmove((char*)mem, (char*)pa, PGSIZE);
      // 解除原映射关系，且设置dofree=1，
      // 从而uvmunmap调用kfree()减少引用次数
      uvmunmap(pagetable, va, 1, 1);
      // 设置新的物理页可写，且去除COW标记
      flags=(flags|PTE_W) & ~PTE_COW;
      if(mappages(pagetable, va, PGSIZE, mem, flags)!=0){
        kfree((void*)mem);
        return 0;
      }
      return (void*)mem;
    }
  }
  return 0;
}