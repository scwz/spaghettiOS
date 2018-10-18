#pragma once

#include <autoconf.h>
#include <utils/util.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <cspace/cspace.h>
#include <aos/sel4_zf_logif.h>
#include <aos/debug.h>

#include "../ut/ut.h"
#include "../syscall/filetable.h"
#include "../vfs/vnode.h"
#include "../picoro/picoro.h"

#define SET_PID_BADGE(pid) (TTY_EP_BADGE | (pid << 20))
#define GET_PID_BADGE(badge) (badge >> 20)

#define TTY_NAME             "sosh"
#define TTY_PRIORITY         (0)
#define TTY_EP_BADGE         (101)

#define MAX_PROCESSES 32

#define KERNEL_PROC 0
#define INIT_PROC   1

struct proc_reap_node {
    pid_t pid;
    struct proc_reap_node * next;
};

struct proc_wait_node {
    pid_t owner;
    pid_t pid_to_wake;
    struct proc_wait_node * next;
};

struct proc_child_node {
    pid_t child;
    struct proc_child_node * next;
};

enum proc_state
{
    RUNNING,
    WAITING,
    ZOMBIE,
};
/* the one process we start */
struct proc {
    char name[N_NAME];
    pid_t pid;
    unsigned stime;
    unsigned size;
    ut_t *tcb_ut;
    seL4_CPtr tcb;
    ut_t *vspace_ut;
    seL4_CPtr vspace;

    int coro_count;
    struct addrspace *as;

    ut_t *ipc_buffer_ut;
    seL4_CPtr ipc_buffer;

    cspace_t cspace;

    ut_t *stack_ut;
    seL4_CPtr stack;

    struct proc_wait_node * wait_list;
    struct proc_child_node * child_list;

    struct filetable *fdt;
    coro wake_co; 
    enum proc_state state;
    
    bool protected_proc;

    void* shared_buf;
};

bool proc_bootstrap(cspace_t *cspace, seL4_CPtr ep);
pid_t proc_start(char *app_name);
struct proc *proc_create(char *app_name);

int zombiefy(pid_t pid );
void reap(void);

int proc_wait_list_add(pid_t pid, pid_t pid_to_add);

int add_child(pid_t parent, pid_t child);
int wait_all_child(pid_t parent);

struct proc *proc_get(pid_t pid);
