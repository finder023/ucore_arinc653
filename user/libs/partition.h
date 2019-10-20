#ifndef __L_USER_PARTITION_H
#define __L_USER_PARTITION_H

#include <apex.h>

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

void get_partition_status(partition_status_t *status, return_code_t *return_code);

void set_partition_mode(operating_mode_t mode, return_code_t *return_code);

#endif