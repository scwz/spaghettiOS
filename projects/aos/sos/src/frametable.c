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
#include "mapping.h"
#include "vmem_layout.h"
#include "frametable.h"

static struct frame_table_entry *frame_table = NULL;
static seL4_Word first_paddr;
static seL4_Word max_paddr;
static seL4_Word curr_paddr;
static cspace_t *cspace;

void frame_table_init(cspace_t *cs) {
    cspace = cs;
    /*
    first_paddr = 0;
    curr_paddr = first_paddr;
    max_paddr = ut_size();
    */
    frame_table = (struct frame_table_entry *) first_paddr;
}

seL4_Word frame_alloc(seL4_Word *vaddr) {
    ut_t *ut = ut_alloc_4k_untyped(&curr_paddr);
    seL4_CPtr cap = cspace_alloc_slot(cspace);
    if (cap == seL4_CapNull) {
        ZF_LOGE("Cspace full");
        ut_free(ut, seL4_PageBits);
        return -1;
    }

    seL4_Error err = cspace_untyped_retype(cspace, 
                                            ut->cap, 
                                            cap, 
                                            seL4_ARM_SmallPageObject, 
                                            seL4_PageBits);
    ZF_LOGE_IFERR(err, "Failed retype untyped");
    if (err != seL4_NoError) {
        ut_free(ut, seL4_PageBits);
        cspace_free_slot(cspace, cap);
        return -1;
    }

    *vaddr = SOS_UT_TABLE + curr_paddr;
    err = map_frame(cspace, cap, seL4_CapInitThreadVSpace, *vaddr, 
                    seL4_AllRights, seL4_ARM_Default_VMAttributes);
    ZF_LOGE_IFERR(err, "Failed to map frame");
    if (err != seL4_NoError) {
        ut_free(ut, seL4_PageBits);
        cspace_free_slot(cspace, cap);
        return -1;
    }

    //frame_table[curr_paddr].cap = cap;

    return curr_paddr;
}

void frame_free(seL4_Word page) {

    (void) page;
}

