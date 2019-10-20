#include <defs.h>
#include <unistd.h>
#include <stdarg.h>
#include <syscall.h>
#include <arinc_proc.h>
#include <semaphore.h>

#define MAX_ARGS            5

static inline int
syscall(int num, ...) {
    va_list ap;
    va_start(ap, num);
    uint32_t a[MAX_ARGS];
    int i, ret;
    for (i = 0; i < MAX_ARGS; i ++) {
        a[i] = va_arg(ap, uint32_t);
    }
    va_end(ap);

    asm volatile (
        "int %1;"
        : "=a" (ret)
        : "i" (T_SYSCALL),
          "a" (num),
          "d" (a[0]),
          "c" (a[1]),
          "b" (a[2]),
          "D" (a[3]),
          "S" (a[4])
        : "cc", "memory");
    return ret;
}

int
sys_exit(int error_code) {
    return syscall(SYS_exit, error_code);
}

int
sys_fork(void) {
    return syscall(SYS_fork);
}

int
sys_wait(int pid, int *store) {
    return syscall(SYS_wait, pid, store);
}

int
sys_yield(void) {
    return syscall(SYS_yield);
}

int
sys_kill(int pid) {
    return syscall(SYS_kill, pid);
}

int
sys_getpid(void) {
    return syscall(SYS_getpid);
}

int
sys_putc(int c) {
    return syscall(SYS_putc, c);
}

int
sys_pgdir(void) {
    return syscall(SYS_pgdir);
}

size_t
sys_gettime(void) {
    return syscall(SYS_gettime);
}

void
sys_lab6_set_priority(uint32_t priority)
{
    syscall(SYS_lab6_set_priority, priority);
}

int
sys_sleep(unsigned int time) {
    return syscall(SYS_sleep, time);
}


int sys_get_partition_id(void) {
    return syscall(SYS_getpartid);
}

int sys_create_process(process_attribute_t *attr, process_id_t *pid, return_code_t *ret) {
    return syscall(SYS_createproc, attr, pid, ret);
} 

void sys_set_priority(process_id_t process_id,
                    uint8_t priority,
                    return_code_t *return_code) {
    syscall(SYS_set_priority, process_id, priority, return_code);
}

void sys_suspend_self(uint32_t time_out,
                    return_code_t *return_code) {
    syscall(SYS_suspendself, time_out, return_code);
}
            
void sys_suspend(process_id_t process_id, return_code_t *return_code) {
    syscall(SYS_suspend, process_id, return_code);
}

void sys_resume(process_id_t process_id, return_code_t *return_code) {
    syscall(SYS_resume, process_id, return_code);
}

void sys_stop_self(void) {
    syscall(SYS_stopself);
}

void sys_stop(process_id_t process_id, return_code_t *return_code) {
    syscall(SYS_stop, process_id, return_code);
}

void sys_start(process_id_t process_id, return_code_t *return_code) {
    syscall(SYS_start, process_id, return_code);
}

void sys_delayed_start(process_id_t process_id,
                    system_time_t delay_time,
                    return_code_t *return_code) {
    syscall(SYS_delayedstart, process_id, delay_time, return_code);
}

void sys_lock_preemption(lock_level_t *lock_level, return_code_t *return_code) {
    syscall(SYS_lockpreemption, lock_level, return_code);
}

void sys_unlock_preemption(lock_level_t *lock_level, return_code_t *return_code) {
    syscall(SYS_unlockpreemption, lock_level, return_code);
}

void sys_get_my_id(process_id_t *process_id, return_code_t *return_code) {
    syscall(SYS_getmyid, process_id, return_code);
}

void sys_get_process_id(process_name_t  process_name,
                    process_id_t    *process_id,
                    return_code_t   *return_code) 
{
    syscall(SYS_getprocessid, process_name, process_id, return_code);
}

void sys_get_process_status(process_id_t process_id,
                    process_status_t    *process_status,
                    return_code_t       *return_code)
{
    syscall(SYS_getprocessstatus, process_id, process_status, return_code);
}


// semaphore
void sys_create_semaphore (
    semaphore_name_t     semaphore_name,
    semaphore_value_t    current_value,
    semaphore_value_t    max_value,
    queuing_discipline_t queuing_discipline,
    semaphore_id_t       *semaphore_id,
    return_code_t        *return_code)
{
    *return_code = syscall(SYS_createsemaphore, semaphore_name, current_value, 
        max_value, queuing_discipline, semaphore_id);
}

void sys_wait_semaphore (
    semaphore_id_t  semaphore_id,
    system_time_t   time_out,
    return_code_t   *return_code)
{
    syscall(SYS_waitsemaphore, semaphore_id, time_out, return_code);
}

void sys_signal_semaphore(
    semaphore_id_t  semaphore_id,
    return_code_t   *return_code)
{
    syscall(SYS_signalsemaphore, semaphore_id, return_code);
}

void sys_get_semaphore_id (
    semaphore_name_t    semaphore_name,
    semaphore_id_t      *semaphore_id,
    return_code_t       *return_code)
{
    syscall(SYS_getsemaphoreid, semaphore_name, semaphore_id, return_code);
}

void sys_get_semaphore_status (
    semaphore_id_t      semaphore_id,
    semaphore_status_t  *semaphore_status,
    return_code_t       *return_code)
{
    syscall(SYS_getsemaphorestatus, semaphore_id, semaphore_status, return_code);
}

void sys_create_event(event_name_t event_name, event_id_t *event_id, return_code_t *return_code)
{
    syscall(SYS_createevent, event_name, event_id, return_code);
}

void sys_set_event(event_id_t event_id, return_code_t *return_code)
{
    syscall(SYS_setevent, event_id, return_code);
}

void sys_reset_event(event_id_t event_id, return_code_t *return_code)
{
    syscall(SYS_resetevent, event_id, return_code);
}

void sys_wait_event(event_id_t event_id, system_time_t time_out, return_code_t *return_code)
{
    syscall(SYS_waitevent, event_id, time_out, return_code);
}

void sys_get_event_id(event_name_t event_name, event_id_t *event_id, return_code_t *return_code)
{
    syscall(SYS_geteventid, event_name, event_id, return_code);
}

void sys_get_event_status(event_id_t event_id, event_status_t *event_status, return_code_t *return_code)
{
    syscall(SYS_geteventstatus, event_id, event_status, return_code);
}

void sys_get_partition_status(partition_status_t *status, return_code_t *return_code) {
    syscall(SYS_getpartitionstatus, status, return_code);
}

void sys_set_partition_mode(operating_mode_t mode, return_code_t *return_code) {
    syscall(SYS_setpartitionstatus, mode, return_code);
}