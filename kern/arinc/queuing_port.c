#include <queuing_port.h>
#include <string.h>
#include <proc.h>
#include <partition.h>
#include <sync.h>
#include <clock.h>
#include <kmalloc.h>
#include <sched.h>
#include <stdio.h>

// all queuing port set
static list_entry_t all_queuing_port;

// free queuing port set
static list_entry_t free_queuing_port;

// queuing port id
static uint32_t _queuing_port_id;

// queuing num
static uint32_t nr_queuing_port;


inline static queuing_port_id_t queuing_port_id_generator(void) {
    return ++_queuing_port_id;
}

static message_t *alloc_message(size_t max_size) {
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

static void free_message(message_t *msg) {
    if (!msg)
        return;

    list_del(&msg->msg_link); 
    kfree(msg->buff);
    kfree(msg);
}

static queuing_port_t* alloc_queuing_port(size_t max_size) {
    queuing_port_t *queue;
    list_entry_t *le;

    if (!list_empty(&free_queuing_port)) {
        le = free_queuing_port.next;
        list_del_init(le);
        queue = le2queue(le, list_link);
        
        le = queue->msg_set.next;
        message_t *msg;
        while (le != &queue->msg_set) {
            msg = le2msg(le, msg_link);
            le = list_next(le);
            free_message(msg);
        }
    }
    else {
        queue = kmalloc(sizeof(queuing_port_t));
        if (queue == NULL) {
            return NULL;
        }
        queue->id = queuing_port_id_generator();
    }

    list_init(&queue->msg_set);
    list_init(&queue->waitting_thread);
    memset(queue->name, 0, sizeof(queue->name));
    memset(&queue->status, 0, sizeof(queuing_port_status_t));
    list_add_after(&all_queuing_port, &queue->list_link);
    nr_queuing_port++;

    return queue;
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
    queuing_port_id_t id = *(queuing_port_id_t*)pid;
    queuing_port_t *queue = le2queue(le, list_link);
    
    if (queue->id == id) {
        return 1;
    }
    return 0;
}

static int comp_name(list_entry_t *le, void *pname) {
    char *name = pname;
    queuing_port_t *queue = le2queue(le, list_link);

    if (strcmp(queue->name, name) == 0)
        return 1;
    
    return 0;
}


static queuing_port_t *find_free_queuing_port_id(queuing_port_id_t id) {
    list_entry_t *le;
    queuing_port_t *queue;
    if ((le = find_list(&free_queuing_port, &id, comp_id)) == NULL) {
        return NULL;
    }

    queue = le2queue(le, list_link); 
    return queue; 
}

static queuing_port_t *find_free_queuing_port_name(queuing_port_name_t name) {
    list_entry_t *le;
    queuing_port_t *queue;

    if ((le = find_list(&free_queuing_port, name, comp_name)) == NULL) {
        return NULL;
    }

    queue = le2queue(le, list_link);
    return queue;
}

static queuing_port_t *get_queue_by_id(queuing_port_id_t id) {
    list_entry_t *le;
    queuing_port_t *queue;

    if ((le = find_list(&all_queuing_port, &id, comp_id)) == NULL) {
        return NULL;
    }

    queue = le2queue(le, list_link);

    if (find_free_queuing_port_id(id) == NULL) {
        return queue;
    }
    
    return NULL;
}

static queuing_port_t *get_queue_by_name(queuing_port_name_t name) {
    list_entry_t *le;
    queuing_port_t *queue;

    if ((le = find_list(&all_queuing_port, name, comp_name)) == NULL) {
        return NULL;
    }

    queue = le2queue(le, list_link);

    if (find_free_queuing_port_name(name) == NULL) {
        return queue;
    }

    return NULL;
}



void do_create_queuing_port(queuing_port_name_t name, message_size_t max_msg_size,
        message_range_t max_nb_msg, port_direction_t port_direction,
        queuing_discipline_t queuing_discipline, queuing_port_id_t *id,
        return_code_t *return_code)
{
    queuing_port_t *queue;
    if (nr_queuing_port >= MAX_NUMBER_OF_QUEUING_PORTS) {
        *return_code = INVALID_CONFIG;
        return;
    }

    queue = get_queue_by_name(name);
    if (queue != NULL) {
        *return_code = NO_ACTION;
        return;
    }

    if (max_msg_size <= 0) {
        *return_code = INVALID_CONFIG;
        return;
    }

    if (max_nb_msg > SYSTEM_LIMIT_NUMBER_OF_MESSAGES) {
        *return_code = INVALID_CONFIG;
        return;
    }

    if (port_direction != SOURCE && port_direction != DESTINATION) {
        *return_code = INVALID_CONFIG;
        return;
    }

    if (queuing_discipline != FIFO && queuing_discipline != PRIORITY) {
        *return_code = INVALID_CONFIG;
        return;    
    }

    if (current->part->status.operating_mode == NORMAL) {
        *return_code = INVALID_MODE;
        return;
    }

    queue = alloc_queuing_port(max_msg_size);
    if (queue == NULL) {
        *return_code = INVALID_CONFIG;
        return;
    }

    list_add_after(&all_queuing_port, &queue->list_link);
    queue->status.max_message_size = max_msg_size;
    queue->status.port_direction = port_direction;
    queue->status.nb_message = 0;
    queue->status.max_nb_message = max_nb_msg;
    queue->status.waiting_process = 0;
    strcpy(queue->name, name);

    *return_code = NO_ERROR;
}

void do_send_queuing_message(queuing_port_id_t id, message_addr_t msg_addr,
        message_size_t length, system_time_t time_out,
        return_code_t *return_code)
{
    queuing_port_t *queue = get_queue_by_id(id);
    if (queue == NULL) {
        *return_code = INVALID_PARAM;
        return;
    }

    if (time_out > MAX_TIME_OUT) {
        *return_code = INVALID_PARAM;
        return;
    }

    if (length > queue->status.max_message_size) {
        *return_code = INVALID_CONFIG;
        return;
    }

    if (length <= 0) {
        *return_code = INVALID_PARAM;
        return;
    }

//    if (queue->status.port_direction != SOURCE) {
//        *return_code = INVALID_MODE;
//        return;
//    }

    bool old_intr;
    local_intr_save(old_intr);

    message_t *msg = alloc_message(queue->status.max_message_size);
    if (queue->status.max_nb_message > queue->status.nb_message) {
        memcpy(msg->buff, msg_addr, length);
        msg->length = length;
        list_add_before(&queue->msg_set, &msg->msg_link);
        queue->status.nb_message++;
        local_intr_restore(old_intr);

        if (queue->status.nb_message == 1) {
            list_entry_t *le;
            struct proc_struct *proc;
            if (!list_empty(&queue->waitting_thread)) {
                le = queue->waitting_thread.next;
                list_del_init(le);
                proc = le2proc(le, run_link);
                queue->status.waiting_process--;
                wakeup_proc(proc);
                schedule();
            }
        }

        *return_code = NO_ERROR;
    }
    else if (time_out == 0) {
        local_intr_restore(old_intr);
        *return_code = NOT_AVAILABLE;
    }
    else if (!PREEMPTION) {
        local_intr_restore(old_intr);
        *return_code = INVALID_MODE;
    }
    else {
        timer_t *timer = NULL;
        if (time_out != INFINITE_TIME_VALUE) {
            timer = kmalloc(sizeof(timer_t));
            timer_init(timer, current, time_out);
            add_timer(timer);
            set_wt_flag(current, WT_TIMER);
        }

        current->status.process_state = WAITTING;
        list_del_init(&current->run_link);
        list_add_after(&queue->waitting_thread, &current->run_link);
        set_wt_flag(current, WT_QUEUE);
        queue->status.waiting_process++; 
        local_intr_restore(old_intr);

        schedule();

        if (timer) {
            del_timer(timer);
            kfree(timer);
        }
        if (current->timer == NULL && test_wt_flag(current, WT_TIMER)) {
            clear_wt_flag(current, WT_QUEUE | WT_TIMER);
            queue->status.waiting_process--;
            *return_code = TIMED_OUT;
        }
        else {
            if (time_out != INFINITE_TIME_VALUE) {
                clear_wt_flag(current, WT_TIMER);
                current->timer = NULL;
            }

            clear_wt_flag(current, WT_QUEUE);
            memcpy(msg->buff, msg_addr, length);
            msg->length = length;
            list_add_before(&queue->msg_set, &msg->msg_link);
            queue->status.nb_message++;
            
            *return_code = NO_ERROR;
        }
    }
}

void do_receive_queuing_message(queuing_port_id_t id, system_time_t time_out,
        message_addr_t message_addr, message_size_t *length,
        return_code_t *return_code)
{
    queuing_port_t *queue = get_queue_by_id(id);
    if (queue == NULL) {
        *return_code = INVALID_PARAM;
        return;
    }

    if (time_out > MAX_TIME_OUT) {
        *return_code = INVALID_PARAM;
        return;
    }

//    if (queue->status.port_direction != DESTINATION) {
//        *return_code = INVALID_MODE;
//        return;
//    }


    list_entry_t *le;
    message_t *msg;
    if (!list_empty(&queue->msg_set)) {
        le = queue->msg_set.next;
        list_del_init(le);
        msg = le2msg(le, msg_link);
        memcpy(message_addr, msg->buff, msg->length);
        *length = msg->length;
        queue->status.nb_message--;

        list_entry_t *le;
        struct proc_struct *proc;
        if (queue->status.nb_message + 1 == SYSTEM_LIMIT_NUMBER_OF_MESSAGES) {
            if (!list_empty(&queue->waitting_thread)) {
                le = queue->waitting_thread.next;
                proc = le2proc(le, run_link);
                list_del_init(le);
                queue->status.waiting_process--;
                wakeup_proc(proc);

                schedule();
            }
        }

        *return_code = NO_ERROR;
    }
    else if (time_out == 0) {
        *length = 0;
        *return_code = NOT_AVAILABLE;
    }
    else if (!PREEMPTION) {
        *length = 0;
        *return_code = INVALID_MODE;
    }
    else {
        timer_t *timer = NULL;
        if (time_out != INFINITE_TIME_VALUE) {
            timer = kmalloc(sizeof(timer_t));
            timer_init(timer, current, time_out);
            add_timer(timer);
            set_wt_flag(current, WT_TIMER);
        }
        current->status.process_state = WAITTING;
        list_del_init(&current->run_link);
        list_add_after(&queue->waitting_thread, &current->run_link);
        set_wt_flag(current, WT_QUEUE);
        queue->status.waiting_process++;

        schedule();

        if (timer) {
            del_timer(timer);
            kfree(timer);
        }

        if (current->timer == NULL && test_wt_flag(current, WT_TIMER)) {
            *length = 0;
            queue->status.waiting_process--;
            clear_wt_flag(current, WT_QUEUE | WT_TIMER);
            *return_code = TIMED_OUT;
        }
        else {
            if (time_out != INFINITE_TIME_VALUE) {
                clear_wt_flag(current, WT_TIMER | WT_QUEUE);
                current->timer = NULL;
            }

            clear_wt_flag(current, WT_QUEUE);
            list_entry_t *le = queue->msg_set.next;
            assert(le != &queue->msg_set);
            message_t *msg = le2msg(le, msg_link);
            list_del_init(le);
            memcpy(message_addr, msg->buff, msg->length);
            *length = msg->length;
            queue->status.nb_message--;

            free_message(msg);
            *return_code = NO_ERROR;
        }
    }
}

void do_get_queuing_port_id(queuing_port_name_t name, queuing_port_id_t *id, 
        return_code_t *return_code)
{
    queuing_port_t *queue = get_queue_by_name(name);
    if (queue == NULL) {
        *return_code = INVALID_CONFIG;
        return;
    }

    *id = queue->id;
    *return_code = NO_ERROR;
    return;
}

void do_get_queuing_port_status(queuing_port_id_t id, queuing_port_status_t *status,
        return_code_t *return_code)
{
    queuing_port_t *queue = get_queue_by_id(id);
    if (queue == NULL) {
        *return_code = INVALID_PARAM;
        return;
    }

    *status = queue->status;
    *return_code = NO_ERROR;
}

void do_clear_queuing_port(queuing_port_id_t id, return_code_t *return_code)
{
    queuing_port_t *queue = get_queue_by_id(id);
    if (queue == NULL) {
        *return_code = INVALID_PARAM;
        return;
    }

    if (queue->status.port_direction != DESTINATION) {
        *return_code = INVALID_MODE;
        return;
    }

    list_entry_t *le = queue->msg_set.next;
    message_t *msg;

    while (le != &queue->msg_set) {
        msg = le2msg(le, msg_link);
        free_message(msg);
        le = list_next(le);
    }

    queue->status.nb_message = 0;
    *return_code = NO_ERROR;
}


void queuing_port_init(void) {
    list_init(&all_queuing_port);
    list_init(&free_queuing_port);

    _queuing_port_id = 0;
    nr_queuing_port = 0;
}
