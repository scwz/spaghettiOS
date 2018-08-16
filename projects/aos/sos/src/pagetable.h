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

struct page_table_entry{
    uint32_t pid;
    uint32_t vpn;
    uint32_t ctrl;
    struct page_table_entry* next;
};

void page_table_init(cspace_t *cs);