#include <list.h>
#include <sync.h>
#include <proc.h>
#include <sched.h>
#include <stdio.h>
#include <assert.h>
#include <arinc_sched.h>
#include <clock.h>

static list_entry_t timer_list;

static struct sched_class *sched_class;

static struct run_queue *rq;

static inline void
sched_class_enqueue(struct proc_struct *proc) {
    if (proc != idleproc) {
        sched_class->enqueue(rq, proc);
    }
}

static inline void
sched_class_dequeue(struct proc_struct *proc) {
    sched_class->dequeue(rq, proc);
}

static inline struct proc_struct *
sched_class_pick_next(void) {
    return sched_class->pick_next(rq);
}

static void
sched_class_proc_tick(struct proc_struct *proc) {
    if (proc != idleproc) {
        sched_class->proc_tick(rq, proc);
    }
    else {
        proc->need_resched = 1;
    }
}

static struct run_queue __rq;

void
sched_init(void) {
    list_init(&timer_list);

    //sched_class = &default_sched_class;
    sched_class = &arinc_sched_class;
    
    rq = &__rq;
    rq->max_time_slice = 5;
    sched_class->init(rq);

    cprintf("sched class: %s\n", sched_class->name);
}

void
wakeup_proc(struct proc_struct *proc) {
    assert(proc_state(proc) != RUNNING);
    bool intr_flag;
    local_intr_save(intr_flag);
    {
        if (test_wt_flag(proc, WT_SUSPEND)) {
            local_intr_restore(intr_flag);
            return;
        }
        list_del_init(&proc->run_link);
        // proc->wait_state = 0;
        proc_state(proc) = READY; 
        if (proc != current) {
            sched_class_enqueue(proc);
        }
    }
    local_intr_restore(intr_flag);
}

void
schedule(void) {
    bool intr_flag;
    struct proc_struct *next;
    local_intr_save(intr_flag);
    {
        current->need_resched = 0;
        // proc time slice end;
        if (proc_state(current) == RUNNING && current->time_slice <=0 ) {
            proc_state(current) = READY;
            sched_class_enqueue(current);
        }
        
        // larger prio in
        if (proc_state(current) == RUNNING) {
            if ((next = sched_class_pick_next()) != NULL) {
                if (proc_prio(next) <= proc_prio(current))
                    return;
                proc_state(current) = READY;
                sched_class_enqueue(current);
            }
        }

        // stop 
        if (proc_state(current) == DORMANT) {
            sched_class_dequeue(current);
        }

        // proc state is waitting, do not need to enqueue
        if ((next = sched_class_pick_next()) != NULL) {
            sched_class_dequeue(next);
        }
        if (next == NULL) {
            next = current->part->idle_proc;
        }
        assert(proc_state(next) == READY);
        next->runs ++;
        proc_state(next) = RUNNING;
        if (next != current) {
            proc_run(next);
        }
    }
    local_intr_restore(intr_flag);
}

void
add_timer(timer_t *timer) {
    bool intr_flag;
    local_intr_save(intr_flag);
    {
        assert(timer->expires > 0 && timer->proc != NULL);
        assert(list_empty(&(timer->timer_link)));
        list_entry_t *le = list_next(&timer_list);
        while (le != &timer_list) {
            timer_t *next = le2timer(le, timer_link);
            if (timer->expires < next->expires) {
                next->expires -= timer->expires;
                break;
            }
            timer->expires -= next->expires;
            le = list_next(le);
        }
        list_add_before(le, &(timer->timer_link));
    }
    local_intr_restore(intr_flag);
}

void
del_timer(timer_t *timer) {
    bool intr_flag;
    local_intr_save(intr_flag);
    {
        if (!list_empty(&(timer->timer_link))) {
            if (timer->expires != 0) {
                list_entry_t *le = list_next(&(timer->timer_link));
                if (le != &timer_list) {
                    timer_t *next = le2timer(le, timer_link);
                    next->expires += timer->expires;
                }
            }
            list_del_init(&(timer->timer_link));
        }
    }
    local_intr_restore(intr_flag);
}

void
run_timer_list(void) {
    bool intr_flag;
    local_intr_save(intr_flag);
    {
        list_entry_t *le = list_next(&timer_list);
        if (le != &timer_list) {
            timer_t *timer = le2timer(le, timer_link);
            assert(timer->expires != 0);
            timer->expires --;
            while (timer->expires == 0) {
                le = list_next(le);
                struct proc_struct *proc = timer->proc;
                if (proc->wait_state != 0) {
                    assert(proc->wait_state & WT_TIMER);
                }
                else {
                    warn("process %d's wait_state == 0.\n", proc->pid);
                }
                // clear wt flag
                wakeup_proc(proc);
                proc->timer = NULL;
                if (le == &timer_list) {
                    break;
                }
                timer = le2timer(le, timer_link);
            }
        }
        // sched_class_proc_tick(current);
    }
    local_intr_restore(intr_flag);
}


void check_deadline(void) {
    if (sched_class != &arinc_sched_class)
        return;
    current->time_slice--;
    partition_t *part = current->part;
    if (part->deadline < ticks || !part->scheduling)
        schedule();
    if (current->pid > 1 && current->time_slice <= 0)
        schedule();
}