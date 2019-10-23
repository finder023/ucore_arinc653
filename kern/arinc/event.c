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
void do_create_event(event_name_t event_name, event_id_t *event_id, return_code_t *return_code) {
    event_t *event;
    partition_t *part = current->part;
    if (part->event_num > MAX_NUMBER_OF_EVENTS) {
        *return_code = INVALID_CONFIG;
        return;
    }

    if ((event = get_event_by_name(event_name)) != NULL) {
        *return_code = NO_ACTION;
        return;
    }

    // TODO: operating mode normal
    if (part->status.operating_mode == NORMAL) {
        *return_code = INVALID_MODE;
        return;
    }

    event = alloc_event();
    if (event == NULL) {
        *return_code = INVALID_CONFIG;
        return;
    }

    *event_id = event->event_id;
    strcpy(event->event_name, event_name);
    event->status.event_state = DOWN;
    list_add_after(&part->all_event, &event->event_link);
    part->event_num += 1;

    *return_code = NO_ERROR;
}

void do_set_event(event_id_t event_id, return_code_t *return_code) {
    event_t *event;
    if ((event = get_event_by_id(event_id)) == NULL) {
        *return_code = INVALID_PARAM;
        return;
    }

    bool old_intr;
    local_intr_save(old_intr);
    event->status.event_state = UP;

    list_entry_t *le;
    struct proc_struct *proc;

    while (!list_empty(&event->waiting_thread)) {
        le = event->waiting_thread.next;
        list_del_init(le);
        proc = le2proc(le, run_link);
        if (proc->timer != NULL) {
            del_timer(proc->timer);
            clear_wt_flag(proc, WT_TIMER);
        }
        clear_wt_flag(proc, WT_EVENT);
        wakeup_proc(proc);
    }

    local_intr_restore(old_intr);

    if (PREEMPTION) {
        schedule();
    }
}

void do_reset_event(event_id_t event_id, return_code_t *return_code) {
    event_t *event;
    if ((event = get_event_by_id(event_id)) == NULL) {
        *return_code = INVALID_PARAM;
        return;
    }

    event->status.event_state = DOWN;
    *return_code = NO_ERROR;
}

void do_wait_event(event_id_t event_id, system_time_t time_out, return_code_t *return_code) {
    event_t *event;
    if ((event = get_event_by_id(event_id)) == NULL) {
        *return_code = INVALID_PARAM;
        return;
    }

    if (time_out > MAX_TIME_OUT) {
        *return_code = INVALID_PARAM;
        return;
    }

    struct proc_struct *proc;
    bool old_intr;
    local_intr_save(old_intr);
    if (event->status.event_state == UP) {
        *return_code = NO_ERROR;
        local_intr_restore(old_intr);
    }
    else if (time_out == 0) {
        *return_code = NOT_AVAILABLE;
        local_intr_restore(old_intr);
    }
    else if (!PREEMPTION) {
        *return_code = INVALID_MODE;
        local_intr_restore(old_intr);
    }
    else if (time_out == INFINITE_TIME_VALUE) {
        proc = current;
        proc->status.process_state = WAITTING;
        set_wt_flag(proc, WT_EVENT);
        list_del_init(&proc->run_link);
        list_add_before(&event->waiting_thread, &proc->run_link);

        local_intr_restore(old_intr);
        schedule();        
        
        *return_code = NO_ERROR;
    } else {
        proc = current;
        proc->status.process_state = WAITTING;
        set_wt_flag(proc, WT_TIMER | WT_EVENT);
        list_del_init(&proc->run_link);
        list_add_after(&event->waiting_thread, &proc->run_link);

        timer_t *timer = kmalloc(sizeof(timer));
        timer_init(timer, proc, time_out);
        add_timer(timer);

        local_intr_restore(old_intr);

        schedule();

        kfree(timer);
        if (proc->timer == NULL) {
            *return_code = TIMED_OUT;
        }
        else {
            *return_code = NO_ERROR;
            proc->timer = NULL;
        }
    }
}

void do_get_event_id(event_name_t event_name, event_id_t *event_id, return_code_t *return_code) {
    event_t *event;
    if ((event = get_event_by_name(event_name)) == NULL) {
        *return_code = INVALID_CONFIG;
        return;
    }

    *event_id = event->event_id;
    *return_code = NO_ERROR;
}

void do_get_event_status(event_id_t event_id, event_status_t *event_status, return_code_t *return_code) {
    event_t *event;
    if ((event = get_event_by_id(event_id)) == NULL) {
        *return_code = INVALID_PARAM;
        return;
    }

    *event_status = event->status;
    *return_code = NO_ERROR;
}