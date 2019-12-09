#include <stdio.h>
#include <ulib.h>
#include <arinc_proc.h>
#include <partition.h>
#include <sampling_port.h>
#include <queuing_port.h>


size_t freq = 8000;

const char *sample_name = "sample1";
const char *queue_name = "queue1";


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

static void create_print_process(void) {
    process_attribute_t attr = DEFAULT_ATTR(print_thread, "print_proc3");
    return_code_t ret;
    process_id_t pid;

    // create process 
    create_process(&attr, &pid, &ret);
    if (ret != NO_ERROR) {
        panic("create process failed.\n");
    }
    cprintf("create process: %d.\n", pid);

    start(pid, &ret);
    if (ret != NO_ERROR) {
        panic("start process failed: %d.\n", pid);
    }

    // create another process
    strcpy(attr.name, "print_proc4");
    create_process(&attr, &pid, &ret);
    if (ret != NO_ERROR) {
        panic("create process failed.\n");
    }
    cprintf("create process: %d.\n", pid);

    start(pid, &ret);
    if (ret != NO_ERROR) {
        panic("start process failed: %d.\n", pid);
    }
}


/*
 sampling port process area
*/

static void read_sampling_port_process(void) {
    process_id_t pid;
    return_code_t ret;

    get_my_id(&pid, &ret);
    if (ret != NO_ERROR)
        panic("read sampling port process, get_my_id failed.\n");
    cprintf("read sampling port process: %d.\n", pid);

    sampling_port_id_t spid;
    get_sampling_port_id(sample_name, &spid, &ret);
    if (ret != NO_ERROR)
        panic("read sampling port process, get sampling port id failed.\n");

    char buff[200];
    message_size_t len;
    validity_t valid;
    for (;;) {
        read_sampling_message(spid, buff, &len, &valid, &ret);
        if (ret == NO_ERROR)
            break;
    }

    cprintf("read sampling port: %s\n", buff);

    // idle
    while(1);
}

static void write_sampling_port_process(void) {
    process_id_t pid;
    return_code_t ret;

    get_my_id(&pid, &ret);
    if (ret != NO_ERROR)
        panic("write sampling port process, get_my_id failed.\n");
    cprintf("write sampling port process: %d.\n", pid);

    sampling_port_id_t spid;
    get_sampling_port_id(sample_name, &spid, &ret);
    if (ret != NO_ERROR)
        panic("write sampling port process, get sampling port id failed.\n");
    
    // message content
    char msg[] = "hello, read sampling port message";
    write_sampling_message(spid, msg, sizeof(msg), &ret);
    if (ret != NO_ERROR)
        panic("write sampling port process, write sampling port failed.\n");

    cprintf("write sampling port process done.\n");

    // idle
    while(1);    
}


/*
    queuing port area
*/

static void send_queuing_port_process(void) {
    process_id_t pid;
    return_code_t ret;

    get_my_id(&pid, &ret);
    if (ret != NO_ERROR)
        panic("send queuing port process, get_my_id failed.\n");
    cprintf("send queuing port process: %d.\n", pid);

    queuing_port_id_t qpid; 
    get_queuing_port_id(queue_name, &qpid, &ret);
    if (ret != NO_ERROR)
        panic("send queuing port process, get queuing port id failed.\n");
    
    // message content
    char msg[] = "hello, recv queuing port message";
    send_queuing_message(qpid, msg, sizeof(msg), INFINITE_TIME_VALUE, &ret);
    if (ret != NO_ERROR)
        panic("send queuing port process, write queuing port failed.\n");

    cprintf("send queuing port process done.\n");

    // idle
    while(1);    
}

static void recv_queuing_port_process(void) {
    process_id_t pid;
    return_code_t ret;

    get_my_id(&pid, &ret);
    if (ret != NO_ERROR)
        panic("recv queuing port process, get_my_id failed.\n");
    cprintf("recv queuing port process: %d.\n", pid);

    queuing_port_id_t qpid;
    get_queuing_port_id(queue_name, &qpid, &ret);
    if (ret != NO_ERROR)
        panic("recv queuing port process, get queuing port id failed.\n");
    
    char buff[200];
    message_size_t len;
    receive_queuing_message(qpid, INFINITE_TIME_VALUE, buff, &len, &ret); 
    if (ret != NO_ERROR)
        panic("recv queuing port process, recv queuing port failed.\n");

    cprintf("recv queuing port: %s.\n", buff);

    // idle
    while(1);    
}


/*
    create process area
*/

static void create_write_sampling_port_process(void) {
    return_code_t ret;
    process_id_t pid;

    process_attribute_t attr = DEFAULT_ATTR(write_sampling_port_process, "write_sampling");
    create_process(&attr, &pid, &ret);
    if (ret != NO_ERROR)
        panic("create write sampling port process failed.\n");
    cprintf("create write sampling port process done.\n");

    start(pid, &ret);
    if (ret != NO_ERROR)
        panic("start write sampling port process failed.\n");

}


static void create_read_sampling_port_process(void) {
    return_code_t ret;
    process_id_t pid;

    process_attribute_t attr = DEFAULT_ATTR(read_sampling_port_process, "read_sampling");
    create_process(&attr, &pid, &ret);
    if (ret != NO_ERROR)
        panic("create read sampling port process failed.\n");
    cprintf("create read sampling port process done.\n");

    start(pid, &ret);
    if (ret != NO_ERROR)
        panic("start read sampling port process failed.\n");
}


static void create_send_queuing_port_process(void) {
    return_code_t ret;
    process_id_t pid;

    process_attribute_t attr = DEFAULT_ATTR(send_queuing_port_process, "send_queuing");
    create_process(&attr, &pid, &ret);
    if (ret != NO_ERROR)
        panic("create send queuing port process failed.\n");
    cprintf("create send queuing port process done.\n");

    start(pid, &ret);
    if (ret != NO_ERROR)
        panic("start send queuing port process failed.\n");
}

static void create_recv_queuing_port_process(void) {
    return_code_t ret;
    process_id_t pid;

    process_attribute_t attr = DEFAULT_ATTR(recv_queuing_port_process, "recv_queuing");
    create_process(&attr, &pid, &ret);
    if (ret != NO_ERROR)
        panic("create recv queuing port process failed.\n");
    cprintf("create recv queuing port process done.\n");

    start(pid, &ret);
    if (ret != NO_ERROR)
        panic("start recv queuing port process failed.\n");
}

int main(void) {
    int pid, ppid;
    return_code_t ret;

    pid = getpid();
    ppid = get_partition_id();

    // create sampling port
    sampling_port_id_t spid;
    create_sampling_port(sample_name, 200, DESTINATION, 200, &spid, &ret);
    if (ret != NO_ERROR) {
        panic("create_sampling port failed.\n");
    }
    cprintf("create sampling port pass: %d.\n", spid);

    // create queuing port
    queuing_port_id_t qpid;
    create_queuing_port(queue_name, 200, 20, SOURCE, FIFO, &qpid, &ret);
    if (ret != NO_ERROR && ret != NO_ACTION)
        panic("create_queuing_port failed.\n");
    cprintf("create queuing port pass: %d.\n", qpid);


    create_write_sampling_port_process();

    // create_read_sampling_port_process();
    create_recv_queuing_port_process();


    set_partition_mode(NORMAL, &ret);
    // idle process
    while (1);
    return 0;
}