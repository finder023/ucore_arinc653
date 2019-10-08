#ifndef __L_ARINC_PROCESS_H
#define __L_ARINC_PROCESS_H

#include "../apex.h"

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

typedef struct {
    system_time_t       deadline_time;
    priority_t          current_priority;
    process_state_t     process_state;
    process_attribute_t attributes;
} process_status_type;

typedef process_status_type process_status_t;



#endif