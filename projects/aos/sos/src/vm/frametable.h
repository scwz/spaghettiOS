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

struct frame_table_entry {
    seL4_CPtr cap;
    seL4_CPtr user_cap;
    ut_t *ut;
    seL4_Word next_free_page;
    seL4_Word pid;
    seL4_Word user_vaddr;
    bool ref_bit;
    bool important;
};

seL4_Word vaddr_to_page_num(seL4_Word vaddr);

seL4_Word page_num_to_vaddr(seL4_Word page);

void frame_table_init(cspace_t *cs);

seL4_Word frame_alloc(seL4_Word *vaddr);

struct frame_table_entry * get_frame(seL4_Word page_num);

void frame_free(seL4_Word page);

