#ifndef __L_BLACKBOARD_H
#define __L_BLACKBOARD_H

#include <apex.h>
#include <list.h>
#include <defs.h>


#define MAX_NUMBER_OF_BLACKBOARDS    SYSTEM_LIMIT_NUMBER_OF_BLACKBOARDS

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


typedef struct blackboard_type {
    blackboard_status_t status;
    blackboard_id_t     id;
    blackboard_name_t   name;
    message_size_t      length;
    message_addr_t      buff;
    list_entry_t        waiting_thread;
    list_entry_t        bb_link;
} blackboard_type;

typedef blackboard_type blackboard_t;

#define le2blackboard(le, member)   (to_struct(le, blackboard_t, member))

void do_create_blackboard(
    blackboard_name_t   blackboard_name,
    message_size_t      max_message_size,
    blackboard_id_t     *blackboard_id,
    return_code_t       *return_code
);

void do_display_blackboard(
    blackboard_id_t blackboard_id,
    message_addr_t  message_addr,
    message_size_t  length,
    return_code_t   *return_code
);

void do_read_blackboard(
    blackboard_id_t blackboard_id,
    system_time_t   time_out,
    message_addr_t  message_addr,
    message_size_t  *length,
    return_code_t   *return_code
);

void do_clear_blackboard(
    blackboard_id_t blackboard_id,
    return_code_t   *return_code
);

void do_get_blackboard_id(
    blackboard_name_t   blackboard_name,
    blackboard_id_t     *blackboard_id,
    return_code_t       *return_code
);

void do_get_blackboard_status(
    blackboard_id_t     blackboard_id,
    blackboard_status_t *blackboard_status,
    return_code_t       *return_code
);


#endif