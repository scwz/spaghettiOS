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

// for now we can use this as is since theres only 1 process
#define curproc (procs[0])

#define MAX_PROCESSES 32

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
    
    struct filetable *fdt;
};

struct proc *procs[MAX_PROCESSES];
bool proc_bootstrap(cspace_t *cspace, seL4_CPtr ep);
bool proc_start(char *app_name);
struct proc *proc_create(void);
