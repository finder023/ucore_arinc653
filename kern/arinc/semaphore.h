#ifndef __L_SEMAPHORE_H
#define __L_SEMAPHORE_H

#include <defs.h>
#include <list.h>
#include <apex.h>

#define MAX_NUMBER_OF_SEMAPHORES SYSTEM_LIMIT_NUMBER_OF_SEMAPHORES
#define MAX_SEMAPHORE_VALUE 32767
#define SEM_MAX_TIMEOUT 1000

typedef name_t              semaphore_name_t;
typedef apex_integer_t      semaphore_id_t;
typedef apex_integer_t      semaphore_value_t;
typedef apex_integer_t      waiting_range_t;

typedef struct semaphore_status_type {
    semaphore_value_t   current_value;
    semaphore_value_t   max_value;
    waiting_range_t     waiting_processes;
} semaphore_status_t;

typedef struct semaphore_status_type semaphore_status_type;


typedef struct sem_type {
    semaphore_status_t  sem_status;
    semaphore_name_t    sem_name;
    semaphore_id_t      sem_id;
    list_entry_t        waiting_thread;
    list_entry_t        sem_link;
} sem_t;

typedef struct sem_type sem_type;

#define le2sem(le, member)  to_struct((le), sem_t, member)

void do_create_semaphore (
    semaphore_name_t     semaphore_name,
    semaphore_value_t    current_value,
    semaphore_value_t    max_value,
    queuing_discipline_t queuing_discipline,
    semaphore_id_t       *semphore_id,
    return_code_t        *return_code);

void do_wait_semaphore (
    semaphore_id_t  semaphore_id,
    system_time_t   time_out,
    return_code_t   *return_code
);

void do_signal_semaphore(
    semaphore_id_t  semaphore_id,
    return_code_t   *return_code
);

void do_get_semaphore_id (
    semaphore_name_t    semaphore_name,
    semaphore_id_t      *semaphore_id,
    return_code_t       *return_code
);

void do_get_semaphore_status (
    semaphore_id_t      semaphore_id,
    semaphore_status_t  *semaphore_status,
    return_code_t       *return_code
);


#endif