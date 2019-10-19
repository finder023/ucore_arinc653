#ifndef __USER_LIBS_SYSCALL_H__
#define __USER_LIBS_SYSCALL_H__

#include <arinc_proc.h>

int sys_exit(int error_code);
int sys_fork(void);
int sys_wait(int pid, int *store);
int sys_yield(void);
int sys_kill(int pid);
int sys_getpid(void);
int sys_putc(int c);
int sys_pgdir(void);
size_t sys_gettime(void);
/* FOR LAB6 ONLY */
void sys_lab6_set_priority(uint32_t priority);

int sys_sleep(unsigned int time);

int sys_get_partition_id(void);

int sys_create_process(process_attribute_t *, process_id_t *pid, return_code_t *return_code);

void sys_set_priority(process_id_t process_id,
                    uint8_t priority,
                    return_code_t *return_code) ;

void sys_suspend_self(uint32_t time_out,
                    return_code_t *return_code);
            
void sys_suspend(process_id_t process_id, return_code_t *return_code) ;

void sys_resume(process_id_t process_id, return_code_t *return_code) ;

void sys_stop_self(void) ;

void sys_stop(process_id_t process_id, return_code_t *return_code) ;

void sys_start(process_id_t process_id, return_code_t *return_code) ;

void sys_delayed_start(process_id_t process_id,
                    system_time_t delay_time,
                    return_code_t *return_code) ;

void sys_lock_preemption(lock_level_t *lock_level, return_code_t *return_code) ;

void sys_unlock_preemption(lock_level_t *lock_level, return_code_t *return_code) ;

void sys_get_my_id(process_id_t *process_id, return_code_t *return_code) ;

void sys_get_process_id(process_name_t  process_name,
                    process_id_t    *process_id,
                    return_code_t   *return_code);

void sys_get_process_status(process_id_t process_id,
                    process_status_t    *process_status,
                    return_code_t       *return_code);
#endif /* !__USER_LIBS_SYSCALL_H__ */

