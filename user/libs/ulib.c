#include <defs.h>
#include <syscall.h>
#include <stdio.h>
#include <ulib.h>
#include <arinc_proc.h>
#include <semaphore.h>
#include <event.h>
#include <arinc_time.h>
#include <sampling_port.h>
#include <queuing_port.h>
#include <buffer.h>

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


void create_sampling_port(sampling_port_name_t name, message_size_t max_msg_size,
        port_direction_t port_direction, system_time_t refresh_period, 
        sampling_port_id_t *sampling_port_id, return_code_t *return_code)
{
    sys_create_sampling_port(name, max_msg_size, port_direction, refresh_period,
        sampling_port_id, return_code);
}

void write_sampling_message(sampling_port_id_t sampling_port_id,
    message_addr_t msg_addr, message_size_t length, return_code_t *return_code)
{
    sys_write_sampling_message(sampling_port_id, msg_addr, length, return_code);
}

void read_sampling_message(sampling_port_id_t sampling_port_id, 
        message_addr_t msg_addr, message_size_t *length, validity_t *validity,
        return_code_t *return_code)
{
    sys_read_sampling_message(sampling_port_id, msg_addr, length, validity,
        return_code);
}

void get_sampling_port_id(sampling_port_name_t name, 
        sampling_port_id_t *sampling_port_id, return_code_t *return_code)
{
    sys_get_sampling_port_id(name, sampling_port_id, return_code);
}

void get_sampling_port_status(sampling_port_id_t sampling_port_id,
    sampling_port_status_t *sampling_port_status, return_code_t *return_code)
{
    sys_get_sampling_port_status(sampling_port_id, sampling_port_status,
        return_code);
}

void create_queuing_port(queuing_port_name_t name, message_size_t max_msg_size,
        message_range_t max_nb_msg, port_direction_t port_direction,
        queuing_discipline_t queuing_discipline, queuing_port_id_t *id,
        return_code_t *return_code)
{
    sys_create_queuing_port(name, max_msg_size, max_nb_msg, port_direction,
        queuing_discipline, id, return_code);
}

void send_queuing_message(queuing_port_id_t id, message_addr_t msg_addr,
        message_size_t length, system_time_t time_out,
        return_code_t *return_code)
{
    sys_send_queuing_message(id, msg_addr, length, time_out, return_code);
}

void receive_queuing_message(queuing_port_id_t id, system_time_t time_out,
        message_addr_t message_addr, message_size_t *length,
        return_code_t *return_code)
{
    sys_receive_queuing_message(id, time_out, message_addr, length,
        return_code);
}

void get_queuing_port_id(queuing_port_name_t name, queuing_port_id_t *id, 
        return_code_t *return_code)
{
    sys_get_queuing_port_id(name, id, return_code);
}

void get_queuing_port_status(queuing_port_id_t id,
    queuing_port_status_t *status, return_code_t *return_code)
{
    sys_get_queuing_port_status(id, status, return_code);
}

void clear_queuing_port(queuing_port_id_t id, return_code_t *return_code)
{
    sys_clear_queuing_port(id, return_code);
}

void create_buffer(
    buffer_name_t      buffer_name,
    message_size_t     max_message_size,
    message_range_t    max_nb_message,
    queuing_discipline_t   queuing_discipline,
    buffer_id_t        *buffer_id,
    return_code_t      *return_code)
{
    sys_create_buffer(buffer_name, max_message_size, max_nb_message,
        queuing_discipline, buffer_id, return_code);
}


void do_send_buffer(
    buffer_id_t    buffer_id,
    message_addr_t message_addr,
    message_size_t length,
    system_time_t  time_out,
    return_code_t  *return_code)
{
    sys_send_buffer(buffer_id, message_addr, length, time_out, return_code);
}

void do_receive_buffer(
    buffer_id_t    buffer_id,
    system_time_t  time_out,
    message_addr_t message_addr,
    message_size_t *length,
    return_code_t  *return_code)
{
    sys_receive_buffer(buffer_id, time_out, message_addr, length, return_code);
}

void do_get_buffer_id(
    buffer_name_t  buffer_name,
    buffer_id_t    *buffer_id,
    return_code_t  *return_code )
{
    sys_get_buffer_id(buffer_name, buffer_id, return_code);
}

void do_get_buffer_status(
    buffer_id_t        buffer_id,
    buffer_status_t    *buffer_status,
    return_code_t      *return_code)
{
    sys_get_buffer_status(buffer_id, buffer_status, return_code);
}
