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

struct frame_table_entry {
    seL4_CPtr cap;
    ut_t* untyped;
};

void frame_table_init(cspace_t *cs);

seL4_Word frame_alloc(seL4_Word *vaddr);

void frame_free(seL4_Word page);

