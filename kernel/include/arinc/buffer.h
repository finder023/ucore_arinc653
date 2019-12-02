#ifndef __L_APEX_BUFFER_H
#define __L_APEX_BUFFER_H

#include <apex.h>
#include <defs.h>
#include <list.h>
#include <message.h>

#define MAX_NUMBER_OF_BUFFERS SYSTEM_LIMIT_NUMBER_OF_BUFFERS
#define MAX_NUMBER_OF_MESSAGE 32

typedef name_t  buffer_name_type;
typedef apex_integer_t   buffer_id_type;

typedef struct buffer_status_type {
    message_range_t     nb_message;
    message_range_t     max_nb_message;
    message_size_t      max_message_size;
    size_t              waiting_processes;
} buffer_status_type;

typedef buffer_name_type   buffer_name_t;
typedef buffer_id_type     buffer_id_t;
typedef buffer_status_type buffer_status_t;

typedef struct buffer_type {
    buffer_status_t         status;
    buffer_name_t           name;
    buffer_id_t             id;
    list_entry_t            buffer_link;
    list_entry_t            msg_set;
    list_entry_t            waiting_thread;
    queuing_discipline_t    discipline;
} buffer_t;


#define le2buffer(le, member)   (to_struct(le, buffer_t, member))

void do_create_buffer(
    buffer_name_t      buffer_name,
    message_size_t     max_message_size,
    message_range_t    max_nb_message,
    queuing_discipline_t   queuing_discipline,
    buffer_id_t        *buffer_id,
    return_code_t      *return_code
);


void do_send_buffer(
    buffer_id_t    buffer_id,
    message_addr_t message_addr,
    message_size_t length,
    system_time_t  time_out,
    return_code_t  *return_code 
);

void do_receive_buffer(
    buffer_id_t    buffer_id,
    system_time_t  time_out,
    message_addr_t message_addr,
    message_size_t *length,
    return_code_t  *return_code
);

void do_get_buffer_id(
    buffer_name_t  buffer_name,
    buffer_id_t    *buffer_id,
    return_code_t  *return_code 
);

void do_get_buffer_status(
    buffer_id_t        buffer_id,
    buffer_status_t    *buffer_status,
    return_code_t      *return_code
);

#endif