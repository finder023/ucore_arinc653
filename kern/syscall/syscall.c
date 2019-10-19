#include <unistd.h>
#include <proc.h>
#include <syscall.h>
#include <trap.h>
#include <stdio.h>
#include <pmm.h>
#include <assert.h>
#include <clock.h>
#include <arinc_proc.h>
#include <string.h>
#include <semaphore.h>

static int
sys_exit(uint32_t arg[]) {
    int error_code = (int)arg[0];
    return do_exit(error_code);
}

static int
sys_fork(uint32_t arg[]) {
    struct trapframe *tf = current->tf;
    uintptr_t stack = tf->tf_esp;
    return do_fork(0, stack, tf);
}

static int
sys_wait(uint32_t arg[]) {
    int pid = (int)arg[0];
    int *store = (int *)arg[1];
    return do_wait(pid, store);
}

static int
sys_exec(uint32_t arg[]) {
    const char *name = (const char *)arg[0];
    size_t len = (size_t)arg[1];
    unsigned char *binary = (unsigned char *)arg[2];
    size_t size = (size_t)arg[3];
    return do_execve(name, len, binary, size);
}

static int
sys_yield(uint32_t arg[]) {
    return do_yield();
}

static int
sys_kill(uint32_t arg[]) {
    int pid = (int)arg[0];
    return do_kill(pid);
}

static int
sys_getpid(uint32_t arg[]) {
    return current->pid;
}

static int
sys_putc(uint32_t arg[]) {
    int c = (int)arg[0];
    cputchar(c);
    return 0;
}

static int
sys_pgdir(uint32_t arg[]) {
    print_pgdir();
    return 0;
}

static uint32_t
sys_gettime(uint32_t arg[]) {
    return (int)ticks;
}
//  static uint32_t
//  sys_lab6_set_priority(uint32_t arg[])
//  {
//      uint32_t priority = (uint32_t)arg[0];
//      lab6_set_priority(priority);
//      return 0;
//  }

static int
sys_sleep(uint32_t arg[]) {
    unsigned int time = (unsigned int)arg[0];
    return do_sleep(time);
}

static int sys_get_partition_id(uint32_t arg[]) {
    partition_t *part = current->part;
    return part->status.identifier;
}

static int sys_create_process(uint32_t  arg[]) {
    process_attribute_t *attr = (process_attribute_t*)arg[0];
    process_id_t *pid = (process_id_t*)arg[1];
    return_code_t *ret = (return_code_t*)arg[2];
    do_create_process(attr, pid, ret);
    return *ret;
}

static int sys_set_priority(uint32_t arg[]) {
    process_id_t pid = (process_id_t)arg[0];
    uint8_t proi = (uint8_t)arg[1];
    return_code_t *ret = (return_code_t*)arg[2];
    do_set_priority(pid, proi, ret);
    return *ret; 
}

static int sys_suspend_self(uint32_t arg[]) {
    uint32_t timeout = arg[0];
    return_code_t *ret = (return_code_t*)arg[1];
    do_suspend_self(timeout, ret);
    return *ret; 

}
            
static int sys_suspend(uint32_t arg[]) {
    process_id_t pid = (process_id_t)arg[0];
    return_code_t *ret = (return_code_t*)arg[1];
    do_suspend(pid, ret);
    return *ret;
}

static int sys_resume(uint32_t arg[]) {
    process_id_t pid = (process_id_t)arg[0];
    return_code_t *ret = (return_code_t*)arg[1];
    do_resume(pid, ret);
    return *ret;
}

static int sys_stop_self(void) {
    do_stop_self();
    return 0;
}

static int sys_stop(uint32_t arg[]) {
    process_id_t pid = (process_id_t)arg[0];
    return_code_t *ret = (return_code_t*)arg[1];
    do_stop(pid, ret);
    return *ret;
}

static int sys_start(uint32_t arg[]) {
    process_id_t pid = (process_id_t)arg[0];
    return_code_t *ret = (return_code_t*)arg[1];
    do_start(pid, ret);
    return *ret;
}

static int sys_delayed_start(uint32_t arg[]) {
    process_id_t pid = (process_id_t)arg[0];
    system_time_t delay = (system_time_t)arg[1];
    return_code_t *ret = (return_code_t*)arg[2];
    do_delayed_start(pid, delay, ret);
    return *ret;
}

static int sys_lock_preemption(uint32_t arg[]) {
    lock_level_t *lock_level = (lock_level_t*)arg[0];
    return_code_t *ret = (return_code_t*)arg[1];
    do_lock_preemption(lock_level, ret);
    return *ret;
}

static int sys_unlock_preemption(uint32_t arg[]) {
    lock_level_t *lock_level = (lock_level_t*)arg[0];
    return_code_t *ret = (return_code_t*)arg[1]; 
    do_unlock_preemption(lock_level, ret);
    return *ret;
}

static int sys_get_my_id(uint32_t arg[]) {
    process_id_t *pid = (process_id_t*)arg[0];
    return_code_t *ret = (return_code_t*)arg[1];
    do_get_my_id(pid, ret);
    return *ret;
}

static int sys_get_process_id(uint32_t arg[]) {

    process_id_t *pid = (process_id_t*)arg[1];
    process_name_t name;
    strcpy(name, (char*)arg[0]);
    return_code_t *ret = (return_code_t*)arg[2];
    do_get_process_id(name, pid, ret);
    return *ret;
}

static int sys_get_process_status(uint32_t arg[]) {
    process_id_t pid = (process_id_t)arg[0];
    process_status_t *status = (process_status_t*)arg[1];
    return_code_t *ret = (return_code_t*)arg[1];
    do_get_process_status(pid, status, ret);
    return *ret;
}


static int sys_create_semphore (uint32_t arg[]) {
    semaphore_name_t     semaphore_name; 
    strcpy(semaphore_name, (char*)arg[0]);
    semaphore_value_t    current_value = arg[1];
    semaphore_value_t    max_value = arg[2];
    queuing_discipline_t queuing_discipline = (queuing_discipline_t)arg[3];
    semaphore_id_t       *semaphore_id = (semaphore_id_t*)arg[4];
    return_code_t        return_code;
    do_create_semaphore(semaphore_name, current_value, max_value,
        queuing_discipline, semaphore_id, &return_code);
    return return_code;
}

static int sys_wait_semaphore ( uint32_t arg[]) {
    semaphore_id_t  semaphore_id = arg[0];
    system_time_t   time_out = arg[1];
    return_code_t   *return_code = (return_code_t*)arg[2];
    do_wait_semaphore(semaphore_id, time_out, return_code);
    return *return_code;
}

static int sys_signal_semaphore(uint32_t arg[]) {
    semaphore_id_t  semaphore_id = arg[0];
    return_code_t   *return_code = (return_code_t*)arg[2];
    do_signal_semaphore(semaphore_id, return_code);
    return *return_code;
}

static int sys_get_semaphore_id (uint32_t arg[]) {
    semaphore_name_t    semaphore_name;
    strcpy(semaphore_name, (char*)arg[0]);
    semaphore_id_t      *semaphore_id = (semaphore_id_t*)arg[1];
    return_code_t       *return_code = (return_code_t*)arg[2];
    do_get_semaphore_id(semaphore_name, semaphore_id, return_code);
    return *return_code;
}

static int sys_get_semaphore_status (uint32_t arg[]) {
    semaphore_id_t      semaphore_id = arg[0];
    semaphore_status_t  *semaphore_status = (semaphore_status_t*)arg[1];
    return_code_t       *return_code = (return_code_t*)arg[2];
    do_get_semaphore_status(semaphore_id, semaphore_status, return_code);
    return *return_code;
}


static int (*syscalls[])(uint32_t arg[]) = {
    [SYS_exit]              sys_exit,
    [SYS_fork]              sys_fork,
    [SYS_wait]              sys_wait,
    [SYS_exec]              sys_exec,
    [SYS_yield]             sys_yield,
    [SYS_kill]              sys_kill,
    [SYS_getpid]            sys_getpid,
    [SYS_putc]              sys_putc,
    [SYS_pgdir]             sys_pgdir,
    [SYS_gettime]           sys_gettime,
    [SYS_sleep]             sys_sleep,
    
    [SYS_getpartid]             sys_get_partition_id,
    [SYS_createproc]            sys_create_process,
    [SYS_set_priority]          sys_set_priority,
    [SYS_suspendself]           sys_suspend_self,
    [SYS_suspend]               sys_suspend,
    [SYS_resume]                sys_resume,
    [SYS_stopself]              sys_stop_self,
    [SYS_stop]                  sys_stop,
    [SYS_start]                 sys_start,
    [SYS_delayedstart]          sys_delayed_start,
    [SYS_lockpreemption]        sys_lock_preemption,
    [SYS_unlockpreemption]      sys_unlock_preemption,
    [SYS_getmyid]               sys_get_my_id,
    [SYS_getprocessid]          sys_get_process_id,
    [SYS_getprocessstatus]      sys_get_process_status,
    [SYS_createsemaphore]       sys_create_semphore,
    [SYS_waitsemaphore]         sys_wait_semaphore,
    [SYS_signalsemaphore]       sys_signal_semaphore,
    [SYS_getsemaphoreid]        sys_get_semaphore_id,
    [SYS_getsemaphorestatus]    sys_get_semaphore_status,
};

#define NUM_SYSCALLS        ((sizeof(syscalls)) / (sizeof(syscalls[0])))

void
syscall(void) {
    struct trapframe *tf = current->tf;
    uint32_t arg[5];
    int num = tf->tf_regs.reg_eax;
    if (num >= 0 && num < NUM_SYSCALLS) {
        if (syscalls[num] != NULL) {
            arg[0] = tf->tf_regs.reg_edx;
            arg[1] = tf->tf_regs.reg_ecx;
            arg[2] = tf->tf_regs.reg_ebx;
            arg[3] = tf->tf_regs.reg_edi;
            arg[4] = tf->tf_regs.reg_esi;
            tf->tf_regs.reg_eax = syscalls[num](arg);
            return ;
        }
    }
    print_trapframe(tf);
    panic("undefined syscall %d, pid = %d, name = %s.\n",
            num, current->pid, current->name);
}

