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
#include "ut.h"


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

struct seL4_page_objects{
    ut_t* ut;
    seL4_CPtr cap;
};

struct seL4_page_objects_frame{
    uint64_t size;
    seL4_Word nextframe;
    struct seL4_page_objects page_objects[253];
};

struct page_table{
    struct pgd* pgd;
    struct seL4_page_objects_frame* seL4_pages;
};


struct page_table* page_table_init();

void save_seL4_info(struct page_table* page_table, ut_t * ut, seL4_CPtr slot);

int page_table_insert(struct page_table * page_table, seL4_Word vaddr, seL4_Word page_num);

int page_table_remove(struct page_table * page_table, seL4_Word vaddr);

void page_table_destroy(struct page_table * page_table);

void vm_fault(seL4_Word faultaddress);
