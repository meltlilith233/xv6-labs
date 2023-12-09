#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "sysinfo.h"

uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

// trace syscall
// 接受一个掩码参数，指定追踪的系统调用
uint64
sys_trace(void){
  int trace_mask;
  // 从a0寄存器取出系统调用的int型参数
  if(argint(0, &trace_mask)<0){
    return -1;
  }
  // trace系统调用功能：设置当前进程的trace_mask
  myproc()->trace_mask=trace_mask;
  return 0;
}

// sysinfo syscall
// 接受一个地址参数，sysinfo系统调用需要将从内核中获取的相关信息传到该地址中
uint64
sys_sysinfo(void){
  struct sysinfo info;
  uint64 addr;
  // 从a0寄存器取出系统调用的地址类型的参数
  if(argaddr(0, &addr)<0){
    return -1;
  }
  info.freemem=free_mem();
  info.nproc=n_proc();
  // copyout()：将info的内容从内核空间中复制到当前用户进程的逻辑内存空间中的addr地址中
  if(copyout(myproc()->pagetable, addr, (char*)&info, sizeof(info))<0){
    return -1;
  }
  return 0;
}