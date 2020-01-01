#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"
#ifdef CS333_P2
#include "uproc.h"
#include "pdx.h"
#endif // CS333_P2

#ifdef CS333_P3
#define statecount NELEM(states)
#endif // CS333_P3

static char *states[] = {
[UNUSED]    "unused",
[EMBRYO]    "embryo",
[SLEEPING]  "sleep ",
[RUNNABLE]  "runble",
[RUNNING]   "run   ",
[ZOMBIE]    "zombie"
};

#ifdef CS333_P3
struct ptrs {
  struct proc* head;
  struct proc* tail;
};
#endif // CS333_P3



static struct {
  struct spinlock lock;
  struct proc proc[NPROC];
#ifdef CS333_P3
  struct ptrs list[statecount];
#endif // CS333_P3
#ifdef CS333_P4
  struct ptrs ready[MAXPRIO+1];
  uint PromoteAtTime;
#endif // CS333_P4
} ptable;

static struct proc *initproc;

uint nextpid = 1;
extern void forkret(void);
extern void trapret(void);
static void wakeup1(void* chan);

#ifdef CS333_P3
static void initProcessLists(void);
static void initFreeList(void);
static void stateListAdd(struct ptrs*, struct proc*);
static int stateListRemove(struct ptrs*, struct proc* p);
static void assertState(struct proc *p, enum procstate state);
#endif // CS333_P3

#ifdef CS333_P4
static void promoteLists(void);
#endif // CS333_P4

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

// Must be called with interrupts disabled
int
cpuid() {
  return mycpu()-cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu*
mycpu(void)
{
  int apicid, i;

  if(readeflags()&FL_IF)
    panic("mycpu called with interrupts enabled\n");

  apicid = lapicid();
  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
  // a reverse map, or reserve a register to store &cpus[i].
  for (i = 0; i < ncpu; ++i) {
    if (cpus[i].apicid == apicid) {
      return &cpus[i];
    }
  }
  panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc*
myproc(void) {
  struct cpu *c;
  struct proc *p;
  pushcli();
  c = mycpu();
  p = c->proc;
  popcli();
  return p;
}

// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);

#ifdef CS333_P3
  if(ptable.list[UNUSED].head == NULL){
    release(&ptable.lock);
    return 0;
  }

  p = ptable.list[UNUSED].head;

  int rc = stateListRemove(&ptable.list[UNUSED], p);
  if(rc < 0)
    panic("Error removing from UNUSED in allocproc().\n");

  assertState(p,UNUSED);
  p->state = EMBRYO;
  stateListAdd(&ptable.list[EMBRYO],p);

#else
  int found = 0;
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED) {
      found = 1;
      break;
    }
  if (!found) {
    release(&ptable.lock);
    return 0;
  }
  p->state = EMBRYO;
#endif
  p->pid = nextpid++;
  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
#ifdef CS333_P3
    acquire(&ptable.lock);
    
    int rc = stateListRemove(&ptable.list[EMBRYO],p);
    if(rc < 0)
      panic("Error removing from EMBRYO in allocproc().\n");
    
    assertState(p,EMBRYO);
    p->state = UNUSED;
    stateListAdd(&ptable.list[UNUSED],p);

    release(&ptable.lock);
#else
    p->state = UNUSED;
#endif
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

#ifdef CS333_P1
  // Initialize start_ticks to value of existing global kernal
  // variable ticks so process knows when it was created
  p->start_ticks = ticks;
#endif /// CS333_P1

#ifdef CS333_P2
  p->cpu_ticks_total = 0;
  p->cpu_ticks_in = 0;
#endif // CS333_P2

#ifdef CS333_P4
//p->priority = MAXPRIO;
  p->priority = PRIORITY;
  p->budget = BUDGET;
#endif // CS333_P4

  return p;
}

// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

#ifdef CS333_P3
  acquire(&ptable.lock);

  initProcessLists();
  initFreeList();

#ifdef CS333_P4
  ptable.PromoteAtTime = ticks + TICKS_TO_PROMOTE;
#endif // CS333_P4

  release(&ptable.lock);
#endif // CS333_P3

  p = allocproc();

  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

#ifdef CS333_P2
  p->uid = UID;
  p->gid = GID;
  p->parent = 0;
#endif // CS333_P2

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  acquire(&ptable.lock);

#ifdef CS333_P4
int rc = stateListRemove(&ptable.list[EMBRYO],p);
  if(rc < 0)
    panic("Error removing from EMBRYO in userinit().\n");
    
  assertState(p,EMBRYO);
  p->state = RUNNABLE;

  stateListAdd(&ptable.ready[MAXPRIO],p);

#elif CS333_P3
  int rc = stateListRemove(&ptable.list[EMBRYO],p);
  if(rc < 0)
    panic("Error removing from EMBRYO in userinit().\n");
    
  assertState(p,EMBRYO);
  p->state = RUNNABLE;

  stateListAdd(&ptable.list[RUNNABLE],p);

#else
  p->state = RUNNABLE;
#endif
  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  struct proc *curproc = myproc();

  sz = curproc->sz;
  if(n > 0){
    if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  curproc->sz = sz;
  switchuvm(curproc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i;
  uint pid;
  struct proc *np;
  struct proc *curproc = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy process state from proc.
  if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;

#ifdef CS333_P3
    acquire(&ptable.lock);
    
    int rc = stateListRemove(&ptable.list[EMBRYO],np);
    if(rc < 0)
      panic("Error removing from EMBRYO in fork().\n");
    
    assertState(np,EMBRYO);
//#ifdef CS333_P4
//    np->budget = DEFAULT_BUDGET;
//#endif // CS333_P4

    np->state = UNUSED;
    stateListAdd(&ptable.list[UNUSED],np);
    
    release(&ptable.lock);
#else
    np->state = UNUSED;
#endif
    return -1;
  }

#ifdef CS333_P2
  np->uid = curproc->uid;
  np->gid = curproc->gid;
#endif // CS333_P2

  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;

  acquire(&ptable.lock);
#ifdef CS333_P4
    int rc = stateListRemove(&ptable.list[EMBRYO],np);
    if(rc < 0)
      panic("Error removing from EMBRYO in fork().\n");
    
    assertState(np,EMBRYO);
    np->state = RUNNABLE;

    stateListAdd(&ptable.ready[MAXPRIO],np);
#elif CS333_P3
    int rc = stateListRemove(&ptable.list[EMBRYO],np);
    if(rc < 0)
      panic("Error removing from EMBRYO in fork().\n");
    
    assertState(np,EMBRYO);
    np->state = RUNNABLE;

    stateListAdd(&ptable.list[RUNNABLE],np);
#else
  np->state = RUNNABLE;
#endif
  release(&ptable.lock);

  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
#ifdef CS333_P4
void
exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int fd;

  if(curproc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd]){
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

  for(int i = 0; i <= MAXPRIO; i++){
    p = ptable.ready[i].head;
    while(p != 0){
      if(p->parent == curproc)
        p->parent = initproc;
        
      p = p->next;
    }
  }

  // Searching RUNNABLE list
  p = ptable.list[RUNNABLE].head;
  while(p != 0){
    if(p->parent == curproc)
      p->parent = initproc;

    p = p->next;
  }

  // Searching RUNNING list 
  p = ptable.list[RUNNING].head;
  while(p != 0){
    if(p->parent == curproc)
      p->parent = initproc;

    p = p->next;
  }

  // Searching ZOMBIE list 

        
  p = ptable.list[ZOMBIE].head;
  while(p != 0){
    if(p->parent == curproc){
      p->parent = initproc;
      wakeup1(initproc);
    }
    p = p->next;
  }

  // Searching SLEEPING list 
  p = ptable.list[SLEEPING].head;
  while(p != 0){
    if(p->parent == curproc)
      p->parent = initproc;

    p = p->next;
  }

  // Searching EMBRYO list
  p = ptable.list[EMBRYO].head;
  while(p != 0){
    if(p->parent == curproc)
      p->parent = initproc;

    p = p->next;
  }

  // Jump into the scheduler, never to return.
  int rc = stateListRemove(&ptable.list[RUNNING], curproc);
  if(rc < 0)
    panic("Error removing from RUNNING in exit().\n");

  assertState(curproc, RUNNING);
  curproc->state = ZOMBIE;
  
  stateListAdd(&ptable.list[ZOMBIE],curproc);

  sched();
  panic("zombie exit");
}
#elif CS333_P3
void
exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int fd;

  if(curproc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd]){
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

  // Searching RUNNABLE list
  p = ptable.list[RUNNABLE].head;
  while(p != 0){
    if(p->parent == curproc)
      p->parent = initproc;

    p = p->next;
  }

  // Searching RUNNING list 
  p = ptable.list[RUNNING].head;
  while(p != 0){
    if(p->parent == curproc)
      p->parent = initproc;

    p = p->next;
  }

  // Searching ZOMBIE list 
  p = ptable.list[ZOMBIE].head;
  while(p != 0){
    if(p->parent == curproc){
      p->parent = initproc;
      wakeup1(initproc);
    }
    p = p->next;
  }

  // Searching SLEEPING list 
  p = ptable.list[SLEEPING].head;
  while(p != 0){
    if(p->parent == curproc)
      p->parent = initproc;

    p = p->next;
  }

  // Searching EMBRYO list
  p = ptable.list[EMBRYO].head;
  while(p != 0){
    if(p->parent == curproc)
      p->parent = initproc;

    p = p->next;
  }

  // Jump into the scheduler, never to return.
  int rc = stateListRemove(&ptable.list[RUNNING], curproc);
  if(rc < 0)
    panic("Error removing from RUNNING in exit().\n");

  assertState(curproc, RUNNING);
  curproc->state = ZOMBIE;
  
  stateListAdd(&ptable.list[ZOMBIE],curproc);

  sched();
  panic("zombie exit");
}
#else // Original Copy of exit()
void
exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int fd;

  if(curproc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd]){
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == curproc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  curproc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}
#endif // End of if/else def for exit()

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
#ifdef CS333_P4
int
wait(void)
{
  struct proc *p;
  int havekids;
  uint pid;
  struct proc *curproc = myproc();

  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;

    // Searching RUNNABLE list 
    p = ptable.list[RUNNABLE].head;
    while(p != 0){// && !havekids){
      if(p->parent == curproc)
      havekids = 1;

      p = p->next;
    }

    // Searching RUNNING list
    p = ptable.list[RUNNING].head;
    while(p != 0){// && !havekids){
     if(p->parent == curproc)
       havekids = 1;

      p = p->next;
    } 

    // Searching SLEEPING list
    p = ptable.list[SLEEPING].head;
    while(p != 0){// && !havekids){
      if(p->parent == curproc)
        havekids = 1;

     p = p->next;
    }

    // Searching EMBRYO list
    p = ptable.list[EMBRYO].head;
    while(p != 0){// && !havekids){
     if(p->parent == curproc)
       havekids = 1;

     p = p->next;
    }
    for(int i = 0; i <= MAXPRIO; i++){
    p = ptable.ready[i].head;

    while(p != 0){
      if(p->parent == curproc)
        havekids = 1;

      p = p->next;
      }
    }
    // Searching ZOMBIE list
    p = ptable.list[ZOMBIE].head;
    while(p != 0){// && !havekids){
      if(p->parent == curproc){
        havekids = 1;
        
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);

        int rc = stateListRemove(&ptable.list[ZOMBIE],p);
        if(rc < 0)
          panic("Error removing from ZOMBIE in wait().\n");

        assertState(p,ZOMBIE);
        p->state = UNUSED;
        stateListAdd(&ptable.list[UNUSED],p);

        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        release(&ptable.lock);
        return pid;
      }
      p = p->next;
    }



    // No point waiting if we don't have any children.
      if(!havekids || curproc->killed){
        release(&ptable.lock);
        return -1;
      }  

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}
#elif CS333_P3
int
wait(void)
{
  struct proc *p;
  int havekids;
  uint pid;
  struct proc *curproc = myproc();

  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;

  // Searching RUNNABLE list 
  p = ptable.list[RUNNABLE].head;
  while(p != 0){// && !havekids){
    if(p->parent == curproc)
      havekids = 1;

    p = p->next;
  }

  // Searching RUNNING list
  p = ptable.list[RUNNING].head;
  while(p != 0){// && !havekids){
    if(p->parent == curproc)
      havekids = 1;

    p = p->next;
  }

  // Searching SLEEPING list
  p = ptable.list[SLEEPING].head;
  while(p != 0){// && !havekids){
    if(p->parent == curproc)
      havekids = 1;

    p = p->next;
  }

  // Searching EMBRYO list
  p = ptable.list[EMBRYO].head;
  while(p != 0){// && !havekids){
    if(p->parent == curproc)
      havekids = 1;

    p = p->next;
  }

// Searching ZOMBIE list
  p = ptable.list[ZOMBIE].head;
  while(p != 0){// && !havekids){
      if(p->parent == curproc){
        havekids = 1;
        
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);

        int rc = stateListRemove(&ptable.list[ZOMBIE],p);
        if(rc < 0)
          panic("Error removing from ZOMBIE in wait().\n");

        assertState(p,ZOMBIE);
        p->state = UNUSED;
        stateListAdd(&ptable.list[UNUSED],p);

        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        release(&ptable.lock);
        return pid;
      }
      p = p->next;
    }

    // No point waiting if we don't have any children.
      if(!havekids || curproc->killed){
        release(&ptable.lock);
        return -1;
      }  

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}
#else // Original Copy of wait()
int
wait(void)
{
  struct proc *p;
  int havekids;
  uint pid;
  struct proc *curproc = myproc();

  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}
#endif // End of if/else def for wait() 
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
#ifdef CS333_P4
void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  c->proc = 0;
#ifdef PDX_XV6
  int idle;  // for checking if processor is idle
#endif // PDX_XV6

  for(;;){
    // Enable interrupts on this processor.
    sti();

#ifdef PDX_XV6
    idle = 1;  // assume idle unless we schedule a process
#endif // PDX_XV6
    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
 
    // Promote All
    if(ticks >= ptable.PromoteAtTime){
     promoteLists();
     ptable.PromoteAtTime = ticks + TICKS_TO_PROMOTE;
     //cprintf("Completed Promotion.\n");
    } // End of Promoting

    for(int i = 0; i <= MAXPRIO; i++){
      p = ptable.ready[i].head;
      
      if(p != 0) {
        // Switch to chosen process.  It is the process's job
        // to release ptable.lock and then reacquire it
        // before jumping back to us.      
#ifdef PDX_XV6
        idle = 0;  // not idle this timeslice
#endif // PDX_XV6
        c->proc = p;
        switchuvm(p);

        p = ptable.ready[i].head;

        //cprintf("PID: %d\tPrio: %d",p->pid,p->priority);
        int rc = stateListRemove(&ptable.ready[i],p);
        if(rc < 0)
          panic("Error removing from ready list in scheduler().\n");

        assertState(p,RUNNABLE);
#ifdef CS333_P2
        p->cpu_ticks_in = ticks;
#endif // CS333_P2
        p->state = RUNNING;
        stateListAdd(&ptable.list[RUNNING],p);

        swtch(&(c->scheduler), p->context);
        switchkvm();

      // Process is done running for now.
      // It should have changed its p->state before coming back.
        c->proc = 0;
      }
    }
    release(&ptable.lock);
#ifdef PDX_XV6
    // if idle, wait for next interrupt
    if (idle) {
      sti();
      hlt();
    }
#endif // PDX_XV6
     //cprintf("Main scheduler for loop.\n");
  }
  //cprintf("Success in Scheduler.\n");
}

#elif CS333_P3
void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  c->proc = 0;
#ifdef PDX_XV6
  int idle;  // for checking if processor is idle
#endif // PDX_XV6

  for(;;){
    // Enable interrupts on this processor.
   
 sti();

#ifdef PDX_XV6
    idle = 1;  // assume idle unless we schedule a process
#endif // PDX_XV6
    // Loop over process table looking for process to run.
    acquire(&ptable.lock);

    p = ptable.list[RUNNABLE].head;

    if(p != 0) {
      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
      int rc = stateListRemove(&ptable.list[RUNNABLE],p);
      if(rc < 0)
        panic("Error removing from RUNNABLE in scheduler().\n");

      assertState(p,RUNNABLE);
#ifdef PDX_XV6
      idle = 0;  // not idle this timeslice
#endif // PDX_XV6
      c->proc = p;
      switchuvm(p);

#ifdef CS333_P2
      p->cpu_ticks_in = ticks;
#endif // CS333_P2

      p->state = RUNNING;
      stateListAdd(&ptable.list[RUNNING],p);

      swtch(&(c->scheduler), p->context);
      switchkvm();

      // Process is done running for now.
      // It should have changed its p->state before coming back.
      c->proc = 0;
    }
    release(&ptable.lock);
#ifdef PDX_XV6
    // if idle, wait for next interrupt
    if (idle) {
      sti();
      hlt();
    }
#endif // PDX_XV6
  }
}
#else // Original Scheduler
void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  c->proc = 0;
#ifdef PDX_XV6
  int idle;  // for checking if processor is idle
#endif // PDX_XV6

  for(;;){
    // Enable interrupts on this processor.
    sti();

#ifdef PDX_XV6
    idle = 1;  // assume idle unless we schedule a process
#endif // PDX_XV6
    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state != RUNNABLE)
        continue;

      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
#ifdef PDX_XV6
      idle = 0;  // not idle this timeslice
#endif // PDX_XV6
      c->proc = p;
      switchuvm(p);

#ifdef CS333_P2
      p->cpu_ticks_in = ticks;
#endif // CS333_P2

      p->state = RUNNING;
      swtch(&(c->scheduler), p->context);
      switchkvm();

      // Process is done running for now.
      // It should have changed its p->state before coming back.
      c->proc = 0;
    }
    release(&ptable.lock);
#ifdef PDX_XV6
    // if idle, wait for next interrupt
    if (idle) {
      sti();
      hlt();
    }
#endif // PDX_XV6
  }
}
#endif // End of if/else def for schedulers

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(mycpu()->ncli != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");

#ifdef CS333_P2
  p->cpu_ticks_total += (ticks - p->cpu_ticks_in);
#endif // CS333_P2

  intena = mycpu()->intena;
  swtch(&p->context, mycpu()->scheduler);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
#ifdef CS333_P4
void
yield(void)
{
  struct proc *curproc = myproc();

  acquire(&ptable.lock);  //DOC: yieldlock

  curproc->budget = curproc->budget - (ticks - curproc->cpu_ticks_in);

  if(curproc->budget <= 0 && curproc->priority > 0 && curproc->priority <= MAXPRIO){
    curproc->priority--;
    curproc->budget = BUDGET;
  }
  release(&ptable.lock);

  acquire(&ptable.lock);
  int rc = stateListRemove(&ptable.list[RUNNING], curproc);
  if(rc < 0)
    panic("Error removing from RUNNING in yield().\n");

  assertState(curproc, RUNNING);
  curproc->state = RUNNABLE;
  stateListAdd(&ptable.ready[curproc->priority],curproc);

  sched();
  release(&ptable.lock);
}
#elif CS333_P3
void
yield(void)
{
  struct proc *curproc = myproc();

  acquire(&ptable.lock);  //DOC: yieldlock

  int rc = stateListRemove(&ptable.list[RUNNING], curproc);
  if(rc < 0)
    panic("Error removing from RUNNING in yield().\n");

  assertState(curproc, RUNNING);
  curproc->state = RUNNABLE;
  stateListAdd(&ptable.list[RUNNABLE],curproc);

  sched();
  release(&ptable.lock);
}
#else // Original yield()
void
yield(void)
{
  struct proc *curproc = myproc();

  acquire(&ptable.lock);  //DOC: yieldlock
  curproc->state = RUNNABLE;
  sched();
  release(&ptable.lock);
}
#endif // End if/else def for yield()
// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
#ifdef CS333_P3
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();

  if(p == 0)
    panic("sleep");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    if (lk) release(lk);
  }
  // Go to sleep.
  p->chan = chan;

#ifdef CS333_P4
  p->budget = p->budget - (ticks - p->cpu_ticks_in);

  if(p->budget <= 0 && p->priority > 0 && p->priority <= MAXPRIO){
    p->priority--;
    p->budget = BUDGET;
  }
#endif // CS333_P4

  int rc = stateListRemove(&ptable.list[RUNNING], p);
  if(rc < 0)
    panic("Error removing from RUNNING in sleep().\n");

  assertState(p, RUNNING);
  p->state = SLEEPING;
  stateListAdd(&ptable.list[SLEEPING],p);

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    if (lk) acquire(lk);
  }
}
#else // Original sleep()
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();

  if(p == 0)
    panic("sleep");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    if (lk) release(lk);
  }
  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    if (lk) acquire(lk);
  }
}
#endif // End if/else def for sleep()

// Wake up all processes sleeping on chan.
// The ptable lock must be held.
#ifdef CS333_P4
static void
wakeup1(void *chan)
{
  struct proc *p;
  p = ptable.list[SLEEPING].head;

  while(p != 0){
    if(p->chan == chan){
      int rc = stateListRemove(&ptable.list[SLEEPING], p);
      if(rc < 0)
         panic("Error removing from SLEEPING in wakeup1().\n");

      assertState(p, SLEEPING);
      p->state = RUNNABLE;
      stateListAdd(&ptable.ready[p->priority],p);
      }
      p = p->next;
  }

}
#elif CS333_P3
static void
wakeup1(void *chan)
{
  struct proc *p;
  p = ptable.list[SLEEPING].head;

  while(p != 0){
    if(p->chan == chan){
      int rc = stateListRemove(&ptable.list[SLEEPING], p);
      if(rc < 0)
         panic("Error removing from SLEEPING in wakeup1().\n");

      assertState(p, SLEEPING);
      p->state = RUNNABLE;
      stateListAdd(&ptable.list[RUNNABLE],p);
      }
      p = p->next;
  }

}
#else // Original wakeup1()
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan)
      p->state = RUNNABLE;
}
#endif // End of if/else def for wakeup1()

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
#ifdef CS333_P4
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  // Searching RUNNABLE list
  p = ptable.list[RUNNABLE].head;
  while(p != 0){
    if(p->pid == pid){
      p->killed = 1;
      release(&ptable.lock);
      return 0;
      }
    p = p->next;
    }
    for(int i = 0; i <= MAXPRIO; i++){
      p = ptable.ready[i].head;

      while(p != 0){
        if(p->pid == pid){
         p->killed = 1;
         release(&ptable.lock);
         return 0;
        }
        p = p->next;
      }
    }

  // Searching RUNNING list
  p = ptable.list[RUNNING].head;
  while(p != 0){
    if(p->pid == pid){
      p->killed = 1;
      release(&ptable.lock);
      return 0;
    }
    p = p->next;
  }

  // Searching ZOMBIE list
  p = ptable.list[ZOMBIE].head;
  while(p != 0){
    if(p->pid == pid){
      p->killed = 1;
      release(&ptable.lock);
      return 0;
    }
    p = p->next;
  }

  // Searching SLEEPING list
  p = ptable.list[SLEEPING].head;
  while(p != 0){
    if(p->pid == pid){
      p->killed = 1;

      int rc = stateListRemove(&ptable.list[SLEEPING], p);
      if(rc < 0)
         panic("Error removing from SLEEPING in kill().\n");

      assertState(p, SLEEPING);
      p->state = RUNNABLE;
      stateListAdd(&ptable.ready[p->priority],p);
      release(&ptable.lock);

      return 0;
      }
    
    p = p->next;
  }

  // Searching EMBRYO list
  p = ptable.list[EMBRYO].head;
  while(p != 0){
    if(p->pid == pid){
      p->killed = 1;
      release(&ptable.lock);
      return 0;
    }
    p = p->next;
  }
 
  release(&ptable.lock);
  return -1;
}
#elif CS333_P3
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  // Searching RUNNABLE list
  p = ptable.list[RUNNABLE].head;
  while(p != 0){
    if(p->pid == pid){
      p->killed = 1;
      release(&ptable.lock);
      return 0;
      }
    p = p->next;
    }

  // Searching RUNNING list
  p = ptable.list[RUNNING].head;
  while(p != 0){
    if(p->pid == pid){
      p->killed = 1;
      release(&ptable.lock);
      return 0;
    }
    p = p->next;
  }

  // Searching ZOMBIE list
  p = ptable.list[ZOMBIE].head;
  while(p != 0){
    if(p->pid == pid){
      p->killed = 1;
      release(&ptable.lock);
      return 0;
    }
    p = p->next;
  }

  // Searching SLEEPING list
  p = ptable.list[SLEEPING].head;
  while(p != 0){
    if(p->pid == pid){
      p->killed = 1;

      int rc = stateListRemove(&ptable.list[SLEEPING], p);
      if(rc < 0)
         panic("Error removing from SLEEPING in kill().\n");

      assertState(p, SLEEPING);
      p->state = RUNNABLE;
      stateListAdd(&ptable.list[RUNNABLE],p);
      release(&ptable.lock);

      return 0;
      }
    
    p = p->next;
  }

  // Searching EMBRYO list
  p = ptable.list[EMBRYO].head;
  while(p != 0){
    if(p->pid == pid){
      p->killed = 1;
      release(&ptable.lock);
      return 0;
    }
    p = p->next;
  }
 
  release(&ptable.lock);
  return -1;
}
#else // Original kill()
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
        p->state = RUNNABLE;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}
#endif // End of if/else for kill()

// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.

// Helper functions
#ifdef CS333_P4
void procdumpP4(struct proc *,char*);
#endif // CS333_P4
//#ifdef CS333_P3
//void procdumpP3(struct proc *,char*);
//#endif // CS333_P3
#ifdef CS333_P2
void procdumpP2(struct proc *,char*);
#endif // CS333_P2
#ifdef CS333_P1
void procdumpP1(struct proc *,char*);
#endif // CS333_P1

void
procdump(void)
{
  int i;
  struct proc *p;
  char *state;
  uint pc[10];
#if defined(CS333_P4)
#define HEADER "\nPID\tName\t	UID\tGID\tPPID\tPrio\tElapsed\tCPU\tState\tSize\t PCs\n"
//#elif defined(CS333_P3)
//#define HEADER "\nPID\tName	UID\tGID\tPPID\tElapsed\tCPU\tState\tSize\t PCs\n"
#elif defined(CS333_P2)
#define HEADER "\nPID\tName	UID\tGID\tPPID\tElapsed\tCPU\tState\tSize\t PCs\n"
#elif defined(CS333_P1)
#define HEADER "\nPID\tName\tElapsed\tState\tSize\t PCs\n"
#else
#define HEADER "\n"
#endif

  cprintf(HEADER);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";

#if defined(CS333_P4)
    procdumpP4(p,state);
//#elif defined(CS333_P3)
//    procdumpP3(p,state);
#elif defined(CS333_P2)
    procdumpP2(p,state);
#elif defined(CS333_P1)
    procdumpP1(p,state);
#else
    cprintf("%d\t%s\t%s\t", p->pid,p->name,state);
#endif

    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}
#ifdef CS333_P4
void
procdumpP4(struct proc *p,char *state){
  uint s;
  uint ms;
  
  // Print PID, Name, UID, GID, PPID, PRIO
  cprintf("%d\t%s\t\t%d\t%d\t%d\t%d\t",p->pid,p->name,p->uid,p->gid,p->parent ? p->parent->pid : p->pid,p->priority);

  // Calculate and print Elapsed Time
  s  = (ticks - p->start_ticks) / 1000;
  ms = (ticks - p->start_ticks) % 1000;

  if(ms >= 100)
     cprintf("%d.%d\t",s,ms);
  else if(ms < 11)
     cprintf("%d.00%d\t",s,ms);
  else
     cprintf("%d.0%d\t",s,ms);

  // Calculate and print CPU Time
  s  = p->cpu_ticks_total / 1000;
  ms = p->cpu_ticks_total % 1000;

  if(ms >= 100)
     cprintf("%d.%d\t",s,ms);
  else if(ms < 10)
     cprintf("%d.00%d\t",s,ms);
  else
     cprintf("%d.0%d\t",s,ms);

  // Print State, Size
  cprintf("%s\t%d\t",state,p->sz);
}

int
setpriority(int pid, int priority)
{
  struct proc *p;

  if(pid < 0 || priority < 0 || priority > MAXPRIO)
    return -1;
 
  acquire(&ptable.lock);
  for(int i = 0; i < MAXPRIO; i++){
    p = ptable.ready[i].head;
    
    while(p != 0){
      if(p->pid == pid){
        p->priority = priority;
        p->budget = BUDGET;

        if(priority != i){
          int rc = stateListRemove(&ptable.ready[i], p);
          if(rc < 0)
            panic("Error removing from ready list in setpriority().\n");

          assertState(p, RUNNABLE);
          p->state = RUNNABLE;
          stateListAdd(&ptable.ready[priority+1],p);
          //stateListAdd(&ptable.ready[priority],p);
        }
        release(&ptable.lock);
        return 0;
      }
      p = p->next;
    }
  }

  p = ptable.list[RUNNING].head;
  while(p != 0){
    if(p->pid == pid){
      p->priority = priority;
      p->budget = BUDGET;
      release(&ptable.lock);
      
      return 0;
    }
    p = p->next;
  }

  p = ptable.list[SLEEPING].head;
  while(p != 0){
    if(p->pid == pid){
      p->priority = priority;
      p->budget = BUDGET;
      release(&ptable.lock);
      
      return 0;
    }
    p = p->next;
  }

  p = ptable.list[ZOMBIE].head;
  while(p != 0){
    if(p->pid == pid){
      p->priority = priority;
      p->budget = BUDGET;
      release(&ptable.lock);
      
      return 0;
    }
    p = p->next;
  }

  p = ptable.list[EMBRYO].head;
  while(p != 0){
    if(p->pid == pid){
      p->priority = priority;
      p->budget = BUDGET;
      release(&ptable.lock);
      
      return 0;
    }
    p = p->next;
  }
  release(&ptable.lock);
  return -1;
}

int
getpriority(int pid)
{
  struct proc *p;
  int priority;

  acquire(&ptable.lock);
  // Search through RUNNING 
  p = ptable.list[RUNNING].head;
  while(p != 0){
    if(p->pid == pid){
      priority = p->priority;
      release(&ptable.lock);
      return priority;
    }
    p = p->next;
  }
  // Search through SLEEPING
  p = ptable.list[SLEEPING].head;
  while(p != 0){
    if(p->pid == pid){
      priority = p->priority;
      release(&ptable.lock);
      return priority;
    }
    p = p->next;
  }

  // Search through ready lists
  for(int i = 0; i<= MAXPRIO; i++){
    p = ptable.ready[i].head;
    while(p != 0){
      if(p->pid == pid){
        priority = p->priority;
        release(&ptable.lock);
        return priority;
      }
      p = p->next;
    }
  }
  release(&ptable.lock);
  return -1;
}

void
promoteLists(void)
{
  struct proc *p;

  p = ptable.list[RUNNING].head;
    while(p != 0){
      if(p->priority < MAXPRIO && p->priority >=0){
        p->priority++;
        p->budget = BUDGET;
      }
      p = p->next;
    }

    p = ptable.list[SLEEPING].head;
    while(p != 0){
      if(p->priority < MAXPRIO && p->priority >= 0){
        p->priority++;
        p->budget = BUDGET;
      }
      p = p->next;
    }

    for(int i = 0; i < MAXPRIO; i++){
      p = ptable.ready[i].head;
      while(p != 0){
        int rc = stateListRemove(&ptable.ready[i], p);
        if(rc < 0)
          panic("Error removing from ready list in Scheduler().\n");

        assertState(p, RUNNABLE);
        p->state = RUNNABLE;
        stateListAdd(&ptable.ready[i+1],p);

        p->priority++;
        p->budget = BUDGET;
        p = p->next;
      }
    }
}
#endif // CS333_P4

#ifdef CS333_P4
void
infodump(char input)
{
  struct proc* p;
  int ppid = 0; 
  int num_free = 0;

  switch(input)
  { 
    // Runnable info dump
    case 'r':
      acquire(&ptable.lock);
      cprintf("Ready List Processes: \n");
    
    //if(p == 0)
    //    cprintf("Ready List Empty\n");

      for(int i = MAXPRIO; i >= 0; i--){
        p = ptable.ready[i].head;
      
        cprintf("%d: ",i);

        while(p != 0){
          cprintf("(%d, %d)",p->pid,p->budget);

          if(p->next)  
            cprintf(" -> ");
          else
            cprintf("\n");

        p = p->next;
        }
      cprintf("\n");
      }
      release(&ptable.lock);
      break;

    // Free/Unused info dump
    case 'f':
      acquire(&ptable.lock);
      p = ptable.list[UNUSED].head;
      if(p == 0)
        cprintf("Free List Empty.\n");

      while(p != 0){
        assertState(p,UNUSED);
        ++num_free;
        p = p->next;
      }
      if(num_free == 1)
        cprintf("Free List Size: 1 process.\n");
      if(num_free > 1)
        cprintf("Free List Size: %d processes.\n", num_free);

      release(&ptable.lock);
      break;

    // Sleeping info dump
    case 's':
      acquire(&ptable.lock);
      p = ptable.list[SLEEPING].head;
      cprintf("Sleep List Processes: \n");
      if(p == 0)
        cprintf("Sleep List Empty\n");

      while(p != 0){
        assertState(p,SLEEPING);
        cprintf("%d ", p->pid);
        if(p->next)
          cprintf("-> ");

        p = p->next;
      }
      cprintf("\n");
      release(&ptable.lock);
      break;
    
    // Zombie info dump
    case 'z':
      acquire(&ptable.lock);
      p = ptable.list[ZOMBIE].head;
      if(p == 0)
        cprintf("Zombie List Empty.\n");
      else
        cprintf("Zombie List Processes: \n");

      while(p != 0){
        assertState(p,ZOMBIE);
        if(p->parent)
          ppid = p->parent->pid;
        else
          ppid = p->pid;
        cprintf("(PID%d,PPID%d) ",p->pid,ppid);

        if(p->next)
          cprintf("-> ");
      
        p = p->next;
      }
      cprintf("\n");
      release(&ptable.lock);
      break;

    // Default
    default: 
      cprintf("Invalid Input in info dump\n");
  }
}
#elif CS333_P3
void
infodump(char input)
{
  struct proc* p;
  int ppid = 0; 
  int num_free = 0;

  switch(input)
  { 
    // Runnable info dump
    case 'r':
      acquire(&ptable.lock);
      p = ptable.list[RUNNABLE].head;
      cprintf("Ready List Processes: \n");
      if(p == 0)
        cprintf("Ready List Empty\n");

      while(p != 0){
        assertState(p,RUNNABLE);
        cprintf("%d ",p->pid);

        if(p->next)  
          cprintf("-> ");

        p = p->next;
      }
      cprintf("\n");
      release(&ptable.lock);
      break;

    // Free/Unused info dump
    case 'f':
      acquire(&ptable.lock);
      p = ptable.list[UNUSED].head;
      if(p == 0)
        cprintf("Free List Empty.\n");

      while(p != 0){
        assertState(p,UNUSED);
        ++num_free;
        p = p->next;
      }
      if(num_free == 1)
        cprintf("Free List Size: 1 process.\n");
      if(num_free > 1)
        cprintf("Free List Size: %d processes.\n", num_free);

      release(&ptable.lock);
      break;

    // Sleeping info dump
    case 's':
      acquire(&ptable.lock);
      p = ptable.list[SLEEPING].head;
      cprintf("Sleep List Processes: \n");
      if(p == 0)
        cprintf("Sleep List Empty\n");

      while(p != 0){
        assertState(p,SLEEPING);
        cprintf("%d ", p->pid);
        if(p->next)
          cprintf("-> ");

        p = p->next;
      }
      cprintf("\n");
      release(&ptable.lock);
      break;
    
    // Zombie info dump
    case 'z':
      acquire(&ptable.lock);
      p = ptable.list[ZOMBIE].head;
      if(p == 0)
        cprintf("Zombie List Empty.\n");
      else
        cprintf("Zombie List Processes: \n");

      while(p != 0){
        assertState(p,ZOMBIE);
        if(p->parent)
          ppid = p->parent->pid;
        else
          ppid = p->pid;
        cprintf("(PID%d,PPID%d) ",p->pid,ppid);

        if(p->next)
          cprintf("-> ");
      
        p = p->next;
      }
      cprintf("\n");
      release(&ptable.lock);
      break;

    // Default
    default: 
      cprintf("Invalid Input in info dump\n");
  }
}
#endif // End of if/else for infodump

#ifdef CS333_P3
static void
stateListAdd(struct ptrs* list, struct proc* p)
{
  if((*list).head == NULL){
    (*list).head = p;
    (*list).tail = p;
    p->next = NULL;
  } else {
    ((*list).tail)->next = p;
    (*list).tail = ((*list).tail)->next;
    ((*list).tail)->next = NULL;
  }
}

static int 
stateListRemove(struct ptrs* list, struct proc* p)
{
  if((*list).head == NULL || (*list).tail == NULL || p == NULL){
    return -1;
  }

  struct proc* current = (*list).head;
  struct proc* previous = 0;

  if(current == p){
    (*list).head = ((*list).head)->next;
    // prevent tail remaining assigned when we've removed the only item
    // on the list
    if((*list).tail == p){
      (*list).tail = NULL;
    }
    return 0;
  }

  while(current){
    if(current == p){
      break;
    }

    previous = current;
    current = current->next;
  }

  // Process not found, hit eject.
  if(current == NULL){
    return -1;
  }

  // Process found. Set the appropriate next pointer.
  if(current == (*list).tail){
    (*list).tail = previous;
    ((*list).tail)->next = NULL;
  } else {
    previous->next = current->next;
  }

  // Make sure p->next doesn't point into the list.
  p->next = NULL;

  return 0;
}

static void
initProcessLists()
{
  int i;

  for(i = UNUSED; i<= ZOMBIE; i++){
    ptable.list[i].head = NULL;
    ptable.list[i].tail = NULL;
  }
#ifdef CS333_P4
  for(i = 0; i <= MAXPRIO; i++){
    ptable.ready[i].head = NULL;
    ptable.ready[i].tail = NULL;
  }
#endif // CS333_P4
}

static void
initFreeList(void)
{
  struct proc* p;

  for(p = ptable.proc; p < ptable.proc + NPROC; ++p){
    p->state = UNUSED;
    stateListAdd(&ptable.list[UNUSED], p);
  }
}

static void
assertState(struct proc* p, enum procstate state)
{
  if(p->state != state){
    cprintf("p->state is in %d, when it needs to be in %d\n",states[p->state], states[state]);
    panic("assertState() failed.\n");
  }
}
#endif // CS333_P3

#ifdef CS333_P2
void
procdumpP2(struct proc *p,char *state)
{
  uint s;
  uint ms;
  
  // Print PID, Name, UID, GID, PPID,
  cprintf("%d\t%s\t%d\t%d\t%d\t",p->pid,p->name,p->uid,p->gid,p->parent ? p->parent->pid : p->pid);

  // Calculate and print Elapsed Time
  s  = (ticks - p->start_ticks) / 1000;
  ms = (ticks - p->start_ticks) % 1000;

  if(ms >= 100)
     cprintf("%d.%d\t",s,ms);
  else if(ms < 11)
     cprintf("%d.00%d\t",s,ms);
  else
     cprintf("%d.0%d\t",s,ms);

  // Calculate and print CPU Time
  s  = p->cpu_ticks_total / 1000;
  ms = p->cpu_ticks_total % 1000;

  if(ms >= 100)
     cprintf("%d.%d\t",s,ms);
  else if(ms < 10)
     cprintf("%d.00%d\t",s,ms);
  else
     cprintf("%d.0%d\t",s,ms);

  // Print State, Size
  cprintf("%s\t%d\t",state,p->sz);
}

int
getprocs(uint max, struct uproc* table)
{
  struct proc* p;
  int proc_num = 0;

  acquire(&ptable.lock);
  
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(proc_num == max){
      break;
      }
   

    if(p->state != UNUSED && p->state != EMBRYO){
      table[proc_num].pid = p->pid;
      table[proc_num].uid = p->uid;
      table[proc_num].gid = p->gid;
      table[proc_num].ppid = p->parent ? p->parent->pid : p->pid;
      table[proc_num].elapsed_ticks = (ticks-p->start_ticks);
      table[proc_num].CPU_total_ticks = p->cpu_ticks_total;
      table[proc_num].size = p->sz;
      safestrcpy(table[proc_num].state,states[p->state],STRMAX);
      safestrcpy(table[proc_num].name,p->name,STRMAX);
#ifdef CS333_P4
      table[proc_num].priority = p->priority;
#endif // CS333_P4

      proc_num++;
    }
  }

  release(&ptable.lock);

  if(proc_num == 0)
    return -1;

  return proc_num;
}
#endif //CS333_P2


#ifdef CS333_P1
// Helper function to display correct information for Project 1
void
procdumpP1(struct proc *p,char *state)
{
  uint s;
  uint ms;
  uint size;
   	
  s  = (ticks - p->start_ticks) / 1000;
  ms = (ticks - p->start_ticks) % 1000;
  size = p->sz;
  if(ms >= 100){
    cprintf("%d\t%s\t%d.%d\t%s\t%d\t%",p->pid,p->name,s,ms,state,size);
  }else if(ms < 10){
    cprintf("%d\t%s\t%d.00%d\t%s\t%d\t%",p->pid,p->name,s,ms,state,size);
  }else{
    cprintf("%d\t%s\t%d.0%d\t%s\t%d\t%",p->pid,p->name,s,ms,state,size);
  }
}
#endif // CS333_P1
