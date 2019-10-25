#include <arinc_time.h>
#include <clock.h>
#include <sched.h>
#include <proc.h>
#include <sync.h>
#include <kmalloc.h>

void do_time_wait(system_time_t delay_time, return_code_t *return_code) {
    if (!PREEMPTION) {
        *return_code = INVALID_MODE;
        return;
    }

    if (delay_time > MAX_DELAY_TIME) {
        *return_code = INVALID_PARAM;
        return;
    }

    if (delay_time == INFINITE_TIME_VALUE) {
        *return_code = INVALID_PARAM;
        return;
    }

    if (delay_time == 0) {
        schedule();
    }

    else {
        bool old_intr;
        local_intr_save(old_intr);
        current->status.process_state = WAITING;
        set_wt_flag(current, WT_TIMER);
        list_del_init(&current->run_link);

        timer_t *timer = kmalloc(sizeof(timer));
        timer_init(timer, current, delay_time);
        add_timer(timer);
        current->timer = timer;
        local_intr_restore(old_intr);

        schedule();
        clear_wt_flag(current, WT_TIMER);
    }
    *return_code = NO_ERROR;
}

void do_periodic_wait(return_code_t *return_code) {
    if (!PREEMPTION) {
        *return_code = INVALID_MODE;
        return;
    }

    if (current->status.attributes.period == INFINITE_TIME_VALUE) {
        *return_code = INVALID_MODE;
        return;
    }

    bool old_intr;
    local_intr_save(old_intr);
    current->status.process_state = WAITING;
    set_wt_flag(current, WT_TIMER);
    list_del_init(&current->run_link);

    current->periodic_release += current->status.attributes.period;
    current->time_slice = current->status.attributes.time_capacity;
    local_intr_restore(old_intr);

    schedule();

    *return_code = NO_ERROR;
}

void do_get_time(system_time_t *system_time, return_code_t *return_code) {
    *system_time = ticks;
    *return_code = NO_ERROR;
}

void do_replenish(system_time_t budget_time, return_code_t *return_code) {
    partition_t *part = current->part;
    if (part->status.operating_mode != NORMAL) {
        *return_code = NO_ACTION;
        return;
    }

    if (current->status.attributes.period != INFINITE_TIME_VALUE &&
        ticks + budget_time > current->periodic_release + proc_timecapa(current)) {
        *return_code = INVALID_MODE;
        return;
    }

    if (budget_time > MAX_DELAY_TIME) {
        *return_code = INVALID_PARAM;
        return;
    }

    current->time_slice += budget_time;
    *return_code = NO_ERROR; 
}