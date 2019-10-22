#include "partition.h"
#include <kmalloc.h>
#include <string.h>
#include <assert.h>
#include <clock.h>
#include <sync.h>
#include <sched.h>

static list_entry_t partition_set;

#define part_id(part_ptr)   (part_ptr->status.identifier)

static partition_id_t gen_part_id(void) {
    static partition_id_t ppid = 0;
    return ppid++;
}

static partition_t *alloc_partition(void) {
    partition_t *part = kmalloc(sizeof(partition_t));
    if (!part)
        return NULL;
    
    list_init(&part->part_tag);
    list_init(&part->proc_set);
    list_init(&part->run_list);
    list_init(&part->dormant_set);
    list_init(&part->timeout_set);

    // semaphore list
    list_init(&part->all_sem);
    list_init(&part->free_sem);
    part->sem_id = 0;
    part->sem_num = 0;

    // event init
    list_init(&part->all_event);
    list_init(&part->free_event);
    part->event_id = 0;
    part->event_num = 0;

    part->mm = NULL;
    part->idle_proc = NULL;
    part->scheduling = 0;
    part->deadline = 0;
    part->proc_num = 0;
    part->first_run = 0;

    memset(&part->status, 0, sizeof(partition_status_t));

    part->status.operating_mode = COLD_START;
    part->status.duration = 9;
    return part;
}

partition_t* partition_add(int *pid) {
    //struct proc_struct *proc = find_proc(pid);
    partition_t *part;
    if ((part = alloc_partition()) == NULL)
        return part;
    
    list_add_before(&partition_set, &part->part_tag);
    // part->proc_num++;

    //part->idle_proc = proc;
    //part->mm = proc->mm;
    //proc->part = part;

    part_id(part) = gen_part_id();
    if (pid != NULL)
        *pid = part_id(part);

    return part;
}

partition_t *next_partition(void) {
    partition_t *part = current->part;
    assert(!list_empty(&partition_set));

    partition_t *target;
    list_entry_t *next = list_next(&part->part_tag); 
    while (next != &part->part_tag) {
        if (next == &partition_set) {
            next = list_next(next);
        }
        target = le2part(next, part_tag);
        if (target->scheduling || !target->first_run)
            break;
        next = list_next(next);
    }

    return le2part(next, part_tag);
}

partition_t *get_partition(int ppid) {
    partition_t *part, *res = NULL;
    list_entry_t *ple = &partition_set;

    while ((ple = list_next(ple)) != &partition_set) {
        part = le2part(ple, part_tag);
        if (part->status.identifier == ppid) {
            res = part;
            break;
        }
    }
    return res;
}

void partition_init(void) {
    list_init(&partition_set);
}


void check_timeout(partition_t *part) {
    assert(part);

    list_entry_t *le = part->timeout_set.next;
    struct proc_struct *proc;
    bool intr_state;

    while (le != &part->timeout_set) {
        proc = le2proc(le, state_link);
        if (proc->timeout_deadline < ticks) {
            local_intr_save(intr_state);
            {
                list_del(le);
                list_add_after(&part->run_list, le);
                proc->wait_state &= ~WT_TIMER;
                if (proc->wait_state & (WT_SUSPEND | WT_TIMER)) {
                    proc->wait_state &= ~WT_TIMER;
                    proc->wait_state &= ~WT_SUSPEND;
                }
            }
            local_intr_restore(intr_state);
       }
        le = le->next;
    }
}


void do_get_partition_status(partition_status_t *status, return_code_t *return_code) {
    partition_t *part = current->part;
    *status = part->status;
    *return_code = NO_ERROR;
    return;
}

void do_set_partition_mode(operating_mode_t mode, return_code_t *return_code) {
    partition_t *part = current->part;
    if (mode >= 4) {
        *return_code = INVALID_PARAM;
        return;
    }

    if (part->status.operating_mode == NORMAL && mode == NORMAL) {
        *return_code = NO_ACTION;
        return;
    } 

    if (part->status.operating_mode == COLD_START && mode == WARM_START) {
        *return_code = INVALID_MODE;
        return;
    }

    bool old_intr;
    local_intr_save(old_intr);
    part->status.operating_mode = mode;

    if (mode == IDLE) {
        part->scheduling = 0;
        local_intr_restore(old_intr);
    }

    if (mode == WARM_START || mode == COLD_START) {

        local_intr_restore(old_intr);
    }

    if (mode == NORMAL) {
        list_entry_t *le = part->proc_set.next;
        struct proc_struct *proc;
        while (le != &part->proc_set)
        {
            proc = le2proc(le, part_link);
            if (proc->status.process_state == WAITTING && test_wt_flag(proc, WT_PNORMAL)) {
                clear_wt_flag(proc, WT_PNORMAL);
                list_del_init(&proc->run_link);
                if (proc->wait_state == 0) {
                    wakeup_proc(proc);
                }
            }
            le = list_next(le);
        }
        part->scheduling = 1;
        if (part->first_run == 0)
            part->first_run = 1;
        part->status.lock_level = 0;
        local_intr_restore(old_intr);
    }
    *return_code = NO_ERROR;
}
