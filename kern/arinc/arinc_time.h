#ifndef __ARINC_TIME_H
#define __ARINC_TIME_H

#include <apex.h>

#define MAX_DELAY_TIME  1000

void do_time_wait(system_time_t delay_time, return_code_t *return_code);

void do_periodic_wait(return_code_t *return_code);

void do_get_time(system_time_t *system_time, return_code_t *return_code);

void do_replenish(system_time_t budget_time, return_code_t *return_code);

#endif