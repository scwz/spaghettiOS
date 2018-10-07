
#include "proc_syscalls.h"

#include "../proc/proc.h"
#include "../shared_buf.h"

int syscall_proc_create(void) {
    size_t nbytes = seL4_GetMR(1);
    char path[nbytes];
    sos_copyout(path, nbytes);
    path[nbytes] = '\0';

    pid_t pid = proc_start(path);
    seL4_SetMR(0, pid);

    return 1;
}

int syscall_proc_delete(void) {
    pid_t pid = seL4_GetMR(1);

    seL4_SetMR(0, -1);
    return 1;
}

int syscall_proc_my_id(void) {
    seL4_SetMR(0, curproc->pid);
    return 1;
}

int syscall_proc_status(void) {
    seL4_SetMR(0, -1);
    return 1;
}

int syscall_proc_wait(void) {
    pid_t pid = seL4_GetMR(1);

    seL4_SetMR(0, -1);
    return 1;
}

