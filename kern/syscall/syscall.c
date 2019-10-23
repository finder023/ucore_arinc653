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
#include <event.h>
#include <arinc_time.h>
#include <sampling_port.h>
#include <queuing_port.h>
#include <buffer.h>
#include <blackboard.h>


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
    return_code_t   *return_code = (return_code_t*)arg[1];
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

// event

static int sys_create_event(uint32_t arg[]) {
    event_name_t event_name;
    strcpy(event_name, (char*)arg[0]);
    event_id_t *event_id = (event_id_t*)arg[1]; 
    return_code_t *return_code = (return_code_t*)arg[2];
    do_create_event(event_name, event_id, return_code);
    return *return_code;
}

static int sys_set_event(uint32_t arg[])
{
    event_id_t event_id = (event_id_t)arg[0];
    return_code_t *return_code = (return_code_t*)arg[1];
    do_set_event(event_id, return_code);
    return *return_code;
}

static int sys_reset_event(uint32_t arg[]) {
    event_id_t event_id = (event_id_t)arg[0];
    return_code_t *return_code = (return_code_t*)arg[1];
    do_reset_event(event_id, return_code);
    return *return_code;
}

static int sys_wait_event(uint32_t arg[]) {
    event_id_t event_id = (event_id_t)arg[0];
    system_time_t time_out = (system_time_t)arg[1];
    return_code_t *return_code = (return_code_t*)arg[2];
    do_wait_event(event_id, time_out, return_code);
    return *return_code;
}

static int sys_get_event_id(uint32_t arg[]) {
    event_name_t event_name;
    strcpy(event_name, (char*)arg[0]);
    event_id_t *event_id = (event_id_t*)arg[1];
    return_code_t *return_code = (return_code_t*)arg[2];
    do_get_event_id(event_name, event_id, return_code);
    return *return_code;
}

static int sys_get_event_status(uint32_t arg[]) {
    event_id_t event_id = (event_id_t)arg[0];
    event_status_t *event_status = (event_status_t*)arg[1];
    return_code_t *return_code = (return_code_t*)arg[2];
    do_get_event_status(event_id, event_status, return_code);
    return *return_code;
}


static int sys_get_partition_status(uint32_t arg[]) {
    partition_status_t *status = (partition_status_t*)arg[0];
    return_code_t *return_code = (return_code_t*)arg[1];
    do_get_partition_status(status, return_code);
    return *return_code;
}

static int sys_set_partition_mode(uint32_t arg[]) {
    operating_mode_t mode = (operating_mode_t)arg[0];
    return_code_t *return_code = (return_code_t*)arg[1];
    do_set_partition_mode(mode, return_code);
    return *return_code;
}

static int sys_time_wait(uint32_t arg[]) {
    system_time_t delay_time = arg[0];
    return_code_t *return_code = (return_code_t *)arg[1];
    do_time_wait(delay_time, return_code);
    
    return *return_code;
}

static int sys_periodic_wait(uint32_t arg[]) {
    return_code_t *return_code = (return_code_t*)arg[0];
    do_periodic_wait(return_code);

    return *return_code;
}

static int sys_get_time(uint32_t arg[]) {
    system_time_t *system_time = (system_time_t*)arg[0];
    return_code_t *return_code = (return_code_t*)arg[1];
    do_get_time(system_time, return_code);

    return *return_code;
}

static int sys_replenish(uint32_t arg[]) {
    system_time_t budget_time = arg[0];
    return_code_t *return_code = (return_code_t*)arg[1];
    do_replenish(budget_time, return_code);

    return *return_code;
} 

// sampling port
static int sys_create_sampling_port(uint32_t arg[]) {
    sampling_port_name_t name;
    strcpy(name, (char*)arg[0]);
    sampling_port_status_t *status = (sampling_port_status_t*)arg[1];
    message_size_t max_msg_size = status->max_message_size;
    port_direction_t port_direction = status->port_direction;
    system_time_t refresh_period = status->refresh_period; 
    sampling_port_id_t *sampling_port_id = (sampling_port_id_t*)arg[2];
    return_code_t *return_code = (return_code_t*)arg[3];

    do_create_sampling_port(name, max_msg_size, port_direction, refresh_period,
        sampling_port_id, return_code);

    return *return_code;
}


static int sys_write_sampling_message(uint32_t arg[]) {
    sampling_port_id_t sampling_port_id = arg[0];
    message_addr_t msg_addr = (message_addr_t)arg[1];
    message_size_t length = arg[2];
    return_code_t *return_code = (return_code_t*)arg[3];

    do_write_sampling_message(sampling_port_id, msg_addr, length, return_code);
    return *return_code;
}

static int sys_read_sampling_message(uint32_t arg[]) {
    sampling_port_id_t sampling_port_id = arg[0]; 
    message_addr_t msg_addr = (message_addr_t)arg[1];
    message_size_t *length = (message_size_t*)arg[2];
    validity_t *validity = (validity_t*)arg[3];
    return_code_t *return_code = (return_code_t*)arg[4];

    do_read_sampling_message(sampling_port_id, msg_addr, length, validity,
            return_code);

    return *return_code;
}

static int sys_get_sampling_port_id(uint32_t arg[]) {
    sampling_port_name_t name;
    strcpy(name, (char*)arg[0]);
    sampling_port_id_t *sampling_port_id = (sampling_port_id_t*)arg[1];
    return_code_t *return_code = (return_code_t*)arg[2];

    do_get_sampling_port_id(name, sampling_port_id, return_code);
    return *return_code;
}

static int sys_get_sampling_port_status(uint32_t arg[]) {
    sampling_port_id_t sampling_port_id = arg[0];
    sampling_port_status_t *sampling_port_status = (sampling_port_status_t*)arg[1];
    return_code_t *return_code = (return_code_t*)arg[2];

    do_get_sampling_port_status(sampling_port_id, sampling_port_status, return_code);
    return *return_code;
}

static int sys_create_queuing_port(uint32_t arg[]) {
    queuing_port_name_t name;
    strcpy(name, (char*)arg[0]);
    queuing_port_status_t *status = (queuing_port_status_t*)arg[1];
    message_size_t max_msg_size = status->max_message_size;
    message_range_t max_nb_msg = status->max_nb_message;
    port_direction_t port_direction = status->port_direction;
    queuing_discipline_t queuing_discipline = (queuing_discipline_t)arg[2];
    queuing_port_id_t *id = (queuing_port_id_t*)arg[3];
    return_code_t *return_code = (return_code_t*)arg[4];

    do_create_queuing_port(name, max_msg_size, max_nb_msg, port_direction,
        queuing_discipline, id, return_code);
    return *return_code;
}

static int sys_send_queuing_message(uint32_t arg[]) {
    queuing_port_id_t id = arg[0];
    message_addr_t msg_addr = (message_addr_t)arg[1];
    message_size_t length = arg[2];
    system_time_t time_out = arg[3];
    return_code_t *return_code = (return_code_t*)return_code;

    do_send_queuing_message(id, msg_addr, length, time_out, return_code);
    return *return_code;
}

static int sys_receive_queuing_message(uint32_t arg[]) {
    queuing_port_id_t id = arg[0];
    system_time_t time_out = arg[1];
    message_addr_t message_addr = (message_addr_t)arg[2];
    message_size_t *length = (message_size_t*)arg[3];
    return_code_t *return_code = (return_code_t*)arg[4];

    do_receive_queuing_message(id, time_out, message_addr, length, return_code);
    return *return_code;
}

static int sys_get_queuing_port_id(uint32_t arg[]) {
    queuing_port_name_t name;
    strcpy(name, (char*)arg[0]);
    queuing_port_id_t *id = (queuing_port_id_t*)arg[1]; 
    return_code_t *return_code = (return_code_t*)arg[2];

    do_get_queuing_port_id(name, id, return_code);
    return *return_code;
}

static int sys_get_queuing_port_status(uint32_t arg[]) {
    queuing_port_id_t id = arg[0];
    queuing_port_status_t *status = (queuing_port_status_t*)arg[1];
    return_code_t *return_code = (return_code_t*)arg[2];

    do_get_queuing_port_status(id, status, return_code);
    return *return_code;
}

static int sys_clear_queuing_port(uint32_t arg[]) {
    queuing_port_id_t id = arg[0];
    return_code_t *return_code = (return_code_t*)arg[1];

    do_clear_queuing_port(id, return_code);
    return *return_code;
}

// buffer
static int sys_create_buffer(uint32_t arg[]) {
    buffer_name_t buffer_name;
    strcpy(buffer_name, (char*)arg[0]);
    message_size_t max_message_size = arg[1];
    message_range_t max_nb_message = arg[2];
    queuing_discipline_t queuing_discipline = (queuing_discipline_t)arg[3];
    buffer_id_t *buffer_id = (buffer_id_t*)arg[4];
    return_code_t return_code;

    do_create_buffer(buffer_name, max_message_size, max_nb_message,
        queuing_discipline, buffer_id, &return_code);
    return return_code;
}


static int sys_send_buffer(uint32_t arg[]) {
    buffer_id_t    buffer_id = arg[0];
    message_addr_t message_addr = (message_addr_t)arg[1];
    message_size_t length = arg[2];
    system_time_t  time_out = arg[3];
    return_code_t  *return_code = (return_code_t*)arg[4];

    do_send_buffer(buffer_id, message_addr, length, time_out, return_code);
    return *return_code;
}

static int sys_receive_buffer(uint32_t arg[]) {
    buffer_id_t    buffer_id = arg[0];
    system_time_t  time_out = arg[1];
    message_addr_t message_addr = (message_addr_t)arg[2];
    message_size_t *length = (message_size_t*)arg[3];
    return_code_t  *return_code = (return_code_t*)arg[4];

    do_receive_buffer(buffer_id, time_out, message_addr, length, return_code);
    return *return_code;
}

static int sys_get_buffer_id(uint32_t arg[]) {
    buffer_name_t  buffer_name;
    strcpy(buffer_name, (char*)arg[0]);
    buffer_id_t    *buffer_id = (buffer_id_t*)arg[1];
    return_code_t  *return_code = (return_code_t*)arg[2];

    do_get_buffer_id(buffer_name, buffer_id, return_code);
    return *return_code;
}

static int sys_get_buffer_status(uint32_t arg[]) {
    buffer_id_t        buffer_id = arg[0];
    buffer_status_t    *buffer_status = (buffer_status_t*)arg[1];
    return_code_t      *return_code = (return_code_t*)arg[2];

    do_get_buffer_status(buffer_id, buffer_status, return_code);
    return *return_code;
}

// blackboard
static int sys_create_blackboard(uint32_t arg[]) {
    blackboard_name_t   blackboard_name;
    strcpy(blackboard_name, (char*)arg[0]);
    message_size_t      max_message_size = arg[1];
    blackboard_id_t     *blackboard_id = (blackboard_id_t*)arg[2];
    return_code_t       *return_code = (return_code_t*)arg[3];

    do_create_blackboard(blackboard_name, max_message_size, blackboard_id,
        return_code);
    return *return_code;
}

static int sys_display_blackboard(uint32_t arg[]) {
    blackboard_id_t blackboard_id = arg[0];
    message_addr_t  message_addr = (message_addr_t)arg[1];
    message_size_t  length = arg[2];
    return_code_t   *return_code = (return_code_t*)arg[3];

    do_display_blackboard(blackboard_id, message_addr, length, return_code);
    return *return_code;
}


static int sys_read_blackboard(uint32_t arg[]) {
    blackboard_id_t blackboard_id = arg[0];
    system_time_t   time_out = arg[1];
    message_addr_t  message_addr = (message_addr_t)arg[2];
    message_size_t  *length = (message_size_t*)arg[3];
    return_code_t   *return_code = (return_code_t*)arg[4];

    do_read_blackboard(blackboard_id, time_out, message_addr, length, 
        return_code);

    return *return_code;
}

static int sys_clear_blackboard(uint32_t arg[]) {
    blackboard_id_t blackboard_id = arg[0];
    return_code_t   *return_code = (return_code_t*)arg[1];

    do_clear_blackboard(blackboard_id, return_code);
    return *return_code;
}

static int sys_get_blackboard_id(uint32_t arg[]) {
    blackboard_name_t   blackboard_name;
    strcpy(blackboard_name, (char*)arg[0]);
    blackboard_id_t     *blackboard_id = (blackboard_id_t*)arg[1];
    return_code_t       *return_code = (return_code_t*)arg[2];

    do_get_blackboard_id(blackboard_name, blackboard_id, return_code);
    return *return_code;
}

static int sys_get_blackboard_status(uint32_t arg[]) {
    blackboard_id_t     blackboard_id = arg[0];
    blackboard_status_t *blackboard_status = (blackboard_status_t*)arg[1];
    return_code_t       *return_code = (return_code_t*)arg[2];

    do_get_blackboard_status(blackboard_id, blackboard_status, return_code);
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
    [SYS_createevent]           sys_create_event,
    [SYS_setevent]              sys_set_event,
    [SYS_resetevent]            sys_reset_event,
    [SYS_waitevent]             sys_wait_event,
    [SYS_geteventid]            sys_get_event_id,
    [SYS_geteventstatus]        sys_get_event_status,
    [SYS_getpartitionstatus]    sys_get_partition_status,
    [SYS_setpartitionstatus]    sys_set_partition_mode,
    [SYS_timewait]              sys_time_wait,
    [SYS_periodicwait]          sys_periodic_wait,
    [SYS_arincgettime]          sys_get_time,
    [SYS_replenish]             sys_replenish,
    [SYS_createsamplingport]    sys_create_sampling_port,
    [SYS_writesamplingmessage]  sys_write_sampling_message,
    [SYS_readsamplingmessage]   sys_read_sampling_message,
    [SYS_getsamplingportid]     sys_get_sampling_port_id,
    [SYS_getsamplingportstatus] sys_get_sampling_port_status,
    [SYS_createqueuingport]     sys_create_queuing_port,
    [SYS_sendqueuingmessage]    sys_send_queuing_message, 
    [SYS_receivequeuingmessage] sys_receive_queuing_message,
    [SYS_getqueuingportid]      sys_get_queuing_port_id,
    [SYS_getqueuingportstatus]  sys_get_queuing_port_status,
    [SYS_clearqueuingport]      sys_clear_queuing_port,
    [SYS_createbuffer]          sys_create_buffer,
    [SYS_sendbuffer]            sys_send_buffer,
    [SYS_receivebuffer]         sys_receive_buffer,
    [SYS_getbufferid]           sys_get_buffer_id,
    [SYS_getbufferstatus]       sys_get_buffer_status,
    [SYS_createblackboard]      sys_create_blackboard,
    [SYS_displayblackboard]     sys_display_blackboard,
    [SYS_readblackboard]        sys_read_blackboard,
    [SYS_clearblackboard]       sys_clear_blackboard,
    [SYS_getblackboardid]       sys_get_blackboard_id,
    [SYS_getblackboardstatus]   sys_get_blackboard_status,
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

