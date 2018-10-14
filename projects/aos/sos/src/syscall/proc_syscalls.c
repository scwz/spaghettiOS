
#include "proc_syscalls.h"

#include "../proc/proc.h"
#include "../vm/shared_buf.h"

int syscall_proc_create(struct proc *curproc) {
    size_t nbytes = seL4_GetMR(1);
    char path[nbytes];
    sos_copyout(curproc->pid, path, nbytes);
    path[nbytes] = '\0';

    pid_t pid = proc_start(path);
    seL4_SetMR(0, pid);

    return 1;
}

int syscall_proc_delete(struct proc *curproc) {
    pid_t pid = seL4_GetMR(1);
    seL4_SetMR(0, proc_destroy(pid));
    return 1;
}

int syscall_proc_my_id(struct proc *curproc) {
    seL4_SetMR(0, curproc->pid);
    return 1;
}

int syscall_proc_status(struct proc *curproc) {
    seL4_SetMR(0, -1);
    return 1;
}

int syscall_proc_wait(struct proc *curproc) {
    pid_t pid = seL4_GetMR(1);

    seL4_SetMR(0, -1);
    return 1;
}

