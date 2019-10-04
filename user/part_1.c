#include <stdio.h>
#include <ulib.h>

int main(void) {
    int pid, ppid;
    pid = getpid();
    ppid = get_partition_id();
    
    cprintf("this is process: %d, partition: %d\n", pid, ppid);
    return 0;
}