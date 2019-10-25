#include <blackboard.h>
#include <message.h>
#include <kmalloc.h>
#include <proc.h>
#include <string.h>
#include <sched.h>

inline static blackboard_id_t blackboard_id_generaotr(void) {
    return current->part->blackboard_id++;
}

static blackboard_t *alloc_blackboard(size_t max_len) {
    blackboard_t *blackboard;
    list_entry_t *le;
    list_entry_t *all_blackboard = &current->part->all_blackboard;
    list_entry_t *free_blackboard = &current->part->free_blackboard;

    if (!list_empty(free_blackboard)) {
        le = free_blackboard->next;
        list_del_init(le);
        blackboard = le2blackboard(le, bb_link);

        kfree(blackboard->buff);
    }
    else {
        blackboard = kmalloc(sizeof(blackboard_t));
        if (blackboard == NULL)
            return NULL;

        blackboard->id = blackboard_id_generaotr();
    }

    blackboard->buff = kmalloc(max_len);
    if (blackboard->buff == NULL) {
        kfree(blackboard);
        return NULL;
    }

    list_init(&blackboard->bb_link);
    list_init(&blackboard->waiting_thread);

    memset(blackboard->name, 0, sizeof(blackboard->name));
    memset(&blackboard->status, 0, sizeof(blackboard->status));

    list_add_after(all_blackboard, &blackboard->bb_link);
    current->part->blackboard_num++;

    return blackboard;
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
    blackboard_id_t id = *(blackboard_id_t*)pid;
    blackboard_t *blackboard = le2blackboard(le, bb_link);

    if (blackboard->id == id) {
        return 1;
    }
    return 0;
}

static int comp_name(list_entry_t *le, void *pname) {
    char *name = pname;
    blackboard_t *blackboard = le2blackboard(le, bb_link);

    if (strcmp(blackboard->name, name) == 0)
        return 1;
    
    return 0;
}


static blackboard_t *find_free_blackboard_id(blackboard_id_t id) {
    list_entry_t *le;
    blackboard_t *blackboard;
    if ((le = find_list(&current->part->free_blackboard, &id, comp_id)) == NULL) {
        return NULL;
    }

    blackboard = le2blackboard(le, bb_link); 
    return blackboard; 
}

static blackboard_t *find_free_blackboard_name(blackboard_name_t name) {
    list_entry_t *le;
    blackboard_t *blackboard;

    if ((le = find_list(&current->part->free_blackboard, name, comp_name)) == NULL) {
        return NULL;
    }

    blackboard = le2blackboard(le, bb_link);
    return blackboard;
}

static blackboard_t *get_blackboard_by_id(blackboard_id_t id) {
    list_entry_t *le;
    blackboard_t *blackboard;

    if ((le = find_list(&current->part->all_blackboard, &id, comp_id)) == NULL) {
        return NULL;
    }

    blackboard = le2blackboard(le, bb_link);

    if (find_free_blackboard_id(id) == NULL) {
        return blackboard;
    }
    
    return NULL;
}

static blackboard_t *get_blackboard_by_name(blackboard_name_t name) {
    list_entry_t *le;
    blackboard_t *blackboard;

    if ((le = find_list(&current->part->all_blackboard, name, comp_name)) == NULL) {
        return NULL;
    }

    blackboard = le2blackboard(le, bb_link);

    if (find_free_blackboard_name(name) == NULL) {
        return blackboard;
    }

    return NULL;
}


void do_create_blackboard(
    blackboard_name_t   blackboard_name,
    message_size_t      max_message_size,
    blackboard_id_t     *blackboard_id,
    return_code_t       *return_code)
{
    blackboard_t *blackboard;

    if (current->part->blackboard_num >= MAX_NUMBER_OF_BLACKBOARDS) {
        *return_code = INVALID_CONFIG;
        return;
    }

    blackboard = get_blackboard_by_name(blackboard_name);
    if (blackboard != NULL) {
        *return_code = NO_ACTION;
        return;
    }

    if (max_message_size <= 0) {
        *return_code = INVALID_PARAM;
        return;
    }

    if (current->part->status.operating_mode == NORMAL) {
        *return_code = INVALID_MODE;
        return;
    }

    blackboard = alloc_blackboard(max_message_size);
    if (blackboard == NULL) {
        *return_code = INVALID_CONFIG;
        return;
    }

    blackboard->status.max_message_size = max_message_size;
    blackboard->status.waiting_processes = 0;
    strcpy(blackboard->name, blackboard_name);
    current->part->blackboard_num++;

    *blackboard_id = blackboard->id;
    *return_code = NO_ERROR;
}

void do_display_blackboard(
    blackboard_id_t blackboard_id,
    message_addr_t  message_addr,
    message_size_t  length,
    return_code_t   *return_code)
{
    blackboard_t *board;
    if ((board = get_blackboard_by_id(blackboard_id)) == NULL) {
        *return_code = INVALID_PARAM;
        return;
    }

    if (length > board->status.max_message_size) {
        *return_code = INVALID_PARAM;
        return;
    }

    if (length <= 0) {
        *return_code = INVALID_PARAM;
        return;
    }

    board->status.empty_indicator = OCCUPIED;
    memcpy(board->buff, message_addr, length);
    board->length = length;

    struct proc_struct *proc;
    list_entry_t *le;
    if (!list_empty(&board->waiting_thread)) {
        le = board->waiting_thread.next;

        while (le != &board->waiting_thread) {
            proc = le2proc(le, run_link);
            if (proc->timer != NULL && test_wt_flag(proc, WT_TIMER)) {
                clear_wt_flag(proc, WT_TIMER);
                del_timer(proc->timer);
            }
            le = list_next(le);
            list_del_init(&proc->run_link);

            clear_wt_flag(proc, WT_BBOARD);
            wakeup_proc(proc);
            board->status.waiting_processes--;
        }

        if (PREEMPTION) {
            schedule();
        }
    }

    *return_code = NO_ERROR;
}

void do_read_blackboard(
    blackboard_id_t blackboard_id,
    system_time_t   time_out,
    message_addr_t  message_addr,
    message_size_t  *length,
    return_code_t   *return_code)
{
     blackboard_t *board;
    if ((board = get_blackboard_by_id(blackboard_id)) == NULL) {
        *return_code = INVALID_PARAM;
        return;
    }

    if (time_out > MAX_TIME_OUT) {
        *return_code = INVALID_PARAM;
        return;
    }

    if (board->status.empty_indicator == OCCUPIED) {
        memcpy(message_addr, board->buff, board->length);
        
        *length = board->length;
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

    else if (time_out == INFINITE_TIME_VALUE) {
        current->status.process_state = WAITING;
        list_del_init(&current->run_link);
        list_add_before(&board->waiting_thread, &current->run_link);
        set_wt_flag(current, WT_BBOARD);
        board->status.waiting_processes++;

        schedule();

        memcpy(message_addr, board->buff, board->length);
        
        *length = board->length;
        *return_code = NO_ERROR;
    }
    else {
        current->status.process_state = WAITING;
        list_del_init(&current->run_link);
        list_add_before(&board->waiting_thread, &current->run_link);
        set_wt_flag(current, WT_BBOARD);
        board->status.waiting_processes++;

        timer_t *timer = kmalloc(sizeof(timer_t));
        timer_init(timer, current, time_out);
        add_timer(timer);
        current->timer = timer;
        set_wt_flag(current, WT_TIMER);
        board->status.waiting_processes++;

        schedule();

        kfree(timer);
        if (current->timer == NULL) {
            *length = 0;
            *return_code = TIMED_OUT;
        }
        else {
            memcpy(message_addr, board->buff, board->length);
        
            *length = board->length;
            *return_code = NO_ERROR;
        }

    }

}

void do_clear_blackboard(
    blackboard_id_t blackboard_id,
    return_code_t   *return_code)
{
    blackboard_t *board;
    if ((board = get_blackboard_by_id(blackboard_id)) == NULL) {
        *return_code = INVALID_PARAM;
        return;
    }

    board->status.empty_indicator = EMPTY;

    *return_code = NO_ERROR;
}

void do_get_blackboard_id(
    blackboard_name_t   blackboard_name,
    blackboard_id_t     *blackboard_id,
    return_code_t       *return_code)
{
    blackboard_t *board;
    if ((board = get_blackboard_by_name(blackboard_name)) == NULL) {
        *return_code = INVALID_PARAM;
        return;
    }

    *blackboard_id = board->id;
    *return_code = NO_ERROR;
}

void do_get_blackboard_status(
    blackboard_id_t     blackboard_id,
    blackboard_status_t *blackboard_status,
    return_code_t       *return_code)
{
    blackboard_t *board;
    if ((board = get_blackboard_by_id(blackboard_id)) == NULL) {
        *return_code = INVALID_PARAM;
        return;
    }

    *blackboard_status = board->status;
    *return_code = NO_ERROR; 
}
