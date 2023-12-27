#include "types.h"
#include "riscv.h"
#include "param.h"
#include "defs.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "sysinfo.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
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
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
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


  argint(0, &n);
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  backtrace();
  return 0;
}


#ifdef LAB_PGTBL

inline int _pgcheck(int pte_bit)
{
  // lab pgtbl: your code here.

  uint64 buf = 0; // char *buf
  int page_num = 0;
  uint64 abits = 0; // unsigned int abits

  argaddr(0, &buf);
  argint(1, &page_num);
  argaddr(2, &abits);

  unsigned int tem_abits = 0;
  struct proc *proc_p = myproc();

  for (int i = 0; i < page_num; i++)
  {
    pte_t *pte = walk(proc_p->pagetable, buf + PGSIZE * i, 0);
    if (*pte & pte_bit)
    {
      tem_abits |= 1 << i;
      *pte &= ~pte_bit;
    }
  }
  
  if (copyout(proc_p->pagetable, abits, (char *)&tem_abits, sizeof(tem_abits)) < 0)
  {
    return -1;
  }

  return 0;
}

int
sys_pgaccess(void)
{
  // lab pgtbl: your code here.

  return _pgcheck(PTE_A);
}

int
sys_pgdirty(void)
{
  // lab pgtbl: your code here.

  return _pgcheck(PTE_D);
}

#endif

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
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

uint64 sys_trace(void)
{
    int msk;

    argint(0, &msk);
    myproc()->mask = msk;
    return 0;
}

uint64 sys_sysinfo(void)
{
    uint64 info_p;

    argaddr(0, &info_p);
    struct proc *proc_p = myproc();

    struct sysinfo sinfo;
    sinfo.freemem = (uint64)kcount();
    sinfo.nproc = (uint64)proccount();

    if (copyout(proc_p->pagetable, info_p, (char *)&sinfo, sizeof(struct sysinfo)) < 0)
    {
      return -1;
    }

    return 0;
}