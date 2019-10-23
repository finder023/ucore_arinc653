#include <stdio.h>
#include <ulib.h>
#include <arinc_proc.h>
#include <partition.h>
#include <sampling_port.h>
#include <semaphore.h>
#include <queuing_port.h>

size_t freq = 8000;

static void test_thread(void) {
    uint32_t count = 0;
    while (1) {
        int pid = getpid();
        int ppid = get_partition_id();
        ++count;
        if (count % freq == 0) {
            count = 0;        
            cprintf("this is user process %d, in partition %d\n", pid, ppid);
        }
    }
}



static void test_write_sampling_port(void) {
    process_id_t pid;
    return_code_t ret;
    get_my_id(&pid, &ret);
    assert(ret == NO_ERROR);
    cprintf("write get my id: %d\n", pid);
    
    semaphore_id_t sem_id;
    get_semaphore_id("sem1", &sem_id, &ret);
    assert(ret == NO_ERROR);

    sampling_port_id_t spid;
    get_sampling_port_id("sample1", &spid, &ret);
    assert(ret == NO_ERROR);

    char msg[] = "hello______world";
    write_sampling_message(spid, msg, sizeof(msg), &ret);
    assert(ret == NO_ERROR);

    signal_semaphore(sem_id, &ret);
    assert(ret == NO_ERROR);
    cprintf("signal semaphore done.\n");

    while (1);
}

static void test_read_sampling_port(void) {
    process_id_t pid;
    return_code_t ret;
    get_my_id(&pid, &ret);
    assert(ret == NO_ERROR);
    cprintf("read get my id: %d\n", pid);
    
    semaphore_id_t sem_id;
    get_semaphore_id("sem1", &sem_id, &ret);
    assert(ret == NO_ERROR);

    wait_semaphore(sem_id, INFINITE_TIME_VALUE, &ret);
    assert(ret == NO_ERROR);
    cprintf("wait semphore done.\n");

    sampling_port_id_t spid;
    get_sampling_port_id("sample1", &spid, &ret);
    assert(ret == NO_ERROR);

    char buff[40] = {0};
    validity_t valid;
    long len;
    read_sampling_message(spid, buff, &len, &valid, &ret);
    assert(ret == NO_ERROR);

    cprintf("read sampling message: %s\n", buff);

    while (1);
}


static void test_queuing_port_send_msg(void) {
    queuing_port_id_t qid;
    return_code_t ret;

    get_queuing_port_id("queue1", &qid, &ret);
    assert(ret == NO_ERROR);

    char msg[] = "hello world_____";
    send_queuing_message(qid, msg, sizeof(msg), INFINITE_TIME_VALUE, &ret);
    assert(ret == NO_ERROR);

    cprintf("queue send msg done.\n");
    while (1);
}

static void test_queuing_port_recv_msg(void) {
    queuing_port_id_t qid;
    return_code_t ret;

    get_queuing_port_id("queue1", &qid, &ret);
    assert(ret == NO_ERROR);

    char buff[40] = {0};
    long len;
    receive_queuing_message(qid, INFINITE_TIME_VALUE, buff, &len, &ret);
    assert(ret == NO_ERROR);

    cprintf("queue recv message: %s.\n", buff);
    while (1);
}


static void test_queuing_port(void) {
    return_code_t ret;
    process_id_t apid;
    int pid = getpid();

    process_attribute_t attr1 = DEFAULT_ATTR(test_queuing_port_recv_msg, "write_sample");
    create_process(&attr1, &apid, &ret);
    if (ret != NO_ERROR)
        panic("create process failed.\n");
    cprintf("create process: %d\n", apid);
    start(apid, &ret);
    if (ret != NO_ERROR) {
        panic("start process failed: %d.\n", pid);
    }

    process_attribute_t attr2 = DEFAULT_ATTR(test_queuing_port_send_msg, "read_sample");
    create_process(&attr2, &apid, &ret);
    if (ret != NO_ERROR)
        panic("create process failed.\n");
    cprintf("create process: %d\n", apid);
    start(apid, &ret);
    if (ret != NO_ERROR) {
        panic("start process failed: %d.\n", pid);
    }
}

static void test_sampling_port(void) {
    return_code_t ret;
    process_id_t apid;
    int pid = getpid();

    process_attribute_t attr1 = DEFAULT_ATTR(test_write_sampling_port, "write_sample");
    create_process(&attr1, &apid, &ret);
    if (ret != NO_ERROR)
        panic("create process failed.\n");
    cprintf("create process: %d\n", apid);
    start(apid, &ret);
    if (ret != NO_ERROR) {
        panic("start process failed: %d.\n", pid);
    }

    process_attribute_t attr2 = DEFAULT_ATTR(test_read_sampling_port, "read_sample");
    create_process(&attr2, &apid, &ret);
    if (ret != NO_ERROR)
        panic("create process failed.\n");
    cprintf("create process: %d\n", apid);
    start(apid, &ret);
    if (ret != NO_ERROR) {
        panic("start process failed: %d.\n", pid);
    }
}


int main(void) {
    int pid, ppid;
    pid = getpid();
    ppid = get_partition_id();
    return_code_t ret;
    
    cprintf("this is process: %d, partition: %d\n", pid, ppid);

    test_queuing_port();
    test_sampling_port();

    sampling_port_id_t spid;
    create_sampling_port("sample1", 200, SOURCE, 200, &spid, &ret);
    if (ret != NO_ERROR) {
        panic("create_sampling port failed.\n");
    }
    cprintf("create sampling port pass: %d.\n", spid);

    semaphore_id_t semid;
    create_semaphore("sem1", 0, 1, FIFO, &semid, &ret);
    assert(ret == NO_ERROR);
    cprintf("create semaphore pass: %d.\n", semid);

    queuing_port_id_t qid;
    port_direction_t direct = SOURCE;
    create_queuing_port("queue1", 100, 100, direct, FIFO, &qid, &ret);
    assert(ret == NO_ERROR);
    cprintf("create queue pass: %d.\n", qid);

    set_partition_mode(NORMAL, &ret);
    // idle process
    while (1);
    return 0;
}