#ifndef __LIBS_UNISTD_H__
#define __LIBS_UNISTD_H__

#define T_SYSCALL           0x80

/* syscall number */
#define SYS_exit            1
#define SYS_fork            2
#define SYS_wait            3
#define SYS_exec            4
#define SYS_clone           5
#define SYS_yield           10
#define SYS_sleep           11
#define SYS_kill            12
#define SYS_gettime         17
#define SYS_getpid          18
#define SYS_brk             19
#define SYS_mmap            20
#define SYS_munmap          21
#define SYS_shmem           22
#define SYS_putc            30
#define SYS_pgdir           31

// ARINC653
#define SYS_getpartid           32
#define SYS_createproc          33
#define SYS_set_priority        34
#define SYS_suspendself         35
#define SYS_suspend             36
#define SYS_resume              37
#define SYS_stopself            38
#define SYS_stop                39
#define SYS_start               40
#define SYS_delayedstart        41
#define SYS_lockpreemption      42
#define SYS_unlockpreemption    43
#define SYS_getmyid             44
#define SYS_getprocessid        45
#define SYS_getprocessstatus    46

/* OLNY FOR LAB6 */
#define SYS_lab6_set_priority 255

/* SYS_fork flags */
#define CLONE_VM            0x00000100  // set if VM shared between processes
#define CLONE_THREAD        0x00000200  // thread group

#endif /* !__LIBS_UNISTD_H__ */

