#ifndef __L_USER_BLACKBOARD_H
#define __L_USER_BLACKBOARD_H

#include <apex.h>
#include <defs.h>

typedef name_t          blackboard_name_type;
typedef apex_integer_t  blackboard_id_type;
typedef enum {EMPTY = 0, OCCUPIED = 1} empty_indicator_type;

typedef blackboard_id_type      blackboard_id_t;
typedef blackboard_name_type    blackboard_name_t;
typedef empty_indicator_type    empty_indicator_t;

typedef struct blackboard_status_type {
    empty_indicator_t   empty_indicator;
    message_size_t      max_message_size;
    size_t              waiting_processes;
} blackboard_status_type;

typedef blackboard_status_type  blackboard_status_t;


void create_blackboard(
    blackboard_name_t   blackboard_name,
    message_size_t      max_message_size,
    blackboard_id_t     *blackboard_id,
    return_code_t       *return_code
);

void display_blackboard(
    blackboard_id_t blackboard_id,
    message_addr_t  message_addr,
    message_size_t  length,
    return_code_t   *return_code
);

void read_blackboard(
    blackboard_id_t blackboard_id,
    system_time_t   time_out,
    message_addr_t  message_addr,
    message_size_t  *length,
    return_code_t   *return_code
);

void clear_blackboard(
    blackboard_id_t blackboard_id,
    return_code_t   *return_code
);

void get_blackboard_id(
    blackboard_name_t   blackboard_name,
    blackboard_id_t     *blackboard_id,
    return_code_t       *return_code
);

void get_blackboard_status(
    blackboard_id_t     blackboard_id,
    blackboard_status_t *blackboard_status,
    return_code_t       *return_code
);


#endif