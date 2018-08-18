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
#include <utils/page.h>

#define PAGE_INDEX_SIZE (PAGE_SIZE_4K / 8)

struct pt {
    seL4_CPtr cap;
    seL4_Word page[PAGE_INDEX_SIZE];
};

struct pd {
    seL4_CPtr cap;
    struct pt pt[PAGE_INDEX_SIZE];
};

struct pud {
    seL4_CPtr cap;
    struct pd pd[PAGE_INDEX_SIZE];
};

struct pgd {
    seL4_CPtr cap;
    struct pud pud[PAGE_INDEX_SIZE];
};


void page_table_init(cspace_t *cs);

int page_table_insert(void);

int page_table_insert_cap(seL4_Word vaddr, seL4_CPtr cap, uint8_t level);

int page_table_remove(void);

void vm_fault(seL4_Word faultaddress);
