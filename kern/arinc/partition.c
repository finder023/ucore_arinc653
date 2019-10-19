#include "partition.h"
#include <kmalloc.h>
#include <string.h>
#include <assert.h>
#include <clock.h>
#include <sync.h>

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

    part->mm = NULL;
    part->idle_proc = NULL;
    part->done = 0;
    part->deadline = 0;
    part->proc_num = 0;

    memset(&part->status, 0, sizeof(partition_status_t));

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

    list_entry_t *next = list_next(&part->part_tag); 
    if (next == &partition_set) {
        next = list_next(&partition_set);
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
                if (proc->wait_state & WT_SUSPEND_TIMER) {
                    proc->wait_state &= ~WT_SUSPEND_TIMER;
                    proc->wait_state &= ~WT_SUSPEND;
                }
            }
            local_intr_restore(intr_state);
       }
        le = le->next;
    }
}