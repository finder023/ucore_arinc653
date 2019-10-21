#include <sampling_port.h>
#include <kmalloc.h>
#include <string.h>
#include <proc.h>
#include <partition.h>
#include <sync.h>
#include <clock.h>

// all sampling port set
static list_entry_t all_sampling_port;

// free sampling port set
static list_entry_t free_sampling_port;

// sampling port id
static uint32_t _sampling_port_id;

// sampling num
static uint32_t nr_sampling_port;

inline static sampling_port_id_t sampling_port_id_generator(void) {
    return ++_sampling_port_id;
}

static sampling_port_t* alloc_sampling_port(size_t max_size) {
    sampling_port_t *sample;
    list_entry_t *le;

    if (!list_empty(&free_sampling_port)) {
        le = free_sampling_port.next;
        list_del_init(le);
        sample = le2sample(le, list_link);
        kfree(sample->buff);
    }
    else {
        sample = kmalloc(sizeof(sampling_port_t));
        if (sample == NULL) {
            return NULL;
        }
        sample->id = sampling_port_id_generator();
    }

    memset(sample->name, 0, sizeof(sample->name));
    memset(&sample->status, 0, sizeof(sampling_port_status_t));
    sample->buff = kmalloc(max_size);
    list_add_after(&all_sampling_port, &sample->list_link);
    nr_sampling_port++;

    return sample;
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
    sampling_port_id_t id = *(sampling_port_id_t*)pid;
    sampling_port_t *sample = le2sample(le, list_link);
    
    if (sample->id == id) {
        return 1;
    }
    return 0;
}

static int comp_name(list_entry_t *le, void *pname) {
    char *name = pname;
    sampling_port_t *sample = le2sample(le, list_link);

    if (strcmp(sample->name, name) == 0)
        return 1;
    
    return 0;
}


static sampling_port_t *find_free_sampling_port_id(sampling_port_id_t id) {
    list_entry_t *le;
    sampling_port_t *sample;
    if ((le = find_list(&free_sampling_port, &id, comp_id)) == NULL) {
        return NULL;
    }

    sample = le2sample(le, list_link); 
    return sample; 
}

static sampling_port_t *find_free_sampling_port_name(sampling_port_name_t name) {
    list_entry_t *le;
    sampling_port_t *sample;

    if ((le = find_list(&free_sampling_port, name, comp_name)) == NULL) {
        return NULL;
    }

    sample = le2sample(le, list_link);
    return sample;
}

static sampling_port_t *get_sample_by_id(sampling_port_id_t id) {
    list_entry_t *le;
    sampling_port_t *sample;

    if ((le = find_list(&all_sampling_port, &id, comp_id)) == NULL) {
        return NULL;
    }

    sample = le2sample(le, list_link);

    if (find_free_sampling_port_id(id) == NULL) {
        return sample;
    }
    
    return NULL;
}

static sampling_port_t *get_sample_by_name(sampling_port_name_t name) {
    list_entry_t *le;
    sampling_port_t *sample;

    if ((le = find_list(&all_sampling_port, name, comp_name)) == NULL) {
        return NULL;
    }

    sample = le2sample(le, list_link);

    if (find_free_sampling_port_name(name) == NULL) {
        return sample;
    }

    return NULL;
}


void do_create_sampling_port(sampling_port_name_t name, message_size_t max_msg_size,
        port_direction_t port_direction, system_time_t refresh_period, 
        sampling_port_id_t *sampling_port_id, return_code_t *return_code)
{
    sampling_port_t *sample;
    if (nr_sampling_port >= MAX_NUMBER_OF_SAMPLING_PORTS) {
        *return_code = INVALID_CONFIG;
        return;
    }

    if (get_sample_by_name(name) != NULL) {
        *return_code = NO_ACTION;
        return;
    }

    if (max_msg_size <= 0) {
        *return_code = INVALID_CONFIG;
        return;
    }

    if (port_direction != SOURCE && port_direction != DESTINATION) {
        *return_code = INVALID_CONFIG;
        return;
    }

    if (refresh_period >= MAX_TIME_OUT) {
        *return_code = INVALID_CONFIG;
        return;
    }

    if (current->part->status.operating_mode == NORMAL) {
        *return_code = INVALID_MODE;
        return;
    }

    sample = alloc_sampling_port(max_msg_size);
    if (sample == NULL) {
        *return_code = INVALID_CONFIG;
        return;
    }

    sample->status.max_message_size = max_msg_size;
    sample->status.port_direction = port_direction;
    sample->status.refresh_period = refresh_period;
    strcpy(sample->name, name);
    
    *return_code = NO_ERROR;
}

void do_write_sampling_message(sampling_port_id_t sampling_port_id,
        message_addr_t msg_addr, message_size_t length,
        return_code_t *return_code)
{
    sampling_port_t *sample = get_sample_by_id(sampling_port_id);
    if (sample == NULL) {
        *return_code = INVALID_PARAM;
        return;
    }

    if (length > sample->status.max_message_size) {
        *return_code = INVALID_CONFIG;
        return;
    }

    if (length <= 0) {
        *return_code = INVALID_PARAM;
        return;
    }

    if (sample->status.port_direction != SOURCE) {
        *return_code = INVALID_MODE;
        return;
    }

    bool old_intr;
    local_intr_save(old_intr);

    memcpy(sample->buff, msg_addr, length);
    sample->length = length;
    sample->last_time_stamp = ticks;

    local_intr_restore(old_intr);

    *return_code = NO_ERROR;
}

void do_read_sampling_message(sampling_port_id_t sampling_port_id, 
        message_addr_t msg_addr, message_size_t *length, validity_t *validity,
        return_code_t *return_code) 
{
    sampling_port_t *sample = get_sample_by_id(sampling_port_id);
    if (sample == NULL) {
        *return_code = INVALID_PARAM;
        return;
    }

    if (sample->status.port_direction != DESTINATION) {
        *return_code = INVALID_MODE;
        return;
    }

    if (sample->length == 0) {
        *length = 0;
        *validity = INVALID;
        *return_code = NO_ACTION;
    }
    else {
        memcpy(msg_addr, sample->buff, sample->length);
        *length = sample->length;
        if (sample->status.last_msg_validity == VALID) {
            *validity = VALID;
        }
        else {
            *validity = INVALID;
        }
        *return_code = NO_ERROR;
    }

    if (ticks - sample->last_time_stamp > sample->status.refresh_period) {
        sample->status.last_msg_validity = INVALID;
    }
    else {
        sample->status.last_msg_validity = VALID;
    }

    *return_code = NO_ERROR;
}

void do_get_sampling_port_id(sampling_port_name_t name, 
        sampling_port_id_t *sampling_port_id, return_code_t *return_code) 
{
    sampling_port_t *sample = get_sample_by_name(name);
    if (sample == NULL) {
        *return_code = INVALID_CONFIG;
        return;
    }

    *sampling_port_id = sample->id;
    *return_code = NO_ERROR;
}

void do_get_sampling_port_status(sampling_port_id_t sampling_port_id,
        sampling_port_status_t *sampling_port_status, 
        return_code_t *return_code)
{
    sampling_port_t *sample = get_sample_by_id(sampling_port_id);
    if (sample == NULL) {
        *return_code = INVALID_PARAM;
        return;
    }

    *sampling_port_status = sample->status;
    *return_code = NO_ERROR;
}

void sampling_port_init(void) {
    list_init(&all_sampling_port);
    list_init(&free_sampling_port);
    _sampling_port_id = 0;
    nr_sampling_port = 0;
}