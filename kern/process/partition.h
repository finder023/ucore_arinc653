#ifndef __L_ARINC_PARTITION_H
#define __L_ARINC_PARTITION_H

#include "../apex.h"
#include <proc.h>
#include <list.h>

#define MAX_NUMBER_OF_PARTITIONS SYSTEM_LIMIT_NUMBER_OF_PARTITIONS

typedef enum {
    IDLE = 0,
    COLD_START = 1,
    WARM_START = 2,
    NORMAL = 3
} OPERATING_MODE_TYPE;

typedef APEX_INTEGER PARTITION_ID_TYPE;
typedef APEX_INTEGER    LOCK_LEVEL_TYPE;


typedef PARTITION_ID_TYPE   partition_id_t;
typedef LOCK_LEVEL_TYPE     lock_level_t;

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

typedef struct {
    partition_status_t  status;
    struct mm_struct    *mm;
    list_entry_t        part_tag;
    list_entry_t        proc_set;
    struct proc_struct  *idle_proc;
} partition_t;


void partition_init(void);

int partition_add(int pid);

#endif