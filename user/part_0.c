#include <stdio.h>
#include <ulib.h>
#include <arinc_proc.h>
#include <partition.h>

size_t freq = 8000;

void test_thread(void) {
    uint32_t count = 0;
    while (1) {
        int pid = getpid();
        int ppid = get_partition_id();
//        cprintf("this is part %d, thread %d.\n", pid, ppid);
        ++count;
        if (count % freq == 0) {
            count = 0;        
            //cprintf("this is user process: %d, created by "
            //"create_process in partition: %d.\n", pid, ppid);
            cprintf("this is user process %d, in partition %d\n", pid, ppid);
        // cprintf("thread stack addr, &pid: %u, &ppid: %u.\n", 
        //            (uintptr_t)&pid, (uintptr_t)&ppid);
        }
    }
}


int main(void) {
    int pid, ppid;
    pid = getpid();
    ppid = get_partition_id();

    process_attribute_t attr = DEFAULT_ATTR(test_thread, "proc..");
    return_code_t ret;
    process_id_t apid;

    cprintf("this is process: %d, partition: %d\n", pid, ppid);
    create_process(&attr, &apid, &ret);
    if (ret != NO_ERROR)
        panic("create process failed.\n");
    cprintf("create process: %d\n", apid);
    start(apid, &ret);
    if (ret != NO_ERROR) {
        panic("start process failed: %d.\n", pid);
    }
    create_process(&attr, &apid, &ret);
    if (ret != NO_ERROR)
        panic("create process failed.\n");
    cprintf("create process: %d\n", apid);
    start(apid, &ret);
    if (ret != NO_ERROR) {
        panic("start process failed: %d.\n", pid);
    }
    set_partition_mode(NORMAL, &ret);
    // idle process
    while (1);
    return 0;
}