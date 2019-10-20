#include <proc.h>
#include <kmalloc.h>
#include <string.h>
#include <sync.h>
#include <pmm.h>
#include <error.h>
#include <sched.h>
#include <elf.h>
#include <vmm.h>
#include <trap.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <clock.h>

/* ------------- process/thread mechanism design&implementation -------------
(an simplified Linux process/thread mechanism )
introduction:
  ucore implements a simple process/thread mechanism. process contains the independent memory sapce, at least one threads
for execution, the kernel data(for management), processor state (for context switch), files(in lab6), etc. ucore needs to
manage all these details efficiently. In ucore, a thread is just a special kind of process(share process's memory).
------------------------------
process state       :     meaning               -- reason
    PROC_UNINIT     :   uninitialized           -- alloc_proc
    PROC_SLEEPING   :   sleeping                -- try_free_pages, do_wait, do_sleep
    PROC_RUNNABLE   :   runnable(maybe running) -- proc_init, wakeup_proc, 
    PROC_ZOMBIE     :   almost dead             -- do_exit

-----------------------------
process state changing:
                                            
  alloc_proc                                 RUNNING
      +                                   +--<----<--+
      +                                   + proc_run +
      V                                   +-->---->--+ 
PROC_UNINIT -- proc_init/wakeup_proc --> PROC_RUNNABLE -- try_free_pages/do_wait/do_sleep --> PROC_SLEEPING --
                                           A      +                                                           +
                                           |      +--- do_exit --> PROC_ZOMBIE                                +
                                           +                                                                  + 
                                           -----------------------wakeup_proc----------------------------------
-----------------------------
process relations
parent:           proc->parent  (proc is children)
children:         proc->cptr    (proc is parent)
older sibling:    proc->optr    (proc is younger sibling)
younger sibling:  proc->yptr    (proc is older sibling)
-----------------------------
related syscall for process:
SYS_exit        : process exit,                           -->do_exit
SYS_fork        : create child process, dup mm            -->do_fork-->wakeup_proc
SYS_wait        : wait process                            -->do_wait
SYS_exec        : after fork, process execute a program   -->load a program and refresh the mm
SYS_clone       : create child thread                     -->do_fork-->wakeup_proc
SYS_yield       : process flag itself need resecheduling, -- proc->need_sched=1, then scheduler will rescheule this process
SYS_sleep       : process sleep                           -->do_sleep 
SYS_kill        : kill process                            -->do_kill-->proc->flags |= PF_EXITING
                                                                 -->wakeup_proc-->do_wait-->do_exit   
SYS_getpid      : get the process's pid

*/
// the process set's list
list_entry_t proc_list;

#define HASH_SHIFT          10
#define HASH_LIST_SIZE      (1 << HASH_SHIFT)
#define pid_hashfn(x)       (hash32(x, HASH_SHIFT))

// has list for process set based on pid
static list_entry_t hash_list[HASH_LIST_SIZE];

// idle proc
struct proc_struct *idleproc = NULL;
// init proc
struct proc_struct *initproc = NULL;
// current proc
struct proc_struct *current = NULL;

static int nr_process = 0;

void kernel_thread_entry(void);
void forkrets(struct trapframe *tf);
void switch_to(struct context *from, struct context *to);

// alloc_proc - alloc a proc_struct and init all fields of proc_struct
static struct proc_struct *
alloc_proc(void) {
    struct proc_struct *proc = kmalloc(sizeof(struct proc_struct));
    if (proc != NULL) {
    //LAB4:EXERCISE1 YOUR CODE
    /*
     * below fields in proc_struct need to be initialized
     *       enum proc_state state;                      // Process state
     *       int pid;                                    // Process ID
     *       int runs;                                   // the running times of Proces
     *       uintptr_t kstack;                           // Process kernel stack
     *       volatile bool need_resched;                 // bool value: need to be rescheduled to release CPU?
     *       struct proc_struct *parent;                 // the parent process
     *       struct mm_struct *mm;                       // Process's memory management field
     *       struct context context;                     // Switch here to run process
     *       struct trapframe *tf;                       // Trap frame for current interrupt
     *       uintptr_t cr3;                              // CR3 register: the base addr of Page Directroy Table(PDT)
     *       uint32_t flags;                             // Process flag
     *       char name[PROC_NAME_LEN + 1];               // Process name
     */
     //LAB5 YOUR CODE : (update LAB4 steps)
    /*
     * below fields(add in LAB5) in proc_struct need to be initialized	
     *       uint32_t wait_state;                        // waiting state
     *       struct proc_struct *cptr, *yptr, *optr;     // relations between processes
	 */
        proc->state = PROC_UNINIT;
        proc->pid = -1;
        proc->runs = 0;
        proc->kstack = 0;
        proc->need_resched = 0;
        proc->parent = NULL;
        proc->mm = NULL;
        memset(&(proc->context), 0, sizeof(struct context));
        proc->tf = NULL;
        proc->cr3 = boot_cr3;
        proc->flags = 0;
        memset(proc->name, 0, PROC_NAME_LEN);
        proc->wait_state = 0;
        proc->cptr = proc->optr = proc->yptr = NULL;
        proc->rq = NULL;
        list_init(&(proc->run_link));
        list_init(&proc->part_link);
        proc->time_slice = 0;
        // proc->lab6_run_pool.left = proc->lab6_run_pool.right = proc->lab6_run_pool.parent = NULL;
        // proc->lab6_stride = 0;
        // proc->lab6_priority = 0;
        proc->timeout_deadline = 0;
        proc_state(proc) = DORMANT;
        proc_timecapa(proc) = 3;
        proc_baseproi(proc) = 1;
        proc_prio(proc) = proc_baseproi(proc);
        proc->part = NULL;
        proc->time_slice = proc_timecapa(proc);
        proc->timer = NULL;
        proc->ustack = 0;
    }
    return proc;
}

// set_proc_name - set the name of proc
char *
set_proc_name(struct proc_struct *proc, const char *name) {
    memset(proc->name, 0, sizeof(proc->name));
    return memcpy(proc->name, name, PROC_NAME_LEN);
}

// get_proc_name - get the name of proc
char *
get_proc_name(struct proc_struct *proc) {
    static char name[PROC_NAME_LEN + 1];
    memset(name, 0, sizeof(name));
    return memcpy(name, proc->name, PROC_NAME_LEN);
}

// set_links - set the relation links of process
static void
set_links(struct proc_struct *proc) {
    list_add(&proc_list, &(proc->list_link));
//    proc->yptr = NULL;
//    if ((proc->optr = proc->parent->cptr) != NULL) {
//        proc->optr->yptr = proc;
//    }
//    proc->parent->cptr = proc;
    nr_process ++;
}

// remove_links - clean the relation links of process
static void
remove_links(struct proc_struct *proc) {
    list_del(&(proc->list_link));
    if (proc->optr != NULL) {
        proc->optr->yptr = proc->yptr;
    }
    if (proc->yptr != NULL) {
        proc->yptr->optr = proc->optr;
    }
    else {
       proc->parent->cptr = proc->optr;
    }
    nr_process --;
}

// get_pid - alloc a unique pid for process
static int
get_pid(void) {
    static_assert(MAX_PID > MAX_PROCESS);
    struct proc_struct *proc;
    list_entry_t *list = &proc_list, *le;
    static int next_safe = MAX_PID, last_pid = MAX_PID;
    if (++ last_pid >= MAX_PID) {
        last_pid = 1;
        goto inside;
    }
    if (last_pid >= next_safe) {
    inside:
        next_safe = MAX_PID;
    repeat:
        le = list;
        while ((le = list_next(le)) != list) {
            proc = le2proc(le, list_link);
            if (proc->pid == last_pid) {
                if (++ last_pid >= next_safe) {
                    if (last_pid >= MAX_PID) {
                        last_pid = 1;
                    }
                    next_safe = MAX_PID;
                    goto repeat;
                }
            }
            else if (proc->pid > last_pid && next_safe > proc->pid) {
                next_safe = proc->pid;
            }
        }
    }
    return last_pid;
}

// proc_run - make process "proc" running on cpu
// NOTE: before call switch_to, should load  base addr of "proc"'s new PDT
void
proc_run(struct proc_struct *proc) {
    if (proc != current) {
        bool intr_flag;
        struct proc_struct *prev = current, *next = proc;
        local_intr_save(intr_flag);
        {
            current = proc;
            load_esp0(next->kstack + KSTACKSIZE);
            lcr3(next->cr3);
            switch_to(&(prev->context), &(next->context));
        }
        local_intr_restore(intr_flag);
    }
}

// forkret -- the first kernel entry point of a new thread/process
// NOTE: the addr of forkret is setted in copy_thread function
//       after switch_to, the current proc will execute here.
static void
forkret(void) {
    forkrets(current->tf);
}

// hash_proc - add proc into proc hash_list
static void
hash_proc(struct proc_struct *proc) {
    list_add(hash_list + pid_hashfn(proc->pid), &(proc->hash_link));
}

// unhash_proc - delete proc from proc hash_list
static void
unhash_proc(struct proc_struct *proc) {
    list_del(&(proc->hash_link));
}

// find_proc - find proc frome proc hash_list according to pid
struct proc_struct *
find_proc(int pid) {
    if (0 < pid && pid < MAX_PID) {
        list_entry_t *list = hash_list + pid_hashfn(pid), *le = list;
        while ((le = list_next(le)) != list) {
            struct proc_struct *proc = le2proc(le, hash_link);
            if (proc->pid == pid) {
                return proc;
            }
        }
    }
    return NULL;
}

// kernel_thread - create a kernel thread using "fn" function
// NOTE: the contents of temp trapframe tf will be copied to 
//       proc->tf in do_fork-->copy_thread function
int
kernel_thread(int (*fn)(void *), void *arg, uint32_t clone_flags) {
    struct trapframe tf;
    memset(&tf, 0, sizeof(struct trapframe));
    tf.tf_cs = KERNEL_CS;
    tf.tf_ds = tf.tf_es = tf.tf_ss = KERNEL_DS;
    tf.tf_regs.reg_ebx = (uint32_t)fn;
    tf.tf_regs.reg_edx = (uint32_t)arg;
    tf.tf_eip = (uint32_t)kernel_thread_entry;
    return do_fork(clone_flags | CLONE_VM, 0, &tf);
}

// setup_kstack - alloc pages with size KSTACKPAGE as process kernel stack
static int
setup_kstack(struct proc_struct *proc) {
    struct Page *page = alloc_pages(KSTACKPAGE);
    if (page != NULL) {
        proc->kstack = (uintptr_t)page2kva(page);
        return 0;
    }
    return -E_NO_MEM;
}

// put_kstack - free the memory space of process kernel stack
static void
put_kstack(struct proc_struct *proc) {
    free_pages(kva2page((void *)(proc->kstack)), KSTACKPAGE);
}

// setup_pgdir - alloc one page as PDT
static int
setup_pgdir(struct mm_struct *mm) {
    struct Page *page;
    if ((page = alloc_page()) == NULL) {
        return -E_NO_MEM;
    }
    pde_t *pgdir = page2kva(page);
    memcpy(pgdir, boot_pgdir, PGSIZE);
    pgdir[PDX(VPT)] = PADDR(pgdir) | PTE_P | PTE_W;
    mm->pgdir = pgdir;
    return 0;
}

// put_pgdir - free the memory space of PDT
static void
put_pgdir(struct mm_struct *mm) {
    free_page(kva2page(mm->pgdir));
}

// copy_mm - process "proc" duplicate OR share process "current"'s mm according clone_flags
//         - if clone_flags & CLONE_VM, then "share" ; else "duplicate"
static int
copy_mm(uint32_t clone_flags, struct proc_struct *proc) {
    struct mm_struct *mm, *oldmm = current->mm;

    /* current is a kernel thread */
    if (oldmm == NULL) {
        return 0;
    }
    if (clone_flags & CLONE_VM) {
        mm = oldmm;
        goto good_mm;
    }

    int ret = -E_NO_MEM;
    if ((mm = mm_create()) == NULL) {
        goto bad_mm;
    }
    if (setup_pgdir(mm) != 0) {
        goto bad_pgdir_cleanup_mm;
    }

    lock_mm(oldmm);
    {
        ret = dup_mmap(mm, oldmm);
    }
    unlock_mm(oldmm);

    if (ret != 0) {
        goto bad_dup_cleanup_mmap;
    }

good_mm:
    mm_count_inc(mm);
    proc->mm = mm;
    proc->cr3 = PADDR(mm->pgdir);
    return 0;
bad_dup_cleanup_mmap:
    exit_mmap(mm);
    put_pgdir(mm);
bad_pgdir_cleanup_mm:
    mm_destroy(mm);
bad_mm:
    return ret;
}

// copy_thread - setup the trapframe on the  process's kernel stack top and
//             - setup the kernel entry point and stack of process
static void
copy_thread(struct proc_struct *proc, uintptr_t esp, struct trapframe *tf) {
    proc->tf = (struct trapframe *)(proc->kstack + KSTACKSIZE) - 1;
    *(proc->tf) = *tf;
    proc->tf->tf_regs.reg_eax = 0;
    proc->tf->tf_esp = esp;
    proc->tf->tf_eflags |= FL_IF;

    proc->context.eip = (uintptr_t)forkret;
    proc->context.esp = (uintptr_t)(proc->tf);
}

/* do_fork -     parent process for a new child process
 * @clone_flags: used to guide how to clone the child process
 * @stack:       the parent's user stack pointer. if stack==0, It means to fork a kernel thread.
 * @tf:          the trapframe info, which will be copied to child process's proc->tf
 */
int
do_fork(uint32_t clone_flags, uintptr_t stack, struct trapframe *tf) {
    int ret = -E_NO_FREE_PROC;
    struct proc_struct *proc;
    if (nr_process >= MAX_PROCESS) {
        goto fork_out;
    }
    ret = -E_NO_MEM;
    //LAB4:EXERCISE2 YOUR CODE
    /*
     * Some Useful MACROs, Functions and DEFINEs, you can use them in below implementation.
     * MACROs or Functions:
     *   alloc_proc:   create a proc struct and init fields (lab4:exercise1)
     *   setup_kstack: alloc pages with size KSTACKPAGE as process kernel stack
     *   copy_mm:      process "proc" duplicate OR share process "current"'s mm according clone_flags
     *                 if clone_flags & CLONE_VM, then "share" ; else "duplicate"
     *   copy_thread:  setup the trapframe on the  process's kernel stack top and
     *                 setup the kernel entry point and stack of process
     *   hash_proc:    add proc into proc hash_list
     *   get_pid:      alloc a unique pid fprocor process
     *   wakeup_proc:  set proc->state = PROC_RUNNABLE
     * VARIABLES:
     *   proc_list:    the process set's list
     *   nr_process:   the number of process set
     */

    //    1. call alloc_proc to allocate a proc_struct
    //    2. call setup_kstack to allocate a kernel stack for child process
    //    3. call copy_mm to dup OR share mm according clone_flag
    //    4. call copy_thread to setup tf & context in proc_struct
    //    5. insert proc_struct into hash_list && proc_list
    //    6. call wakeup_proc to make the new child process RUNNABLE
    //    7. set ret vaule using child proc's pid

	//LAB5 YOUR CODE : (update LAB4 steps)
   /* Some Functions
    *    set_links:  set the relation links of process.  ALSO SEE: remove_links:  lean the relation links of process 
    *    -------------------
	*    update step 1: set child proc's parent to current process, make sure current process's wait_state is 0
	*    update step 5: insert proc_struct into hash_list && proc_list, set the relation links of process
    */
    if ((proc = alloc_proc()) == NULL) {
        goto fork_out;
    }

    proc->parent = current;
    assert(current->wait_state == 0);

    if (setup_kstack(proc) != 0) {
        goto bad_fork_cleanup_proc;
    }
    if (copy_mm(clone_flags, proc) != 0) {
        goto bad_fork_cleanup_kstack;
    }
    copy_thread(proc, stack, tf);

    bool intr_flag;
    local_intr_save(intr_flag);
    {
        proc->pid = get_pid();
        hash_proc(proc);
        set_links(proc);

    }
    local_intr_restore(intr_flag);

    wakeup_proc(proc);

    ret = proc->pid;
fork_out:
    return ret;

bad_fork_cleanup_kstack:
    put_kstack(proc);
bad_fork_cleanup_proc:
    kfree(proc);
    goto fork_out;
}

// do_exit - called by sys_exit
//   1. call exit_mmap & put_pgdir & mm_destroy to free the almost all memory space of process
//   2. set process' state as PROC_ZOMBIE, then call wakeup_proc(parent) to ask parent reclaim itself.
//   3. call scheduler to switch to other process
int
do_exit(int error_code) {
    if (current == idleproc) {
        panic("idleproc exit.\n");
    }
    if (current == initproc) {
        panic("initproc exit.\n");
    }
    
    struct mm_struct *mm = current->mm;
    if (mm != NULL) {
        lcr3(boot_cr3);
        if (mm_count_dec(mm) == 0) {
            exit_mmap(mm);
            put_pgdir(mm);
            mm_destroy(mm);
        }
        current->mm = NULL;
    }
    current->state = PROC_ZOMBIE;
    current->exit_code = error_code;
    
    bool intr_flag;
    struct proc_struct *proc;
    local_intr_save(intr_flag);
    {
        proc = current->parent;
        if (proc->wait_state == WT_CHILD) {
            wakeup_proc(proc);
        }
        while (current->cptr != NULL) {
            proc = current->cptr;
            current->cptr = proc->optr;
    
            proc->yptr = NULL;
            if ((proc->optr = initproc->cptr) != NULL) {
                initproc->cptr->yptr = proc;
            }
            proc->parent = initproc;
            initproc->cptr = proc;
            if (proc->state == PROC_ZOMBIE) {
                if (initproc->wait_state == WT_CHILD) {
                    wakeup_proc(initproc);
                }
            }
        }
    }
    local_intr_restore(intr_flag);
    
    schedule();
    panic("do_exit will not return!! %d.\n", current->pid);
}

/* load_icode - load the content of binary program(ELF format) as the new content of current process
 * @binary:  the memory addr of the content of binary program
 * @size:  the size of the content of binary program
 */
static int
load_icode(unsigned char *binary, size_t size) {
    if (current->mm != NULL) {
        panic("load_icode: current->mm must be empty.\n");
    }

    int ret = -E_NO_MEM;
    struct mm_struct *mm;
    //(1) create a new mm for current process
    if ((mm = mm_create()) == NULL) {
        goto bad_mm;
    }
    //(2) create a new PDT, and mm->pgdir= kernel virtual addr of PDT
    if (setup_pgdir(mm) != 0) {
        goto bad_pgdir_cleanup_mm;
    }
    //(3) copy TEXT/DATA section, build BSS parts in binary to memory space of process
    struct Page *page;
    //(3.1) get the file header of the bianry program (ELF format)
    struct elfhdr *elf = (struct elfhdr *)binary;
    //(3.2) get the entry of the program section headers of the bianry program (ELF format)
    struct proghdr *ph = (struct proghdr *)(binary + elf->e_phoff);
    //(3.3) This program is valid?
    if (elf->e_magic != ELF_MAGIC) {
        ret = -E_INVAL_ELF;
        goto bad_elf_cleanup_pgdir;
    }

    uint32_t vm_flags, perm;
    struct proghdr *ph_end = ph + elf->e_phnum;
    for (; ph < ph_end; ph ++) {
    //(3.4) find every program section headers
        if (ph->p_type != ELF_PT_LOAD) {
            continue ;
        }
        if (ph->p_filesz > ph->p_memsz) {
            ret = -E_INVAL_ELF;
            goto bad_cleanup_mmap;
        }
        if (ph->p_filesz == 0) {
            continue ;
        }
    //(3.5) call mm_map fun to setup the new vma ( ph->p_va, ph->p_memsz)
        vm_flags = 0, perm = PTE_U;
        if (ph->p_flags & ELF_PF_X) vm_flags |= VM_EXEC;
        if (ph->p_flags & ELF_PF_W) vm_flags |= VM_WRITE;
        if (ph->p_flags & ELF_PF_R) vm_flags |= VM_READ;
        if (vm_flags & VM_WRITE) perm |= PTE_W;
        if ((ret = mm_map(mm, ph->p_va, ph->p_memsz, vm_flags, NULL)) != 0) {
            goto bad_cleanup_mmap;
        }
        unsigned char *from = binary + ph->p_offset;
        size_t off, size;
        uintptr_t start = ph->p_va, end, la = ROUNDDOWN(start, PGSIZE);

        ret = -E_NO_MEM;

     //(3.6) alloc memory, and  copy the contents of every program section (from, from+end) to process's memory (la, la+end)
        end = ph->p_va + ph->p_filesz;
     //(3.6.1) copy TEXT/DATA section of bianry program
        while (start < end) {
            if ((page = pgdir_alloc_page(mm->pgdir, la, perm)) == NULL) {
                goto bad_cleanup_mmap;
            }
            off = start - la, size = PGSIZE - off, la += PGSIZE;
            if (end < la) {
                size -= la - end;
            }
            memcpy(page2kva(page) + off, from, size);
            start += size, from += size;
        }

      //(3.6.2) build BSS section of binary program
        end = ph->p_va + ph->p_memsz;
        if (start < la) {
            /* ph->p_memsz == ph->p_filesz */
            if (start == end) {
                continue ;
            }
            off = start + PGSIZE - la, size = PGSIZE - off;
            if (end < la) {
                size -= la - end;
            }
            memset(page2kva(page) + off, 0, size);
            start += size;
            assert((end < la && start == end) || (end >= la && start == la));
        }
        while (start < end) {
            if ((page = pgdir_alloc_page(mm->pgdir, la, perm)) == NULL) {
                goto bad_cleanup_mmap;
            }
            off = start - la, size = PGSIZE - off, la += PGSIZE;
            if (end < la) {
                size -= la - end;
            }
            memset(page2kva(page) + off, 0, size);
            start += size;
        }
    }
    //(4) build user stack memory
    vm_flags = VM_READ | VM_WRITE | VM_STACK;
    if ((ret = mm_map(mm, USTACKTOP - USTACKSIZE, USTACKSIZE, vm_flags, NULL)) != 0) {
        goto bad_cleanup_mmap;
    }
    assert(pgdir_alloc_page(mm->pgdir, USTACKTOP-PGSIZE , PTE_USER) != NULL);
    assert(pgdir_alloc_page(mm->pgdir, USTACKTOP-2*PGSIZE , PTE_USER) != NULL);
    assert(pgdir_alloc_page(mm->pgdir, USTACKTOP-3*PGSIZE , PTE_USER) != NULL);
    assert(pgdir_alloc_page(mm->pgdir, USTACKTOP-4*PGSIZE , PTE_USER) != NULL);
    
    //(5) set current process's mm, sr3, and set CR3 reg = physical addr of Page Directory
    mm_count_inc(mm);
    current->mm = mm;
    current->cr3 = PADDR(mm->pgdir);
    lcr3(PADDR(mm->pgdir));

    //(6) setup trapframe for user environment
    struct trapframe *tf = current->tf;
    memset(tf, 0, sizeof(struct trapframe));
    /* LAB5:EXERCISE1 YOUR CODE
     * should set tf_cs,tf_ds,tf_es,tf_ss,tf_esp,tf_eip,tf_eflags
     * NOTICE: If we set trapframe correctly, then the user level process can return to USER MODE from kernel. So
     *          tf_cs should be USER_CS segment (see memlayout.h)
     *          tf_ds=tf_es=tf_ss should be USER_DS segment
     *          tf_esp should be the top addr of user stack (USTACKTOP)
     *          tf_eip should be the entry point of this binary program (elf->e_entry)
     *          tf_eflags should be set to enable computer to produce Interrupt
     */
    tf->tf_cs = USER_CS;
    tf->tf_ds = tf->tf_es = tf->tf_ss = USER_DS;
    tf->tf_esp = USTACKTOP;
    tf->tf_eip = elf->e_entry;
    tf->tf_eflags = FL_IF;
    ret = 0;
out:
    return ret;
bad_cleanup_mmap:
    exit_mmap(mm);
bad_elf_cleanup_pgdir:
    put_pgdir(mm);
bad_pgdir_cleanup_mm:
    mm_destroy(mm);
bad_mm:
    goto out;
}

// do_execve - call exit_mmap(mm)&put_pgdir(mm) to reclaim memory space of current process
//           - call load_icode to setup new memory space accroding binary prog.
int
do_execve(const char *name, size_t len, unsigned char *binary, size_t size) {
    struct mm_struct *mm = current->mm;
    if (!user_mem_check(mm, (uintptr_t)name, len, 0)) {
        return -E_INVAL;
    }
    if (len > PROC_NAME_LEN) {
        len = PROC_NAME_LEN;
    }

    char local_name[PROC_NAME_LEN + 1];
    memset(local_name, 0, sizeof(local_name));
    memcpy(local_name, name, len);

    if (mm != NULL) {
        lcr3(boot_cr3);
        if (mm_count_dec(mm) == 0) {
            exit_mmap(mm);
            put_pgdir(mm);
            mm_destroy(mm);
        }
        current->mm = NULL;
    }
    int ret;
    if ((ret = load_icode(binary, size)) != 0) {
        goto execve_exit;
    }
    set_proc_name(current, local_name);
    return 0;

execve_exit:
    do_exit(ret);
    panic("already exit: %e.\n", ret);
}

// do_yield - ask the scheduler to reschedule
int
do_yield(void) {
    current->need_resched = 1;
    return 0;
}

// do_wait - wait one OR any children with PROC_ZOMBIE state, and free memory space of kernel stack
//         - proc struct of this child.
// NOTE: only after do_wait function, all resources of the child proces are free.
int
do_wait(int pid, int *code_store) {
    struct mm_struct *mm = current->mm;
    if (code_store != NULL) {
        if (!user_mem_check(mm, (uintptr_t)code_store, sizeof(int), 1)) {
            return -E_INVAL;
        }
    }

    struct proc_struct *proc;
    bool intr_flag, haskid;
repeat:
    haskid = 0;
    if (pid != 0) {
        proc = find_proc(pid);
        if (proc != NULL && proc->parent == current) {
            haskid = 1;
            if (proc->state == PROC_ZOMBIE) {
                goto found;
            }
        }
    }
    else {
        proc = current->cptr;
        for (; proc != NULL; proc = proc->optr) {
            haskid = 1;
            if (proc->state == PROC_ZOMBIE) {
                goto found;
            }
        }
    }
    if (haskid) {
        current->state = PROC_SLEEPING;
        current->wait_state = WT_CHILD;
        schedule();
        if (current->flags & PF_EXITING) {
            do_exit(-E_KILLED);
        }
        goto repeat;
    }
    return -E_BAD_PROC;

found:
    if (proc == idleproc || proc == initproc) {
        panic("wait idleproc or initproc.\n");
    }
    if (code_store != NULL) {
        *code_store = proc->exit_code;
    }
    local_intr_save(intr_flag);
    {
        unhash_proc(proc);
        remove_links(proc);
    }
    local_intr_restore(intr_flag);
    put_kstack(proc);
    kfree(proc);
    return 0;
}

// do_kill - kill process with pid by set this process's flags with PF_EXITING
int
do_kill(int pid) {
    struct proc_struct *proc;
    if ((proc = find_proc(pid)) != NULL) {
        if (!(proc->flags & PF_EXITING)) {
            proc->flags |= PF_EXITING;
            if (proc->wait_state & WT_INTERRUPTED) {
                wakeup_proc(proc);
            }
            return 0;
        }
        return -E_KILLED;
    }
    return -E_INVAL;
}

// kernel_execve - do SYS_exec syscall to exec a user program called by user_main kernel_thread
static int
kernel_execve(const char *name, unsigned char *binary, size_t size) {
    int ret, len = strlen(name);
    asm volatile (
        "int %1;"
        : "=a" (ret)
        : "i" (T_SYSCALL), "0" (SYS_exec), "d" (name), "c" (len), "b" (binary), "D" (size)
        : "memory");
    return ret;
}

#define __KERNEL_EXECVE(name, binary, size) ({                          \
            cprintf("kernel_execve: pid = %d, name = \"%s\".\n",        \
                    current->pid, name);                                \
            kernel_execve(name, binary, (size_t)(size));                \
        })

#define KERNEL_EXECVE(x) ({                                             \
            extern unsigned char _binary_obj___user_##x##_out_start[],  \
                _binary_obj___user_##x##_out_size[];                    \
            __KERNEL_EXECVE(#x, _binary_obj___user_##x##_out_start,     \
                            _binary_obj___user_##x##_out_size);         \
        })

#define __KERNEL_EXECVE2(x, xstart, xsize) ({                           \
            extern unsigned char xstart[], xsize[];                     \
            __KERNEL_EXECVE(#x, xstart, (size_t)xsize);                 \
        })

#define KERNEL_EXECVE2(x, xstart, xsize)        __KERNEL_EXECVE2(x, xstart, xsize)

// user_main - kernel thread used to exec a user program
static int
user_main(void *arg) {
#ifdef TEST
    KERNEL_EXECVE2(TEST, TESTSTART, TESTSIZE);
#else
    KERNEL_EXECVE(exit);
#endif
    panic("user_main execve failed.\n");
}

#define PARTITION_MAIN_DEF(part_id)                         \
    static int partition_##part_id (void *arg) {            \
        KERNEL_EXECVE(part_##part_id);                      \
        panic("create partition_" #part_id "failed\n");     \
    }                                                       \



#define DEF_1_PARTITION         \
    PARTITION_MAIN_DEF(0)

#define DEF_2_PARTITION         \
    DEF_1_PARTITION             \
    PARTITION_MAIN_DEF(1)

#define DEF_3_PARTITION         \
    DEF_2_PARTITION             \
    PARTITION_MAIN_DEF(2)

#define DEF_4_PARTITION         \
    DEF_3_PARTITION             \
    PARTITION_MAIN_DEF(3)

#define DEF_5_PARTITION         \
    DEF_4_PARTITION             \
    PARTITION_MAIN_DEF(4)

#define DEF_6_PARTITION         \
    DEF_5_PARTITION             \
    PARTITION_MAIN_DEF(5)


#define PARTITION_EXEC(part_id)                                         \
    int part_##part_id;                                                 \
    if (partition_add(&part_##part_id) == NULL) {                       \
        panic("add part_" #part_id " failed.\n");                       \
    }                                                                   \
    int pid_##part_id = kernel_thread(partition_##part_id, NULL, 0);    \
    if (pid_##part_id <= 0) {                                           \
        panic("create part_" #part_id " failed.\n");                    \
    }                                                                   \



#define EXEC_1_PARTITION    \
    PARTITION_EXEC(0)

#define EXEC_2_PARTITION    \
    EXEC_1_PARTITION;       \
    PARTITION_EXEC(1)

#define EXEC_3_PARTITION    \
    EXEC_2_PARTITION;       \
    PARTITION_EXEC(2)

#define EXEC_4_PARTITION    \
    EXEC_3_PARTITION;       \
    PARTITION_EXEC(3)

#define EXEC_5_PARTITION    \
    EXEC_4_PARTITION;       \
    PARTITION_EXEC(4)

#define EXEC_6_PARTITION    \
    EXEC_5_PARTITION;       \
    PARTITION_EXEC(5)


DEF_3_PARTITION

static int
init_main(void *arg) {
//    size_t nr_free_pages_store = nr_free_pages();
//    size_t kernel_allocated_store = kallocated();

//    int pid = kernel_thread(partition_0, NULL, 0);
//    if (pid <= 0) {
//        panic("create user_main failed.\n");
//    }

    EXEC_3_PARTITION;
 // extern void check_sync(void);
    // check_sync();                // check philosopher sync problem

//    while (do_wait(0, NULL) == 0) {
//        schedule();
//    }

//    cprintf("all user-mode processes have quit.\n");
//    assert(initproc->cptr == NULL && initproc->yptr == NULL && initproc->optr == NULL);
//    assert(nr_process == 2);
//    assert(list_next(&proc_list) == &(initproc->list_link));
//    assert(list_prev(&proc_list) == &(initproc->list_link));
//    assert(nr_free_pages_store == nr_free_pages());
//    assert(kernel_allocated_store == kallocated());
//    cprintf("init check memory pass.\n");
    partition_t *part = current->part;
    part->scheduling = 0;
    part->status.operating_mode = NORMAL;
    cprintf("init partition done.\n");
    while (1);
    return 0;
}

// proc_init - set up the first kernel thread idleproc "idle" by itself and 
//           - create the second kernel thread init_main
void
proc_init(void) {
    int i;

    list_init(&proc_list);
    for (i = 0; i < HASH_LIST_SIZE; i ++) {
        list_init(hash_list + i);
    }

    if ((idleproc = alloc_proc()) == NULL) {
        panic("cannot alloc idleproc.\n");
    }

    idleproc->pid = 0;
//    idleproc->state = PROC_RUNNABLE;
    proc_state(idleproc) = RUNNING;
    idleproc->kstack = (uintptr_t)bootstack;
    idleproc->need_resched = 1;
    set_proc_name(idleproc, "idle");
    nr_process ++;

    current = idleproc;

    // init partition
    partition_t *init_part;
    if ((init_part = partition_add(NULL)) == NULL) {
        panic("create init partition failed.\n");
    }

    idleproc->part = init_part;
    init_part->scheduling = 1;

    int pid = kernel_thread(init_main, NULL, 0);
    if (pid <= 0) {
        panic("create init_main failed.\n");
    }

    initproc = find_proc(pid);
    set_proc_name(initproc, "init");

    init_part->idle_proc = initproc;
    initproc->part = init_part;

    assert(idleproc != NULL && idleproc->pid == 0);
    assert(initproc != NULL && initproc->pid == 1);
}

// cpu_idle - at the end of kern_init, the first kernel thread idleproc will do below works
void
cpu_idle(void) {
    while (1) {
        if (current->need_resched) {
            schedule();
        }
    }
}

//FOR LAB6, set the process's priority (bigger value will get more CPU time) 
//  void
//  lab6_set_priority(uint32_t priority)
//  {
//      if (priority == 0)
//          current->lab6_priority = 1;
//      else current->lab6_priority = priority;
//  }

// do_sleep - set current process state to sleep and add timer with "time"
//          - then call scheduler. if process run again, delete timer first.
int
do_sleep(unsigned int time) {
    if (time == 0) {
        return 0;
    }
    bool intr_flag;
    local_intr_save(intr_flag);
    timer_t __timer, *timer = timer_init(&__timer, current, time);
    current->state = PROC_SLEEPING;
    current->wait_state = WT_TIMER;
    add_timer(timer);
    local_intr_restore(intr_flag);

    schedule();

    del_timer(timer);
    return 0;
}


int
do_mmap(uintptr_t *addr_store, size_t len, uint32_t mmap_flags) {
    struct mm_struct *mm = current->mm;
    if (mm == NULL) {
        panic("kernel thread call mmap!!.\n");
    }

    // cprintf("do_mmap arg: %u %d.\n", (int)addr_store, len);

    if (addr_store == NULL || len == 0) {
        return -E_INVAL;
    }

    int ret = -E_INVAL;

    uintptr_t addr;

    lock_mm(mm);
//    if (!copy_from_user(mm, &addr, addr_store, sizeof(uintptr_t), 1)) {
//        goto out_unlock;
//    }
    addr = *addr_store;
    // cprintf("do_mmap copy_from_user pass.\n");

    uintptr_t start = ROUNDDOWN(addr, PGSIZE), end = ROUNDUP(addr + len, PGSIZE);
    addr = start, len = end - start;

    uint32_t vm_flags = VM_READ;
    if (mmap_flags & VM_WRITE) vm_flags |= VM_WRITE;
    if (mmap_flags & VM_STACK) vm_flags |= VM_STACK;

    ret = -E_NO_MEM;
    if (addr == 0) {
        if ((addr = get_unmapped_area(mm, len)) == 0) {
            goto out_unlock;
        }
    }
    if ((ret = mm_map(mm, addr, len, vm_flags, NULL)) == 0) {
        *addr_store = addr;
    }
    cprintf("do_mmap mm_map pass.\n");

out_unlock:
    unlock_mm(mm);
    return ret;
}

int
do_munmap(uintptr_t addr, size_t len) {
    struct mm_struct *mm = current->mm;
    if (mm == NULL) {
        panic("kernel thread call munmap!!.\n");
    }
    if (len == 0) {
        return -E_INVAL;
    }
    int ret;
    lock_mm(mm);
    {
        ret = mm_unmap(mm, addr, len);
    }
    unlock_mm(mm);
    return ret;
}

static int valid_nr_proc(void) {
    partition_t *part = current->part;
    if (part->proc_num >= MAX_NUMBER_OF_PROCESSES) {
        return 0;
    }
    return 1;
}

static struct proc_struct *find_proc_name(const char *name) {
    list_entry_t *le = proc_list.next;
    struct proc_struct *proc;
    while (le != &proc_list) {
        proc = le2proc(le, list_link);
        if (strcmp(proc->name, name) == 0) {
            return proc;
        }
        le = list_next(le);
    }
    return NULL;
}


static int setup_ustack(struct proc_struct *proc) {
    proc->ustack = 0;
    if (do_mmap(&proc->ustack, proc->status.attributes.stack_size, VM_WRITE | VM_STACK) != 0) {
        warn("do_create_process: do_mmap failed.\n");
        return 1;
    }
    return 0;
}

//  static void proc_add_timeout(struct proc_struct *proc, int timeout) {
//      partition_t *part = proc->part;
//      if (proc->wait_state & WT_TIMER && proc->timeout_deadline < ticks + timeout) {
//          proc->timeout_deadline = ticks + timeout;
//          return;
//      }
//  
//      proc->status.process_state = WAITTING;
//      if (proc->wait_state == 0 || proc->wait_state & WT_SUSPEND_TIMER) {
//          list_del(&proc->state_link);
//          list_add_after(&part->timeout_set, &proc->state_link);
//      }
//      proc->wait_state |= WT_TIMER;
//  }

// arinc 653 api

static void init_proc_context(struct proc_struct *proc) {
    proc->tf = (struct trapframe *)(proc->kstack + KSTACKSIZE) - 1;
    memset(proc->tf, 0, sizeof(struct trapframe));
    memset(&proc->context, 0, sizeof(struct context));
    cprintf("set up user and kernel stack pass.\n");

    uintptr_t ustack = proc->ustack;
    uintptr_t ustack_size = proc->status.attributes.stack_size;
    uintptr_t entry = (uintptr_t)proc->status.attributes.entry_point;
    // copy thread
    proc->tf->tf_regs.reg_eax = 0;
    proc->tf->tf_esp = ustack + ustack_size;
    proc->tf->tf_eflags |= FL_IF;
    proc->tf->tf_cs = USER_CS;
    proc->tf->tf_ds = USER_DS;
    proc->tf->tf_es = USER_DS;
    proc->tf->tf_ss = USER_DS;
    proc->tf->tf_eip = (uint32_t)entry;

    proc->context.eip = (uintptr_t)forkret;
    proc->context.esp = (uintptr_t)(proc->tf);

    cprintf("copy thread pass.\n");
}

static void set_proc_link(struct proc_struct *proc) {
    // add proc to runlist
    bool intr_flag;
    local_intr_save(intr_flag);
    {
        proc->pid = get_pid();
        hash_proc(proc);
        set_links(proc);
    }
    local_intr_restore(intr_flag);

    // new partition schedule, add to partition set
    partition_t *part = proc->part;
    list_add(&part->proc_set, &proc->part_link);
    proc->status.process_state = DORMANT;
    list_add(&part->dormant_set, &proc->run_link);
    part->proc_num++;
}

void do_create_process(process_attribute_t *attr, 
                    process_id_t *pid,   
                    return_code_t *return_code) 
{
    partition_t *part = current->part;
    if (!valid_nr_proc()) {
        *return_code = INVALID_CONFIG;
        return ;
    }

    if (find_proc_name(attr->name) != NULL) {
        *return_code = NO_ACTION;
        return ;
    }

    if (attr->stack_size > MAX_STACK_SIZE) {
        *return_code = INVALID_PARAM;
        return ;
    }

    if (attr->base_priority > MAX_PRIORITY_VALUE) {
        *return_code = INVALID_PARAM;
        return ;
    }

    if (attr->period > MAX_PROC_PEROID) {
        *return_code = INVALID_PARAM;
        return ;
    }

    if (attr->time_capacity > MAX_TIME_CAPA) {
        *return_code = INVALID_PARAM;
        return ;
    }

    if (part->status.operating_mode == NORMAL) {
        *return_code = INVALID_MODE;
        return;
    }

    // operating mode
    struct proc_struct *proc;
    // alloc proc and pid
    if ((proc = alloc_proc()) == NULL) {
        *return_code = INVALID_CONFIG; 
        return ;
    }
    proc->status.attributes = *attr;
    proc->part = current->part;

    // set up user and kernel stack
    if (setup_ustack(proc) != 0) {
        *return_code = INVALID_CONFIG;
        kfree(proc);
        return;
    }
    cprintf("stack_addr: %x.\n", proc->ustack);
    // assert(user_mem_check(current->mm, stack, stack_size, 1));

    if (setup_kstack(proc) != 0) {
        *return_code = INVALID_CONFIG;
        do_munmap(proc->ustack, attr->stack_size);
        kfree(proc);
        return;
    }

    init_proc_context(proc);

    // set mm
    struct mm_struct *mm = current->mm;
    mm_count_inc(mm);
    proc->mm = mm;
    proc->cr3 = PADDR(mm->pgdir);

    set_proc_link(proc);

    proc->status.process_state = DORMANT; 
    *pid = proc->pid;
    *return_code = NO_ERROR;
    return;
}


void do_set_priority(process_id_t process_id,
                    uint8_t priority,
                    return_code_t *return_code) {
    struct proc_struct *proc = find_proc(process_id);
    if (proc == NULL) {
        *return_code = INVALID_PARAM;
        return;
    }

    if (priority > MAX_PRIORITY_VALUE) {
        *return_code = INVALID_PARAM;
        return;
    }

    if (proc->status.process_state == DORMANT) {
        *return_code = INVALID_MODE;
        return;
    }

    proc->status.current_priority = priority;
    if (PREEMPTION) {
        schedule();
    }

    *return_code = NO_ERROR;

}

void do_suspend_self(uint32_t time_out, return_code_t *return_code) {
    if (!PREEMPTION) {
        *return_code = INVALID_MODE;
        return;
    }

    if (time_out > MAX_TIME_OUT) {
        *return_code = INVALID_PARAM;
        return;
    }

    if (current->status.attributes.period == INFINITE_TIME_VALUE) {
        *return_code = INVALID_MODE;
        return;
    }

    if (time_out == 0) {
        *return_code = NO_ERROR;
    } else {
        struct proc_struct *proc = current;
        proc->status.process_state = WAITTING;
        list_del_init(&proc->run_link);
        set_wt_flag(proc, WT_TIMER | WT_SUSPEND);
        timer_t *timer;
        if (time_out != INFINITE_TIME_VALUE) {
            timer = kmalloc(sizeof(timer_t));
            timer_init(timer, current, time_out);
            add_timer(timer);
            current->timer = timer;
            // proc_add_timeout(current, time_out);
        }

        schedule();
        kfree(timer);
        if (proc->timer == NULL) {
            *return_code = TIMED_OUT;
            return;
        } else {
            proc->timer = NULL;
            *return_code = NO_ERROR;
            return;
        }
    }
}
            
void do_suspend(process_id_t process_id, return_code_t *return_code) {
    if (!PREEMPTION) {
        *return_code = INVALID_MODE;
        return;
    }

    struct proc_struct *proc = find_proc(process_id);
    if (proc == NULL || proc == current) {
        *return_code = INVALID_PARAM;
        return;
    }

    if (proc->status.process_state == DORMANT) {
        *return_code = INVALID_MODE;
        return;
    }

    if (proc->status.attributes.period != INFINITE_TIME_VALUE) {
        *return_code = INVALID_MODE;
        return;
    }

    if (proc->status.process_state == WAITTING && proc->wait_state & WT_SUSPEND) {
        *return_code = NO_ACTION;
        return;
    } else {
        list_del_init(&proc->run_link);
        proc->status.process_state = WAITTING;
        set_wt_flag(proc, WT_SUSPEND);
    }
}

void do_resume(process_id_t process_id, return_code_t *return_code) {
    struct proc_struct *proc = find_proc(process_id);
    if (proc == NULL) {
        *return_code = INVALID_PARAM;
        return;
    }

    if (proc->status.process_state == DORMANT) {
        *return_code = INVALID_MODE;
        return;
    }

    if (proc->status.attributes.period != INFINITE_TIME_VALUE) {
        *return_code = INVALID_MODE;
        return;
    }

    if (!test_wt_flag(proc, WT_SUSPEND)) {
        *return_code = NO_ACTION;
        return;
    }

    if (test_wt_flag(proc, WT_SUSPEND) && test_wt_flag(proc, WT_TIMER)) {
        // do not set proc->timer to NULL
        del_timer(proc->timer);
    }
    // TODO
    if (!test_wt_flag(proc, WT_KSEM) && !test_wt_flag(proc, WT_EVENT)) {
        proc->status.process_state = READY;
        wakeup_proc(proc);
        if (PREEMPTION) {
            schedule();
        }
    }
    *return_code = NO_ERROR;
}

void do_stop_self(void) {
    current->status.process_state = DORMANT;
    list_del_init(&current->run_link);
    list_add_after(&current->part->dormant_set, &current->run_link);
    schedule();
}

void do_stop(process_id_t process_id, return_code_t *return_code) {
    struct proc_struct *proc = find_proc(process_id);
    if (proc == NULL) {
        *return_code = INVALID_PARAM;
        return;
    }

    if (proc->status.process_state == DORMANT) {
        *return_code = NO_ACTION;
        return;
    }

    proc->status.process_state = DORMANT;
    list_del_init(&proc->run_link);
    if (proc->wait_state != 0) {
        if (test_wt_flag(proc, WT_TIMER)) {
            del_timer(proc->timer);
            kfree(proc->timer);
            proc->timer = NULL;
        }
        proc->wait_state = 0;
    }
    *return_code = NO_ERROR;
}

void do_start(process_id_t process_id, return_code_t *return_code) {
    struct proc_struct *proc = find_proc(process_id);
    if (proc == NULL) {
        *return_code = INVALID_PARAM;
        return;
    }

    if (proc->status.process_state != DORMANT) {
        *return_code = NO_ACTION;
        return;
    }

    if (proc->status.attributes.period == INFINITE_TIME_VALUE) {
        proc->status.current_priority = proc_baseproi(proc);
        // reset context and stack
        init_proc_context(proc);
        partition_t *part = proc->part;
        if (part->status.operating_mode == NORMAL) {
            proc->status.process_state = READY;
            proc->time_slice = proc->status.attributes.time_capacity;
            list_del_init(&proc->run_link);
            wakeup_proc(proc);
            if (PREEMPTION) {
                schedule();
            }
        } else {
            proc->status.process_state = WAITTING;
            set_wt_flag(proc, WT_PNORMAL);
        }
        *return_code = NO_ERROR;   
    }
}

void do_delayed_start(process_id_t process_id,
                    system_time_t delay_time,
                    return_code_t *return_code) {

                    }

void do_lock_preemption(lock_level_t *lock_level, return_code_t *return_code) {
    partition_t *part = current->part;
    if (part->status.operating_mode != NORMAL) {
        *return_code = NO_ACTION;
        return;
    }

    if (part->status.lock_level > MAX_LOCK_LEVEL) {
        *return_code = INVALID_CONFIG;
        return;
    }

    part->status.lock_level += 1;
    *lock_level = part->status.lock_level;

    // lock level
    *return_code = NO_ERROR;
    return;
}

void do_unlock_preemption(lock_level_t *lock_level, return_code_t *return_code) {
    partition_t *part = current->part;

    if (part->status.operating_mode != NORMAL || part->status.lock_level == 0) {
        *return_code = NO_ACTION;
        return;
    }

    part->status.lock_level -= 1;
    if (part->status.lock_level == 0) {
        *lock_level = 0;
        schedule();
    }
    *lock_level = part->status.lock_level;
    *return_code = NO_ERROR;
}

void do_get_my_id(process_id_t *process_id, return_code_t *return_code) {
    *process_id = current->pid;
    *return_code = NO_ERROR;
    return;
}

void do_get_process_id(process_name_t  process_name,
                    process_id_t    *process_id,
                    return_code_t   *return_code) {
    struct proc_struct *proc = find_proc_name(process_name);
    if (proc == NULL) {
        *return_code = INVALID_CONFIG;
        return;
    }
    *process_id = proc->pid;
    *return_code = NO_ERROR;
    return;
}


void do_get_process_status(process_id_t process_id,
                    process_status_t    *process_status,
                    return_code_t       *return_code) {
    struct proc_struct *proc = find_proc(process_id);
    if (proc == NULL) {
        *return_code = INVALID_PARAM;
        return;
    }
    *process_status = proc->status;
    *return_code = NO_ERROR;
    return;
}
