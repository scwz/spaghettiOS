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

//store attributes in first n bytes 
struct pt {
    seL4_Word page[PAGE_INDEX_SIZE];
};

struct pd {
    struct pt* pt[PAGE_INDEX_SIZE];
};

struct pud {
    struct pd* pd[PAGE_INDEX_SIZE];
};

struct pgd {
    struct pud* pud[PAGE_INDEX_SIZE];
};

struct caps{
    seL4_CPtr caps[2 << 27];
};

void page_table_init(cspace_t *cs);

int page_table_insert(seL4_Word vaddr, seL4_Word page_num);

int page_table_remove(seL4_Word vaddr);

void vm_fault(seL4_Word faultaddress);
