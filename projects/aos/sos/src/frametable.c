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
#include "frametable.h"

static struct frame_table_entry *frame_table = NULL;
static cspace_t *cspace;

void frame_table_init(cspace_t *cs) {
    cspace = cs;
}

seL4_Word frame_alloc(seL4_Word *vaddr) {
    uintptr_t *paddr = NULL;

    ut_t *ut = ut_alloc_4k_untyped(paddr);
    seL4_CPtr cap = cspace_alloc_slot(cspace);

    seL4_Error err = cspace_untyped_retype(cspace, ut->cap, 
                                            cap, 
                                            seL4_ARM_SmallPageObject, 
                                            PAGE_SIZE_4K);
    ZF_LOGE_IFERR(err, "Failed retype untyped");
    if (err != seL4_NoError) {
        ut_free(ut, PAGE_SIZE_4K);
        cspace_free_slot(cspace, cap);
        return -1;
    }

    err = map_frame(cspace, cap, seL4_CapInitThreadVSpace, vaddr, 
                    seL4_AllRights, seL4_ARM_Default_VMAttributes);

    if (err != seL4_NoError) {
        ut_free(ut, PAGE_SIZE_4K);
        cspace_free_slot(cspace, cap);
        return -1;
    }

    return 0;
}

void frame_free(seL4_Word page) {

    (void) page;
}

