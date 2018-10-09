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

#define TTY_NAME             "sosh"
#define TTY_PRIORITY         (0)
#define TTY_EP_BADGE         (101)

#define MAX_PROCESSES 32

struct proc_wait_node {
    pid_t pid;
    struct proc_wait_node * next;
};

/* the one process we start */
struct proc {
    pid_t pid;
    ut_t *tcb_ut;
    seL4_CPtr tcb;
    ut_t *vspace_ut;
    seL4_CPtr vspace;

    struct addrspace *as;

    ut_t *ipc_buffer_ut;
    seL4_CPtr ipc_buffer;

    cspace_t cspace;

    ut_t *stack_ut;
    seL4_CPtr stack;

    struct proc_wait_node * wait_list;

    struct filetable *fdt;
};

bool proc_bootstrap(cspace_t *cspace, seL4_CPtr ep);
pid_t proc_start(char *app_name);
struct proc *proc_create(void);

int proc_destroy(pid_t pid);
int proc_wait_list_add(pid_t pid, pid_t pid_to_add);

struct proc *proc_get(pid_t pid);
