#include "arinc_sched.h"
#include <assert.h>
#include <list.h>
#include <proc.h>
#include <partition.h>
#include <clock.h>

#define proc_curprio(proc) ((proc)->status.current_priority)

extern struct proc_struct *idleproc, *initproc;

static struct proc_struct *part_pick_proc_by_prio(partition_t *part) {
    list_entry_t *ple = &part->run_list; 
    struct proc_struct *proc = NULL, *tmp;

    if (list_empty(&part->run_list)) {
        return part->idle_proc;
    }

    while ((ple = list_next(ple)) != &part->run_list) {
        tmp = le2proc(ple, run_link);

        if (proc_state(tmp) != READY)
            continue;
        
        if (!proc) {
            proc = tmp;
            continue;
        }

        if (proc_curprio(tmp) > proc_curprio(proc))
            proc = tmp;
    }

    return proc;
} 

struct proc_struct *pick_next(void) {
    partition_t *part = current->part;
    partition_t *init_part = get_partition(0);
    struct proc_struct *next = NULL;

    if (current == initproc && init_part->scheduling == 1)
        return current;

    if (part->deadline < ticks || !part->scheduling) {
        part = next_partition();
        part->deadline = ticks + part->status.duration;
    }

    next = part_pick_proc_by_prio(part);
    return next;
}


static void arinc_sched_init(struct run_queue *rq) {
    list_init(&rq->run_list);
}

static void arinc_sched_enqueue(struct run_queue *rq, struct proc_struct *proc) {

    partition_t *part = proc->part;
    if (part == NULL) {
        part = get_partition(proc->pid - 1);
        proc->part = part;
        if (part->idle_proc == NULL)
            part->idle_proc = proc; 
    }

    assert(part != NULL);
    
    assert(list_empty(&proc->run_link));
    assert(proc_state(proc) == RUNNING || proc_state(proc) == READY);
    
    list_add_before(&part->run_list, &proc->run_link);

    if (proc->time_slice <= 0 || proc->time_slice > proc_timecapa(proc)) {
        proc->time_slice = proc_timecapa(proc);
    }
}


static void arinc_sched_deque(struct run_queue *rq, struct proc_struct *proc) {

    partition_t *part = proc->part;
    assert(part != NULL);
    assert(proc_state(proc) == READY);

    if (proc == idleproc)
        return;

    assert(!list_empty(&proc->run_link));

    

    list_del_init(&proc->run_link);
}


static struct proc_struct *arinc_pick_next(struct run_queue *rq) {
    return pick_next();
}

static void arinc_proc_tick(struct run_queue *rq, struct proc_struct *proc) {
    if (proc->time_slice > 0)
        proc->time_slice--;
    if (proc->time_slice == 0)
        proc->need_resched = 1;
}

struct sched_class arinc_sched_class = {
    .name = "arinc_scheduler",
    .init = arinc_sched_init,
    .enqueue = arinc_sched_enqueue,
    .dequeue = arinc_sched_deque,
    .pick_next = arinc_pick_next,
    .proc_tick = arinc_proc_tick,
};