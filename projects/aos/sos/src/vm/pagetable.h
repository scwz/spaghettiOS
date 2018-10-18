#pragma once

#include <autoconf.h>
#include <utils/util.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <sos.h>

#include <cspace/cspace.h>
#include <aos/sel4_zf_logif.h>
#include <aos/debug.h>
#include <utils/page.h>

#include "../ut/ut.h"
#include "../proc/proc.h"

#define P_INVALID 1
#define P_PAGEFILE 2

#define PAGE_INDEX_SIZE (PAGE_SIZE_4K / 8)
#define PAGE_OBJECT_SIZE 253

//store attributes in first 8 bytes 
struct pt {
    seL4_Word page[PAGE_INDEX_SIZE];
};

struct pd {
    struct pt *pt[PAGE_INDEX_SIZE];
};

struct pud {
    struct pd *pd[PAGE_INDEX_SIZE];
};

struct pgd {
    struct pud *pud[PAGE_INDEX_SIZE];
};

struct seL4_page_objects{
    ut_t *ut;
    seL4_CPtr cap;
};

struct seL4_page_objects_frame{
    uint64_t size;
    struct sel4_page_objects_frame *nextframe;
    struct seL4_page_objects page_objects[PAGE_OBJECT_SIZE];
};

struct page_table{
    struct pgd* pgd;
    struct seL4_page_objects_frame *seL4_pages;
};

struct page_table *page_table_init(void);

seL4_Word *page_lookup(struct page_table *, seL4_Word vaddr);

void page_set_bits(seL4_Word *page_entry, uint8_t bits);

uint8_t page_get_bits(seL4_Word page_entry);

void page_update_entry(seL4_Word *page_entry, uint8_t bits, seL4_Word num);

void save_seL4_info(struct page_table *page_table, ut_t *ut, seL4_CPtr slot);

int page_table_insert(struct page_table *page_table, seL4_Word vaddr, seL4_Word page_num);

seL4_Word * page_lookup(struct page_table* page_table, seL4_Word vaddr);

seL4_Word page_entry_number(seL4_Word page);

uint8_t page_get_bits(seL4_Word page_entry);

int page_table_invalidate(struct page_table * page_table, seL4_Word vaddr);

void page_table_destroy(struct page_table *page_table, cspace_t *cspace);

int get_proc_size(struct proc *proc);

seL4_Word page_entry_number(seL4_Word page);

void vm_fault(pid_t pid);

void vm_bootstrap(cspace_t *cs);
