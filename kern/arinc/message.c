#include <message.h>
#include <kmalloc.h>
#include <list.h>

message_t *alloc_message(size_t max_size) {
    message_t *msg = kmalloc(sizeof(message_t));
    if (msg == NULL) {
        return NULL;
    }

    list_init(&msg->msg_link);
    msg->buff = kmalloc(max_size);
    if (msg->buff == NULL) {
        kfree(msg);
        return NULL;
    }

    return msg;
}

void free_message(message_t *msg) {
    if (!msg)
        return;

    list_del(&msg->msg_link); 
    kfree(msg->buff);
    kfree(msg);
}