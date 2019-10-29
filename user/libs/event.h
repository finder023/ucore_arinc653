#ifndef __L_USER_EVENT_H
#define __L_USER_EVENT_H

#include <apex.h>
#include <list.h>

#define MAX_NUMBER_OF_EVENTS    SYSTEM_LIMIT_NUMBER_OF_EVENTS

typedef name_t          event_name_type;
typedef apex_integer_t  event_id_type;
typedef enum {DOWN = 0, UP = 1} event_state_type;

typedef event_name_type     event_name_t;
typedef event_id_type       event_id_t;
typedef event_state_type    event_state_t;

typedef struct event_status_type {
    event_state_t   event_state;
    apex_integer_t  waiting_processes;
} event_status_type;

typedef event_status_type event_status_t;

typedef struct event_type {
    event_status_t  status;
    event_id_t      event_id;
    list_entry_t    waiting_thread;
    list_entry_t    event_link;
    event_name_t    event_name;
} event_type;

typedef event_type event_t;


void create_event(event_name_t event_name, event_id_t *event_id, return_code_t *return_code);

void set_event(event_id_t event_id, return_code_t *return_code);

void reset_event(event_id_t event_id, return_code_t *return_code);

void wait_event(event_id_t event_id, system_time_t time_out, return_code_t *return_code);

void get_event_id(event_name_t event_name, event_id_t *event_id, return_code_t *return_code);

void get_event_status(event_id_t event_id, event_status_t *event_status, return_code_t *return_code);



#endif