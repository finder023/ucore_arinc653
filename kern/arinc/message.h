#ifndef __L_MESSAGE_H
#define __L_MESSAGE_H

#include <defs.h>
#include <list.h>

typedef struct message_type {
    size_t          length;
    list_entry_t    msg_link;
    void            *buff;
} message_t;

#define le2msg(le, member)  (to_struct(le, message_t, member))


message_t *alloc_message(size_t max_size);

void free_message(message_t *msg);

#endif