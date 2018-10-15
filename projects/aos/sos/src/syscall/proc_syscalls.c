
#include <string.h>
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
    if(pid == 0 || pid >= MAX_PROCESSES){
        seL4_SetMR(0, -1);
    }
    if(pid == curproc->pid){
        proc_destroy(pid);
        yield(NULL);
    }
    seL4_SetMR(0, proc_destroy(pid));
    
    return 1;
}

int syscall_proc_my_id(struct proc *curproc) {
    seL4_SetMR(0, curproc->pid);
    return 1;
}

int syscall_proc_status(struct proc *curproc) {
    unsigned int max = seL4_GetMR(1);
    sos_process_t *processes = malloc(sizeof(sos_process_t) * max);
    struct proc *curr;
    size_t nactive = 0;

    for (pid_t id = 1; id < max; id++) {
        if ((curr = proc_get(id)) != NULL) {
            processes[nactive].pid = id;
            processes[nactive].stime = curr->stime;
            processes[nactive].size = curr->size;
            strcpy(processes[nactive].command, curr->name);
            nactive++;
        }
    }
    sos_copyin(curproc->pid, processes, nactive * sizeof(sos_process_t));
    free(processes);
    seL4_SetMR(0, nactive);
    return 1;
}

int syscall_proc_wait(struct proc *curproc) {
    pid_t pid = seL4_GetMR(1);
    if(pid < -1 || pid == 0 || pid >= MAX_PROCESSES){
        seL4_SetMR(0, -1);
        return 1;
    }
    curproc->state = WAITING;
    
    if(pid >= 1){
        proc_wait_list_add(pid, curproc->pid);  
    } else {
        for(pid_t id = 1; id < MAX_PROCESSES; id++){
            struct proc * curr = proc_get(id);
            proc_wait_list_add(id, curproc->pid);
        }
    }
    curproc->wake_co = get_running();
    struct proc_wait_node * node = yield(NULL);
   
    curproc = proc_get(node->pid_to_wake);
    printf("rerunning again %d\n",curproc->pid);
    curproc->state = RUNNING;
    seL4_SetMR(0, node->owner);
    return 1;
}

