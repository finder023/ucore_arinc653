#include <buffer.h>
#include <kmalloc.h>
#include <proc.h>
#include <string.h>
#include <sched.h>

inline static buffer_id_t buffer_id_generaotr(void) {
    return current->part->buffer_id++;
}

static buffer_t *alloc_buffer(void) {
    buffer_t *buffer;
    list_entry_t *le;
    list_entry_t *all_buffer = &current->part->all_buffer;
    list_entry_t *free_buffer = &current->part->free_buffer;

    if (!list_empty(free_buffer)) {
        le = free_buffer->next;
        list_del_init(le);
        buffer = le2buffer(le, buffer_link);

        le = buffer->msg_set.next;
        message_t *msg;
        while (le != &buffer->msg_set) {
            msg = le2msg(le, msg_link);
            free_message(msg);
            le = list_next(le);
        }
    }
    else {
        buffer = kmalloc(sizeof(buffer_t));
        if (buffer == NULL)
            return NULL;

        buffer->id = buffer_id_generaotr();
    }

    list_init(&buffer->msg_set);
    list_init(&buffer->buffer_link);
    list_init(&buffer->waiting_thread);

    memset(buffer->name, 0, sizeof(buffer->name));
    memset(&buffer->status, 0, sizeof(buffer->status));

    list_add_after(all_buffer, &buffer->buffer_link);
    current->part->buffer_num++;

    return buffer;
}


static list_entry_t *find_list(list_entry_t *list, void *target,
                                int (*comp)(list_entry_t*, void*)) {
    if (list_empty(list)) {
        return NULL;
    }

    list_entry_t *le = list_next(list);
    while (le != list) {
        if (comp(le, target)) {
            return le;
        }
        le = list_next(le);
    }

    return NULL;
}

static int comp_id(list_entry_t *le, void *pid) {
    buffer_id_t id = *(buffer_id_t*)pid;
    buffer_t *buffer = le2buffer(le, buffer_link);

    if (buffer->id == id) {
        return 1;
    }
    return 0;
}

static int comp_name(list_entry_t *le, void *pname) {
    char *name = pname;
    buffer_t *buffer = le2buffer(le, buffer_link);

    if (strcmp(buffer->name, name) == 0)
        return 1;
    
    return 0;
}


static buffer_t *find_free_buffer_id(buffer_id_t id) {
    list_entry_t *le;
    buffer_t *buffer;
    if ((le = find_list(&current->part->free_buffer, &id, comp_id)) == NULL) {
        return NULL;
    }

    buffer = le2buffer(le, buffer_link); 
    return buffer; 
}

static buffer_t *find_free_buffer_name(buffer_name_t name) {
    list_entry_t *le;
    buffer_t *buffer;

    if ((le = find_list(&current->part->free_buffer, name, comp_name)) == NULL) {
        return NULL;
    }

    buffer = le2buffer(le, buffer_link);
    return buffer;
}

static buffer_t *get_buffer_by_id(buffer_id_t id) {
    list_entry_t *le;
    buffer_t *buffer;

    if ((le = find_list(&current->part->all_buffer, &id, comp_id)) == NULL) {
        return NULL;
    }

    buffer = le2buffer(le, buffer_link);

    if (find_free_buffer_id(id) == NULL) {
        return buffer;
    }
    
    return NULL;
}

static buffer_t *get_buffer_by_name(buffer_name_t name) {
    list_entry_t *le;
    buffer_t *buffer;

    if ((le = find_list(&current->part->all_buffer, name, comp_name)) == NULL) {
        return NULL;
    }

    buffer = le2buffer(le, buffer_link);

    if (find_free_buffer_name(name) == NULL) {
        return buffer;
    }

    return NULL;
}


void do_create_buffer( buffer_name_t buffer_name, message_size_t max_message_size, message_range_t max_nb_message, queuing_discipline_t queuing_discipline, buffer_id_t* buffer_id, return_code_t* return_code) {

    partition_t* part = current->part;
    if ( part->buffer_num >= MAX_NUMBER_OF_BUFFERS ) {
        *return_code = INVALID_CONFIG;
        return;
    }
    buffer_t* buffer = get_buffer_by_name(buffer_name);
    if ( buffer != NULL ) {
        *return_code = NO_ACTION;
        return;
    }
    if ( max_message_size <= 0 ) {
        *return_code = INVALID_CONFIG;
        return;
    }
    if ( max_nb_message > MAX_NUMBER_OF_MESSAGE ) {
        *return_code = INVALID_PARAM;
        return;
    }
    if ( queuing_discipline != FIFO && queuing_discipline != PRIORITY ) {
        *return_code = INVALID_PARAM;
        return;
    }
    if ( part->status.operating_mode == NORMAL ) {
        *return_code = INVALID_MODE;
        return;
    }
    buffer = alloc_buffer();
    if ( buffer == NULL ) {
        *return_code = INVALID_CONFIG;
        return;
    }
    buffer->discipline = queuing_discipline;
    strcpy(buffer->name, buffer_name);
    buffer->status.max_message_size = max_message_size;
    buffer->status.max_nb_message = max_nb_message;
    // add_buffer
    list_add_after(&part->all_buffer, &buffer->buffer_link);
    part->buffer_num = part->buffer_num + 1;
    *buffer_id = buffer->id;
    *return_code = NO_ERROR;
}

void do_get_buffer_id( buffer_name_t buffer_name, buffer_id_t* buffer_id, return_code_t* return_code) {

    buffer_t* buffer = get_buffer_by_name(buffer_name);
    if ( buffer == NULL ) {
        *return_code = INVALID_CONFIG;
        return;
    }
    *buffer_id = buffer->id;
    *return_code = NO_ERROR;
}

void do_receive_buffer( buffer_id_t buffer_id, system_time_t time_out, message_addr_t message_addr, message_size_t* len, return_code_t* return_code) {

    buffer_t* buffer = get_buffer_by_id(buffer_id);
    struct proc_struct* proc = current;
    if ( buffer == NULL ) {
        *return_code = INVALID_PARAM;
        return;
    }
    if ( time_out > MAX_TIME_OUT ) {
        *return_code = INVALID_PARAM;
        return;
    }
    message_t* msg = NULL;
    if ( buffer->status.nb_message != 0 ) {
        // remove_message
        list_entry_t *rmle = buffer->msg_set.next;
        list_del_init(rmle);
        msg = le2msg(rmle, msg_link);
        memcpy(message_addr, msg->buff, msg->length);
        buffer->status.nb_message = buffer->status.nb_message - 1;
        *len = msg->length;
        if ( buffer->status.waiting_processes != 0 ) {
            // select_waiting_proc
            list_entry_t *elem = buffer->waiting_thread.next;
            struct proc_struct *proc = le2proc(elem, run_link);
            list_del_init(&proc->run_link);
            if ( proc->timer != NULL ) {
                // stop_timer
                timer_t *timer = proc->timer;
                del_timer(timer);
                clear_wt_flag(proc, WT_TIMER);
                kfree(timer);
            }
            clear_wt_flag(proc, WT_BUFFER);
            wakeup_proc(proc);
            buffer->status.waiting_processes = buffer->status.waiting_processes - 1;
            if ( PREEMPTION != 0 ) {
                schedule();
            }
        }
        *return_code = NO_ERROR;
    }
    else if ( time_out == 0) {
        *len = 0;
        *return_code = NO_ERROR;
    }
    else if ( PREEMPTION == 0) {
        *len = 0;
        *return_code = INVALID_MODE;
    }
    else if ( time_out == INFINITE_TIME_VALUE) {
        // set_proc_waiting
        current->status.process_state = WAITING;
        list_del_init(&current->run_link);
        set_wt_flag(current, WT_BUFFER);
        list_add_before(&buffer->waiting_thread, &current->run_link);
        buffer->status.waiting_processes = buffer->status.waiting_processes + 1;
        schedule();
        // remove_message
        list_entry_t *rmle = buffer->msg_set.next;
        list_del_init(rmle);
        msg = le2msg(rmle, msg_link);
        memcpy(message_addr, msg->buff, msg->length);
        buffer->status.nb_message = buffer->status.nb_message - 1;
        *len = msg->length;
        *return_code = NO_ERROR;
    }
    else {
        // add_timer
        timer_t *timer = kmalloc(sizeof(timer_t));
        timer_init(timer, current, time_out);
        set_wt_flag(current, WT_TIMER);
        add_timer(timer);
        current->timer = timer;
        // set_proc_waiting
        current->status.process_state = WAITING;
        list_del_init(&current->run_link);
        set_wt_flag(current, WT_BUFFER);
        list_add_before(&buffer->waiting_thread, &current->run_link);
        buffer->status.waiting_processes = buffer->status.waiting_processes + 1;
        schedule();
        clear_wt_flag(proc, WT_BUFFER);
        if ( current->timer == NULL ) {
            *len = 0;
            *return_code = TIMED_OUT;
        }
        else {
            // remove_message
            list_entry_t *rmle = buffer->msg_set.next;
            list_del_init(rmle);
            msg = le2msg(rmle, msg_link);
            memcpy(message_addr, msg->buff, msg->length);
            buffer->status.nb_message = buffer->status.nb_message - 1;
            *len = msg->length;
            *return_code = NO_ERROR;
        }
    }
}

void do_send_buffer( buffer_id_t buffer_id, message_addr_t message_addr, message_size_t len, system_time_t time_out, return_code_t* return_code) {

    buffer_t* buffer = get_buffer_by_id(buffer_id);
    message_t* msg = NULL;
    if ( buffer == NULL ) {
        *return_code = INVALID_PARAM;
        return;
    }
    if ( len > buffer->status.max_message_size ) {
        *return_code = INVALID_PARAM;
        return;
    }
    if ( len <= 0 ) {
        *return_code = INVALID_PARAM;
        return;
    }
    if ( time_out > MAX_TIME_OUT ) {
        *return_code = INVALID_PARAM;
        return;
    }
    if ( buffer->status.nb_message < buffer->status.max_nb_message ) {
        if ( buffer->status.waiting_processes == 0 ) {
            msg = alloc_message(buffer->status.max_message_size);
            msg->length = len;
            memcpy(msg->buff, message_addr, len);
            list_add_before(&buffer->msg_set, &msg->msg_link);
            buffer->status.nb_message = buffer->status.nb_message + 1;
        }
        else {
            // select_waiting_proc
            list_entry_t *elem = buffer->waiting_thread.next;
            struct proc_struct *proc = le2proc(elem, run_link);
            list_del_init(&proc->run_link);
            clear_wt_flag(proc, WT_BUFFER);
            wakeup_proc(proc);
            buffer->status.waiting_processes = buffer->status.waiting_processes - 1;
            if ( test_wt_flag(proc, WT_TIMER) ) {
                // stop_timer
                timer_t *timer = proc->timer;
                del_timer(timer);
                clear_wt_flag(proc, WT_TIMER);
                kfree(timer);
            }
            msg = alloc_message(buffer->status.max_message_size);
            msg->length = len;
            memcpy(msg->buff, message_addr, len);
            list_add_before(&buffer->msg_set, &msg->msg_link);
            buffer->status.nb_message = buffer->status.nb_message + 1;
            if ( PREEMPTION != 0 ) {
                schedule();
            }
            *return_code = NO_ERROR;
        }
    }
    else if ( time_out == 0) {
        *return_code = NOT_AVAILABLE;
        return;
    }
    else if ( PREEMPTION == 0) {
        *return_code = INVALID_MODE;
        return;
    }
    else if ( time_out == INFINITE_TIME_VALUE) {
        // set_proc_waiting
        current->status.process_state = WAITING;
        list_del_init(&current->run_link);
        set_wt_flag(current, WT_BUFFER);
        list_add_before(&buffer->waiting_thread, &current->run_link);
        buffer->status.waiting_processes = buffer->status.waiting_processes + 1;
        schedule();
        msg = alloc_message(buffer->status.max_message_size);
        msg->length = len;
        memcpy(msg->buff, message_addr, len);
        list_add_before(&buffer->msg_set, &msg->msg_link);
        buffer->status.nb_message = buffer->status.nb_message + 1;
        *return_code = NO_ERROR;
    }
    else {
        // set_proc_waiting
        current->status.process_state = WAITING;
        list_del_init(&current->run_link);
        set_wt_flag(current, WT_BUFFER);
        list_add_before(&buffer->waiting_thread, &current->run_link);
        // add_timer
        timer_t *timer = kmalloc(sizeof(timer_t));
        timer_init(timer, current, WT_TIMER);
        set_wt_flag(current, WT_TIMER);
        add_timer(timer);
        current->timer = timer;
        buffer->status.waiting_processes = buffer->status.waiting_processes + 1;
        schedule();
        if ( current->timer == NULL ) {
            *return_code = TIMED_OUT;
        }
        else {
            current->timer = NULL;
            msg = alloc_message(buffer->status.max_message_size);
            msg->length = len;
            memcpy(msg->buff, message_addr, len);
            list_add_before(&buffer->msg_set, &msg->msg_link);
            buffer->status.nb_message = buffer->status.nb_message + 1;
            *return_code = NO_ERROR;
        }
    }
}

void do_get_buffer_status( buffer_id_t buffer_id, buffer_status_t* buffer_status, return_code_t* return_code) {

    buffer_t* buffer = get_buffer_by_id(buffer_id);
    if ( buffer == NULL ) {
        *return_code = INVALID_PARAM;
        return;
    }
    *buffer_status = buffer->status;
    *return_code = NO_ERROR;
}

