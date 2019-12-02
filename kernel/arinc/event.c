#include <event.h>
#include <sync.h>
#include <sched.h>
#include <string.h>
#include <kmalloc.h>
#include <proc.h>

static event_t *get_event_by_name(const char *name) {
    list_entry_t *le = current->part->all_event.next;
    event_t *event;
    while (le != &(current->part->all_event)) {
        event = le2event(le, event_link);
        if (strcmp(name, event->event_name) == 0) {
            return event;
        }
        le = le->next;
    }
    return NULL;
}

static int is_event_free(event_id_t id) {
    list_entry_t *le = current->part->free_event.next;
    event_t *event;
    while ( le != &(current->part->free_event)) 
    {
        event = le2event(le, event_link);
        if (event->event_id == id) {
            return 1;
        }
        le = le->next;
    }
    return 0;
}

static event_t *get_event_by_id(event_id_t id) {
    list_entry_t *le = current->part->all_event.next;
    event_t *event;
    while ( le != &(current->part->all_event)) 
    {
        event = le2event(le, event_link);
        if (event->event_id == id) {
            if (is_event_free(id)) 
                return NULL;
            else
                return event;
        }
        le = le->next;
    }
    return NULL;
}

static event_id_t event_id_generator(void) {
    event_id_t id = current->part->event_id;
    current->part->event_id += 1;
    return id;
}

static event_t *alloc_event(void) {
    event_t *event;

    if (!list_empty(&current->part->free_event)) {
        list_entry_t *le = list_next(&current->part->free_event);
        list_del_init(le);
        event = le2event(le, event_link);
        return event;
    }

    if ((event = kmalloc(sizeof(event_t))) == NULL) {
        return NULL;
    }

    list_init(&event->event_link);
    list_init(&event->waiting_thread);
    event->event_id = event_id_generator();

    return event;
}


void do_reset_event( event_id_t event_id, return_code_t* return_code) {

    event_t* event = get_event_by_id(event_id);
    if ( event == NULL ) {
        *return_code = INVALID_PARAM;
        return;
    }
    event->status.event_state = DOWN;
    *return_code = NO_ERROR;
}

void do_wait_event( event_id_t event_id, system_time_t time_out, return_code_t* return_code) {

    event_t* event = get_event_by_id(event_id);
    if ( event == NULL ) {
        *return_code = INVALID_PARAM;
        return;
    }
    if ( time_out > MAX_TIME_OUT ) {
        *return_code = INVALID_PARAM;
        return;
    }
    if ( event->status.event_state == UP ) {
        *return_code = NO_ERROR;
        return;
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
        set_wt_flag(current, WT_EVENT);
        list_add_before(&event->waiting_thread, &current->run_link);
        event->status.waiting_processes = event->status.waiting_processes + 1;
        schedule();
        *return_code = NO_ERROR;
    }
    else {
        // set_proc_waiting
        current->status.process_state = WAITING;
        list_del_init(&current->run_link);
        set_wt_flag(current, WT_EVENT);
        list_add_before(&event->waiting_thread, &current->run_link);
        event->status.waiting_processes = event->status.waiting_processes + 1;
        // add_timer
        timer_t *timer = kmalloc(sizeof(timer_t));
        timer_init(timer, current, time_out);
        set_wt_flag(current, WT_TIMER);
        add_timer(timer);
        current->timer = timer;
        schedule();
        if ( current->timer == NULL ) {
            *return_code = TIMED_OUT;
        }
        else {
            current->timer = NULL;
            *return_code = NO_ERROR;
        }
    }
}

void do_get_event_status( event_id_t event_id, event_status_t* event_status, return_code_t* return_code) {

    event_t* event = get_event_by_id(event_id);
    if ( event == NULL ) {
        *return_code = INVALID_PARAM;
        return;
    }
    *event_status = event->status;
    *return_code = NO_ERROR;
}

void do_set_event( event_id_t event_id, return_code_t* return_code) {

    event_t* event = get_event_by_id(event_id);
    struct proc_struct* proc = current;
    if ( event == NULL ) {
        *return_code = INVALID_PARAM;
        return;
    }
    event->status.event_state = UP;
    // stop_all_timer
    list_entry_t *stle = event->waiting_thread.next;
    while ( stle != &event->waiting_thread ) {
        proc = le2proc(stle, run_link);
        if ( proc->status.process_state == WAITING && test_wt_flag(proc, WT_TIMER) ) {
            clear_wt_flag(proc, WT_TIMER);
            timer_t* timer = proc->timer;
            del_timer(timer);
            kfree(timer);
        }
        stle = list_next(stle);
    }
    // wakeup_waiting_proc
    list_entry_t *wwle = event->waiting_thread.next;
    while ( wwle != &event->waiting_thread ) {
        proc = le2proc(wwle, run_link);
        if ( proc->status.process_state == WAITING && test_wt_flag(proc, WT_EVENT) ) {
            clear_wt_flag(proc, WT_EVENT);
            list_del(&proc->run_link);
            if ( proc->wait_state == 0 ) {
                wakeup_proc(proc);
            }
        }
        wwle = list_next(wwle);
    }
    event->status.waiting_processes = 0;
    if ( PREEMPTION != 0 ) {
        schedule();
    }
}

void do_get_event_id( event_name_t event_name, event_id_t* event_id, return_code_t* return_code) {

    event_t* event = get_event_by_name(event_name);
    if ( event == NULL ) {
        *return_code = INVALID_CONFIG;
        return;
    }
    *event_id = event->event_id;
    *return_code = NO_ERROR;
}

void do_create_event( event_name_t event_name, event_id_t* event_id, return_code_t* return_code) {

    partition_t* part = current->part;
    if ( part->event_num > MAX_NUMBER_OF_EVENTS ) {
        *return_code = INVALID_CONFIG;
        return;
    }
    event_t* event = get_event_by_name(event_name);
    if ( event != NULL ) {
        *return_code = NO_ACTION;
        return;
    }
    if ( part->status.operating_mode == NORMAL ) {
        *return_code = INVALID_MODE;
        return;
    }
    event = alloc_event();
    if ( event == NULL ) {
        *return_code = INVALID_CONFIG;
        return;
    }
    *event_id = event->event_id;
    strcpy(event->event_name, event_name);
    // add_event
    list_add_after(&part->all_event, &event->event_link);
    part->event_num = part->event_num + 1;
    *return_code = NO_ERROR;
}

