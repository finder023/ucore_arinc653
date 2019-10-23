#ifndef __L_ARINC_PARTITION_H
#define __L_ARINC_PARTITION_H

#include <apex.h>
#include <proc.h>
#include <list.h>

#define MAX_NUMBER_OF_PARTITIONS SYSTEM_LIMIT_NUMBER_OF_PARTITIONS

typedef enum {
    IDLE = 0,
    COLD_START = 1,
    WARM_START = 2,
    NORMAL = 3
} OPERATING_MODE_TYPE;

typedef OPERATING_MODE_TYPE operating_mode_t;

typedef enum {
    NORMAL_START = 0,
    PARTITION_RESTART = 1,
    HM_MODULE_RESTART = 2,
    HM_PARTITION_RESTART = 3
} START_CONDITION_TYPE;

typedef struct {
    SYSTEM_TIME_TYPE        peroid;
    SYSTEM_TIME_TYPE        duration;
    PARTITION_ID_TYPE       identifier;
    LOCK_LEVEL_TYPE         lock_level;
    OPERATING_MODE_TYPE     operating_mode;
    START_CONDITION_TYPE    start_condition;
} PARTITION_STATUS_TYPE;

typedef START_CONDITION_TYPE    start_condition_t;

typedef PARTITION_STATUS_TYPE   partition_status_t;

typedef struct partition_type {
    partition_status_t  status;
    struct mm_struct    *mm;
    list_entry_t        part_tag;
    list_entry_t        proc_set;
    list_entry_t        run_list;
    list_entry_t        dormant_set;
    list_entry_t        timeout_set;
    system_time_t       deadline;
    int                 scheduling;
    int                 proc_num;
    struct proc_struct  *idle_proc;
    int                 first_run;

    // semaphore
    list_entry_t        all_sem;
    list_entry_t        free_sem;
    int                 sem_id;
    int                 sem_num;

    // event
    list_entry_t        all_event;
    list_entry_t        free_event;
    int                 event_id;
    int                 event_num;

    // buffer
    list_entry_t        all_buffer;
    list_entry_t        free_buffer;
    int                 buffer_id;
    int                 buffer_num;

} partition_t;

#define le2part(le, member) \
    to_struct((le), partition_t, member);

void partition_init(void);

partition_t *partition_add(int *pid);

partition_t *next_partition(void);

partition_t *get_partition(int ppid);

void check_timeout(partition_t *part);

void do_get_partition_status(partition_status_t *status, return_code_t *return_code);

void do_set_partition_mode(operating_mode_t mode, return_code_t *return_code);

#endif