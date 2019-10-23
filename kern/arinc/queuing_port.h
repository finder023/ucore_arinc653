#ifndef __L_QUEUING_PORT_H
#define __L_QUEUING_PORT_H

#include <apex.h>
#include <defs.h>
#include <list.h>
#include <message.h>

#define MAX_NUMBER_OF_QUEUING_PORTS SYSTEM_LIMIT_NUMBER_OF_QUEUING_PORTS

typedef name_t QUEUING_PORT_NAME_TYPE;
typedef apex_integer_t QUEUING_PORT_ID_TYPE;

typedef QUEUING_PORT_NAME_TYPE  queuing_port_name_t;
typedef QUEUING_PORT_ID_TYPE    queuing_port_id_t;

typedef struct {
    message_range_t     nb_message;
    message_range_t     max_nb_message;
    message_size_t      max_message_size;
    port_direction_t    port_direction;
    size_t              waiting_process; 
} QUEUING_PORT_STATUS_TYPE;

typedef QUEUING_PORT_STATUS_TYPE queuing_port_status_t;


typedef struct queuing_port_type {
    queuing_port_status_t   status;
    queuing_port_name_t     name;
    queuing_port_id_t       id;
    queuing_discipline_t    discipline;
    list_entry_t            list_link;
    list_entry_t            msg_set;
    list_entry_t            waitting_thread;
} queuing_port_t;

#define le2queue(le, member)    (to_struct(le, queuing_port_t, member))

void do_create_queuing_port(queuing_port_name_t name, message_size_t max_msg_size,
        message_range_t max_nb_msg, port_direction_t port_direction,
        queuing_discipline_t queuing_discipline, queuing_port_id_t *id,
        return_code_t *return_code);

void do_send_queuing_message(queuing_port_id_t id, message_addr_t msg_addr,
        message_size_t length, system_time_t time_out,
        return_code_t *return_code);

void do_receive_queuing_message(queuing_port_id_t id, system_time_t time_out,
        message_addr_t message_addr, message_size_t *length,
        return_code_t *return_code);

void do_get_queuing_port_id(queuing_port_name_t name, queuing_port_id_t *id, 
        return_code_t *return_code);

void do_get_queuing_port_status(queuing_port_id_t id, queuing_port_status_t *status,
        return_code_t *return_code);

void do_clear_queuing_port(queuing_port_id_t id, return_code_t *return_code);


void queuing_port_init(void);


#endif