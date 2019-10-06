#include <stdio.h>
#include <ulib.h>

void test_thread(void) {
    int pid = getpid();
    int ppid = get_partition_id();
//    cprintf("this is part %d, thread %d.\n", pid, ppid);
    cprintf("this is user process: %d, created by "
        "create_process in partition: %d.\n", pid, ppid);
    cprintf("thread stack addr, &pid: %u, &ppid: %u.\n", 
                (uintptr_t)&pid, (uintptr_t)&ppid);
    while (1);
}

int main(void) {
    int pid, ppid;
    pid = getpid();
    ppid = get_partition_id();

    cprintf("this is process: %d, partition: %d\n", pid, ppid);
    if (create_process(test_thread, &pid, 4096 * 10) != 0) {
        panic("create process failed.\n");
    }
 
    // idle process
    while (1);
    return 0;
}