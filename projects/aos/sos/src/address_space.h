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

#define MODE_STACK       1
#define MODE_HEAP        2

struct region {
    seL4_Word vbase;
    size_t size;
    int accmode;
    struct region *next;
};

struct addrspace {
    struct region *regions;
};

struct addrspace *as_create(void);

void as_destroy(struct addrspace *as);

int as_define_region(struct addrspace *as, seL4_Word vbase, size_t size, int accmode);

int as_define_stack(struct addrspace *as);

int as_define_heap(struct addrspace *as);
