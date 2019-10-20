#ifndef __L_USER_ARINC_TIME_H
#define __L_USER_ARINC_IIME_H

#include <apex.h>

void time_wait(system_time_t delay_time, return_code_t *return_code);

void periodic_wait(return_code_t *return_code);

void get_time(system_time_t *system_time, return_code_t *return_code);

void replenish(system_time_t budget_time, return_code_t *return_code);


#endif