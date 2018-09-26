
#include "proc_syscalls.h"

#include "../proc/proc.h"
#include "../shared_buf.h"

int syscall_proc_create(void) {
    size_t nbytes = seL4_GetMR(1);
    char path[nbytes];
    sos_copyout(path, nbytes);
    proc_create(path);
    return 0;
}

int syscall_proc_delete(void) {
    return 0;
}

int syscall_proc_my_id(void) {
    return 0;
}

int syscall_proc_status(void) {
    return 0;
}

int syscall_proc_wait(void) {
    return 0;
}

