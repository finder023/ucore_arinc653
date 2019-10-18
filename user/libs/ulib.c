#include <defs.h>
#include <syscall.h>
#include <stdio.h>
#include <ulib.h>
#include <arinc_proc.h>

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

    return sys_create_process(attributes, process_id, return_code);
}