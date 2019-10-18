#ifndef __L_ARINC_PROCESS_H
#define __L_ARINC_PROCESS_H

#include <apex.h>
#include <defs.h>

#define MAX_NUMBER_OF_PROCESSES SYSTEM_LIMIT_NUMBER_OF_PROCESSES

#define MIN_PRIORITY_VALUE 1

#define MAX_PRIORITY_VALUE 239

#define MAX_LOCK_LEVEL 16

typedef enum {
    DORMANT = 0,
    READY = 1,
    RUNNING = 2,
    WAITTING = 3
} process_state_type;

typedef enum {
    SOFT = 0,
    HARD = 1
} deadline_type;

typedef process_state_type process_state_t;

typedef deadline_type   deadline_t;

typedef name_t  process_name_t;

typedef apex_integer_t  process_id_t;

typedef apex_unsigned_t stack_size_t;

typedef apex_integer_t  priority_t;

typedef struct {
    system_time_t       period;
    system_time_t       time_capacity;
    system_address_t    entry_point;
    stack_size_t        stack_size;
    priority_t          base_priority;
    deadline_t          deadline;
    process_name_t      name;
} process_attribute_type;

typedef process_attribute_type  process_attribute_t;

#define DEFAULT_ATTR(_entry, _name) {\
    .period = INFINITE_TIME_VALUE,  \
    .time_capacity = 5,\
    .entry_point = _entry, \
    .stack_size = 4 * 4096, \
    .base_priority = 1, \
    .deadline = SOFT,   \
    .name = _name,  \
}

typedef struct {
    system_time_t       deadline_time;
    priority_t          current_priority;
    process_state_t     process_state;
    process_attribute_t attributes;
} process_status_type;

typedef process_status_type process_status_t;



void create_process(process_attribute_t *attributes, 
                    process_id_t *process_id,   
                    return_code_t *return_code);

void set_priority(process_id_t process_id,
                    uint8_t priority,
                    return_code_t *return_code);

void suspend_self(uint32_t time_out,
                    return_code_t *return_code);
            
void suspend(process_id_t process_id, return_code_t *return_code);

void resume(process_id_t process_id, return_code_t *return_code);

void stop_self(void);

void stop(process_id_t process_id, return_code_t *return_code);

void start(process_id_t process_id, return_code_t *return_code);

void delayed_start(process_id_t process_id,
                    system_time_t delay_time,
                    return_code_t *return_code);

void lock_preemption(lock_level_t *lock_level, return_code_t *return_code);

void unlock_preemption(lock_level_t *lock_level, return_code_t *return_code);

void get_my_id(process_id_t *process_id, return_code_t *return_code);

void get_process_id(process_name_t  process_name,
                    process_id_t    *process_id,
                    return_code_t   *return_code);

void get_process_status(process_id_t process_id,
                    process_status_t    *process_status,
                    return_code_t       *return_code);


#endif