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

#include "ut.h"
#include "vnode.h"

#define TTY_NAME             "tty_test"
#define TTY_PRIORITY         (0)
#define TTY_EP_BADGE         (101)

// for now we can use this as is since theres only 1 process
#define curproc (&procs[0])

#define MAX_PROCESSES 1

/* the one process we start */
struct proc {
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
    
    struct vnode ** fd;
};

struct proc procs[MAX_PROCESSES];

bool start_first_process(cspace_t *cspace, char *app_name, seL4_CPtr ep);
