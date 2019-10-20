#include <defs.h>
#include <syscall.h>
#include <stdio.h>
#include <ulib.h>
#include <arinc_proc.h>
#include <semaphore.h>
#include <event.h>
#include <arinc_time.h>

void
exit(int error_code) {
    sys_exit(error_code);
    cprintf("BUG: exit failed.\n");
    while (1);
}

int
fork(void) {
    return sys_fork();
}

int
wait(void) {
    return sys_wait(0, NULL);
}

int
waitpid(int pid, int *store) {
    return sys_wait(pid, store);
}

void
yield(void) {
    sys_yield();
}

int
kill(int pid) {
    return sys_kill(pid);
}

int
getpid(void) {
    return sys_getpid();
}

//print_pgdir - print the PDT&PT
void
print_pgdir(void) {
    sys_pgdir();
}

unsigned int
gettime_msec(void) {
    return (unsigned int)sys_gettime();
}

void
lab6_set_priority(uint32_t priority)
{
    sys_lab6_set_priority(priority);
}

int
sleep(unsigned int time) {
    return sys_sleep(time);
}

int get_partition_id(void) {
    return sys_get_partition_id();
}

void create_process(process_attribute_t *attributes, 
                    process_id_t *process_id,   
                    return_code_t *return_code) {

    sys_create_process(attributes, process_id, return_code);
}

void set_priority(process_id_t process_id,
                    uint8_t priority,
                    return_code_t *return_code)
{
    sys_set_priority(process_id, priority, return_code);
}

void suspend_self(uint32_t time_out,
                    return_code_t *return_code)
{
    sys_suspend_self(time_out, return_code);
}
            
void suspend(process_id_t process_id, return_code_t *return_code) {
    sys_suspend(process_id, return_code);
}

void resume(process_id_t process_id, return_code_t *return_code) {
    sys_resume(process_id, return_code);
}

void stop_self(void) {
    sys_stop_self();
}

void stop(process_id_t process_id, return_code_t *return_code) {
    sys_stop(process_id, return_code);
}

void start(process_id_t process_id, return_code_t *return_code) {
    sys_start(process_id, return_code);
}

void delayed_start(process_id_t process_id,
                    system_time_t delay_time,
                    return_code_t *return_code)
{
    sys_delayed_start(process_id, delay_time, return_code);
}

void lock_preemption(lock_level_t *lock_level, return_code_t *return_code) {
    sys_lock_preemption(lock_level, return_code);
}

void unlock_preemption(lock_level_t *lock_level, return_code_t *return_code) {
    sys_unlock_preemption(lock_level, return_code);
}

void get_my_id(process_id_t *process_id, return_code_t *return_code) {
    sys_get_my_id(process_id, return_code);
}

void get_process_id(process_name_t  process_name,
                    process_id_t    *process_id,
                    return_code_t   *return_code)
{
    sys_get_process_id(process_name, process_id, return_code);
}

void get_process_status(process_id_t process_id,
                    process_status_t    *process_status,
                    return_code_t       *return_code)
{
    sys_get_process_status(process_id, process_status, return_code);
}

// semaphore

void create_semaphore (
    semaphore_name_t     semaphore_name,
    semaphore_value_t    current_value,
    semaphore_value_t    max_value,
    queuing_discipline_t queuing_discipline,
    semaphore_id_t       *semaphore_id,
    return_code_t        *return_code)
{
    sys_create_semaphore(semaphore_name, current_value, max_value, 
        queuing_discipline, semaphore_id, return_code);
}

void wait_semaphore (
    semaphore_id_t  semaphore_id,
    system_time_t   time_out,
    return_code_t   *return_code)
{
    sys_wait_semaphore(semaphore_id, time_out, return_code);
}

void signal_semaphore(
    semaphore_id_t  semaphore_id,
    return_code_t   *return_code)
{
    sys_signal_semaphore(semaphore_id, return_code);
}

void get_semaphore_id (
    semaphore_name_t    semaphore_name,
    semaphore_id_t      *semaphore_id,
    return_code_t       *return_code)
{
    sys_get_semaphore_id(semaphore_name, semaphore_id, return_code);
}

void get_semaphore_status (
    semaphore_id_t      semaphore_id,
    semaphore_status_t  *semaphore_status,
    return_code_t       *return_code)
{
    sys_get_semaphore_status(semaphore_id, semaphore_status, return_code);
}


void create_event(event_name_t event_name, event_id_t *event_id, return_code_t *return_code)
{
    sys_create_event(event_name, event_id, return_code);
}

void set_event(event_id_t event_id, return_code_t *return_code)
{
    sys_set_event(event_id, return_code);
}

void reset_event(event_id_t event_id, return_code_t *return_code) {
    sys_reset_event(event_id, return_code);
}

void wait_event(event_id_t event_id, system_time_t time_out, return_code_t *return_code)
{
    sys_wait_event(event_id, time_out, return_code);
}

void get_event_id(event_name_t event_name, event_id_t *event_id, return_code_t *return_code)
{
    sys_get_event_id(event_name, event_id, return_code);
}

void get_event_status(event_id_t event_id, event_status_t *event_status, return_code_t *return_code)
{
    sys_get_event_status(event_id, event_status, return_code);
}

void get_partition_status(partition_status_t *status, return_code_t *return_code) {
    sys_get_partition_status(status, return_code);
}

void set_partition_mode(operating_mode_t mode, return_code_t *return_code) {
    sys_set_partition_mode(mode, return_code);
}

void time_wait(system_time_t delay_time, return_code_t *return_code) {
    sys_time_wait(delay_time, return_code);
}

void periodic_wait(return_code_t *return_code) {
    sys_periodic_wait(return_code);
}

void get_time(system_time_t *system_time, return_code_t *return_code) {
    sys_get_time(system_time, return_code);
}

void replenish(system_time_t budget_time, return_code_t *return_code) {
    sys_replenish(budget_time, return_code);
}
