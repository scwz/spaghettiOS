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

#include "pagetable.h"

#define STACK_PAGES 100

typedef enum {
    READ  = 1,
    WRITE = 2,
    EXEC  = 4
} perm_t;

struct region {
    seL4_Word vbase;
    size_t size;
    int accmode;
    struct region *next;
};

struct addrspace {
    struct region *regions;
    struct page_table *pt;
};

struct addrspace *as_create(void);

void as_destroy(struct addrspace *as);

struct region *as_seek_region(struct addrspace *as, seL4_Word vaddr);

int as_define_region(struct addrspace *as, seL4_Word vbase, size_t size, perm_t accmode);

int as_define_stack(struct addrspace *as, seL4_Word *stack_ptr);

int as_define_heap(struct addrspace *as);
