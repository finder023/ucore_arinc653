#ifndef __L_USER_SAMPLING_PORT_H
#define __L_USER_SAMPLING_PORT_H

#include <apex.h>


#define MAX_NUMBER_OF_SAMPLING_PORTS SYSTEM_LIMIT_NUMBER_OF_SAMPLING_PORTS

typedef NAME_TYPE SAMPLING_PORT_NAME_TYPE;
typedef APEX_INTEGER SAMPLING_PORT_ID_TYPE;


typedef SAMPLING_PORT_NAME_TYPE sampling_port_name_t;

typedef SAMPLING_PORT_ID_TYPE   sampling_port_id_t;


typedef struct {
    system_time_t   refresh_period;
    message_size_t  max_message_size;
    port_direction_t    port_direction;
    validity_t      last_msg_validity;
} SAMPLING_PORT_STATUS_TYPE;

typedef SAMPLING_PORT_STATUS_TYPE   sampling_port_status_t;


void create_sampling_port(sampling_port_name_t name, message_size_t max_msg_size,
        port_direction_t port_direction, system_time_t refresh_period, 
        sampling_port_id_t *sampling_port_id, return_code_t *return_code);

void write_sampling_message(sampling_port_id_t sampling_port_id,
        message_addr_t msg_addr, message_size_t length, return_code_t *return_code);

void read_sampling_message(sampling_port_id_t sampling_port_id, 
        message_addr_t msg_addr, message_size_t *length, validity_t *validity,
        return_code_t *return_code);

void get_sampling_port_id(sampling_port_name_t name, 
        sampling_port_id_t *sampling_port_id, return_code_t *return_code);

void get_sampling_port_status(sampling_port_id_t sampling_port_id,
        sampling_port_status_t *sampling_port_status, return_code_t *return_code);


#endif