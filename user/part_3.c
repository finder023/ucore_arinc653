#include <stdio.h>
#include <ulib.h>
#include <arinc_proc.h>
#include <partition.h>


size_t freq = 8000;

void print_thread(void) {
    uint32_t count = 0;
    while (1) {
        int pid = getpid();
        int ppid = get_partition_id();
        ++count;
        if (count % freq == 0) {
            count = 0;        
            cprintf("this is user process %d, in partition %d\n", pid, ppid);
            cprintf("freq: %d, var addr: %x\n", freq, &freq);
        }
    }
}


int main(void) {
    int pid, ppid;
    pid = getpid();
    ppid = get_partition_id();

    process_attribute_t attr = DEFAULT_ATTR(print_thread, "print_proc5");
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

    strcpy(attr.name, "print_proc6");
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