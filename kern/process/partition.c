#include "partition.h"
#include <kmalloc.h>
#include <string.h>
#include <assert.h>

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

    part->mm = NULL;
    part->idle_proc = NULL;

    memset(&part->status, 0, sizeof(partition_status_t));

    return part;
}

int partition_add(int pid) {
    struct proc_struct *proc = find_proc(pid);
    if (proc == NULL) 
        return -1;
    
    partition_t *part;
    if ((part = alloc_partition()) == NULL)
        return -2;
    
    list_add(&partition_set, &part->part_tag);
    part->idle_proc = proc;
    part->mm = proc->mm;
    proc->part = part;

    part_id(part) = gen_part_id();
    return 0;
}

void partition_init(void) {
    list_init(&partition_set);
}