#include "semaphore.h"
#include <sync.h>
#include <kmalloc.h>
#include <list.h>
#include <proc.h>
#include <string.h>
#include <sched.h>


static sem_t *get_sem_by_name(const char *name) {
    list_entry_t *le = current->part->all_sem.next;
    sem_t *sem;
    while (le != &(current->part->all_sem)) {
        sem = le2sem(le, sem_link);
        if (strcmp(name, sem->sem_name) == 0) {
            return sem;
        }
        le = le->next;
    }
    return NULL;
}

static int is_sem_free(semaphore_id_t id) {
    list_entry_t *le = current->part->free_sem.next;
    sem_t *sem;
    while ( le != &(current->part->free_sem)) 
    {
        sem = le2sem(le, sem_link);
        if (sem->sem_id == id) {
            return 1;
        }
        le = le->next;
    }
    return 0;
}

static sem_t *get_sem_by_id(semaphore_id_t id) {
    list_entry_t *le = current->part->all_sem.next;
    sem_t *sem;
    while ( le != &(current->part->all_sem)) 
    {
        sem = le2sem(le, sem_link);
        if (sem->sem_id == id) {
            if (is_sem_free(id)) 
                return NULL;
            else
                return sem;
        }
        le = le->next;
    }
    return NULL;
}

static semaphore_id_t sem_id_generator(void) {
    semaphore_id_t id = current->part->sem_id;
    current->part->sem_id += 1;
    return id;
}

static sem_t *alloc_sem(void) {
    sem_t *sem;

    if (!list_empty(&current->part->free_sem)) {
        list_entry_t *le = list_next(&current->part->free_sem);
        list_del_init(le);
        sem = le2sem(le, sem_link);
        return sem;
    }

    if ((sem = kmalloc(sizeof(sem_t))) == NULL) {
        return NULL;
    }

    list_init(&sem->sem_link);
    list_init(&sem->waiting_thread);
    sem->sem_id = sem_id_generator();

    return sem;
}


void do_create_semaphore (
    semaphore_name_t     semaphore_name,
    semaphore_value_t    current_value,
    semaphore_value_t    max_value,
    queuing_discipline_t queuing_discipline,
    semaphore_id_t       *semphore_id,
    return_code_t        *return_code) 
{
    if (current->part->sem_num >= MAX_NUMBER_OF_SEMAPHORES) {
        *return_code = INVALID_CONFIG;
        return;
    }

    sem_t *sem;
    if ((sem = get_sem_by_name(semaphore_name)) != NULL) {
        *return_code = NO_ACTION;
        return;
    }

    if (current_value > MAX_SEMAPHORE_VALUE) {
        *return_code = INVALID_PARAM;
        return;
    }

    if (max_value > MAX_SEMAPHORE_VALUE) {
        *return_code = INVALID_PARAM;
        return;
    }

    if (current_value > max_value) {
        *return_code = INVALID_PARAM;
        return;
    }

    if (queuing_discipline != FIFO && queuing_discipline != PRIORITY) {
        *return_code = INVALID_PARAM;
        return;
    }

    if ((sem = alloc_sem()) == NULL) {
        *return_code = INVALID_CONFIG;
        return;
    }
    
    strcpy(sem->sem_name, semaphore_name);
    *semphore_id = sem->sem_id;
    sem->sem_status.current_value = current_value;
    sem->sem_status.max_value = max_value;
    sem->sem_status.waiting_processes = 0;

    current->part->sem_num += 1;

    *return_code = NO_ERROR;
}

void do_wait_semaphore (
    semaphore_id_t  semaphore_id,
    system_time_t   time_out,
    return_code_t   *return_code)
{
    sem_t *sem;
    struct proc_struct *pthread;

    if ((sem = get_sem_by_id(semaphore_id)) == NULL) {
        *return_code = INVALID_PARAM;
        return;
    }

    if (time_out > MAX_TIME_OUT) {
        *return_code = INVALID_PARAM;
        return;
    }

    bool old_intr;
    local_intr_save(old_intr);
    if (sem->sem_status.current_value > 0) {
        sem->sem_status.current_value--;
        *return_code = NO_ERROR;
    } 
    else if (time_out == 0) {
        *return_code = NOT_AVAILABLE;
        local_intr_restore(old_intr);
    } 
    // TODO: error handler process
    else if (!PREEMPTION) {
        *return_code = INVALID_MODE;
        local_intr_restore(old_intr);
    }
    else if (time_out == INFINITE_TIME_VALUE) {
        pthread = current;
        pthread->status.process_state = WAITTING;
        set_wt_flag(pthread, WT_KSEM);
        list_del_init(&pthread->run_link);
        list_add_before(&sem->waiting_thread, &pthread->run_link);
        
        local_intr_restore(old_intr);
        schedule();
        *return_code = NO_ERROR;
    } 
    else {
        // timeout > 0 and sem val = 0
        pthread = current;
        pthread->status.process_state = WAITTING;

        timer_t *timer = kmalloc(sizeof(timer_t));
        timer_init(timer, pthread, time_out);
        add_timer(timer);
        pthread->timer = timer;
        set_wt_flag(pthread, WT_TIMER | WT_KSEM);
        list_del_init(&pthread->run_link);
        local_intr_restore(old_intr);
        schedule();

        kfree(timer);
        if (pthread->timer == NULL) {
            *return_code = TIMED_OUT;
        } else {
            pthread->timer = NULL;
            *return_code = NO_ERROR;
        }
    }

}

void do_signal_semaphore(
    semaphore_id_t  semaphore_id,
    return_code_t   *return_code) 
{
    bool old_status;
    sem_t *sem;
    if ((sem = get_sem_by_id(semaphore_id)) == NULL) {
        *return_code = INVALID_PARAM;
        return;
    }

    if (sem->sem_status.current_value == sem->sem_status.max_value) {
        *return_code = NO_ACTION;
        return;
    }

    local_intr_save(old_status);
    if (list_empty(&sem->waiting_thread)) {
        sem->sem_status.current_value++;
        *return_code = NO_ERROR;
        local_intr_restore(old_status);
    } else {
        list_entry_t *elem;
        struct proc_struct *pthread;
        
        elem = list_next(&sem->waiting_thread);
        pthread = le2proc(elem, state_link);

        if (pthread->timer != NULL) {
            del_timer(pthread->timer);
            clear_wt_flag(pthread, WT_TIMER);
        }

        clear_wt_flag(pthread, WT_KSEM);
        list_del_init(&pthread->run_link);

        local_intr_restore(old_status);
        if (!test_wt_flag(pthread, WT_SUSPEND)) {
            pthread->status.process_state = READY;
            wakeup_proc(pthread);
            if (PREEMPTION)
                schedule();
        }
        *return_code = NO_ERROR;
    }
}

void do_get_semaphore_id (
    semaphore_name_t    semaphore_name,
    semaphore_id_t      *semaphore_id,
    return_code_t       *return_code) 
{
    sem_t *sem;
    if ((sem = get_sem_by_name(semaphore_name)) == NULL) {
        *return_code = INVALID_CONFIG;
        return;
    }

    *semaphore_id = sem->sem_id;
    *return_code = NO_ERROR;

}

void do_get_semaphore_status (
    semaphore_id_t      semaphore_id,
    semaphore_status_t  *semaphore_status,
    return_code_t       *return_code)
{
    sem_t *sem;
    if ((sem = get_sem_by_id(semaphore_id)) == NULL) {
        *return_code = INVALID_PARAM;
        return;
    }

    *semaphore_status = sem->sem_status;
    *return_code = NO_ERROR;
}