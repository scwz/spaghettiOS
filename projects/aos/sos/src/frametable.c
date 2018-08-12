#include <autoconf.h>
#include <utils/util.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <utils/page.h>

#include <cspace/cspace.h>
#include <aos/sel4_zf_logif.h>
#include <aos/debug.h>

#include "ut.h"
#include "mapping.h"
#include "vmem_layout.h"
#include "frametable.h"

static struct frame_table_entry *frame_table = NULL;
static seL4_Word top_paddr;
size_t frame_table_size;
static seL4_Word bot_paddr;
static seL4_Word curr_paddr;
static cspace_t *cspace;

static seL4_Word paddr_to_page_num(seL4_Word paddr){
    if(paddr > top_paddr){
        paddr = top_paddr;
    }
    if(paddr < bot_paddr){
        paddr = bot_paddr;
    }
    return (top_paddr - paddr)/PAGE_SIZE_4K; 
}

static seL4_Word page_num_to_paddr(seL4_Word page){
    if(page > paddr_to_page_num(bot_paddr)){
        return bot_paddr;
    }
    return -(page * PAGE_SIZE_4K) + top_paddr;
}

static seL4_Word frame_table_alloc(seL4_Word* vaddr){
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

    *vaddr = SOS_FRAME_TABLE + curr_paddr;
    err = map_frame(cspace, cap, seL4_CapInitThreadVSpace, *vaddr, 
                    seL4_AllRights, seL4_ARM_Default_VMAttributes);
    ZF_LOGE_IFERR(err, "Failed to map frame");
    if (err != seL4_NoError) {
        ut_free(ut, seL4_PageBits);
        cspace_free_slot(cspace, cap);
        return -1;
    }
    return curr_paddr;
}

void frame_table_init(cspace_t *cs) {
    cspace = cs;
    /*
    top_paddr = 0;
    curr_paddr = top_paddr;
    bot_paddr = ut_size();
    */

    //ut_size is huge, run out of cap space
    size_t frame_table_size = ut_size()/PAGE_SIZE_4K;
    size_t frame_table_pages = BYTES_TO_4K_PAGES(frame_table_size * sizeof(struct frame_table_entry));
    seL4_Word frame_table_vaddr;
    printf("===%d, %d\n\n\n", frame_table_pages, sizeof(struct frame_table_entry));
    for(size_t i = 0; i < frame_table_pages; i++){
        top_paddr = frame_table_alloc(&frame_table_vaddr);
        
        if(i == 0)
        printf("%d, %d, %lx\n", frame_table_pages, i, frame_table_vaddr);
    }
    //init values (physical memory allocates downwards)
    top_paddr -= PAGE_SIZE_4K; // first page will be here
    bot_paddr = top_paddr - ut_size();
    
    for(int i = top_paddr; i <= bot_paddr; i+= PAGE_SIZE_4K){
        frame_table[paddr_to_page_num(i)].cap == NULL;
    }

    // test
    assert(frame_table_vaddr);
    printf("frame_table_vaddr: %lx, value %lx\n", frame_table_vaddr, *(seL4_Word *)frame_table_vaddr);
    frame_table = (struct frame_table_entry*) frame_table_vaddr;
    *(seL4_Word *) &(frame_table[999]) = 0x37;
    *(seL4_Word *) &(frame_table[frame_table_size-1]) = 0x37;
    printf("%lx\n", &(frame_table[frame_table_size+4000] ));
    *(seL4_Word *) &(frame_table[frame_table_size+999]) = 0x37;
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

    *vaddr = SOS_FRAME_TABLE + curr_paddr;
    err = map_frame(cspace, cap, seL4_CapInitThreadVSpace, *vaddr, 
                    seL4_AllRights, seL4_ARM_Default_VMAttributes);
    ZF_LOGE_IFERR(err, "Failed to map frame");
    if (err != seL4_NoError) {
        ut_free(ut, seL4_PageBits);
        cspace_free_slot(cspace, cap);
        return -1;
    }

    
    seL4_Word page_num = (top_paddr - curr_paddr)/PAGE_SIZE_4K;
    printf("first: %lx, curr:%lx, diff:%lx, maxdiff: %lx, pagenum: %lx, maxpage: %lx\n", top_paddr, curr_paddr, top_paddr-curr_paddr,top_paddr-bot_paddr, page_num, paddr_to_page_num(bot_paddr));
    frame_table[page_num].cap = cap;
    frame_table[page_num].untyped = ut;
    return page_num;
}



void frame_free(seL4_Word page) {
    //printf("freeing paddr: %lx, %lx, %lx\n", page_num_to_paddr(page), paddr_to_page_num(bot_paddr), page);
    if(page > paddr_to_page_num(bot_paddr)){
        ZF_LOGE("Page does not exist");
        return;
    }
    if(frame_table[page].cap == NULL){
        ZF_LOGE("Page is already free");
        return;
    }
    cspace_delete(cspace, frame_table[page].cap);
    cspace_free_slot(cspace, frame_table[page].cap);
    ut_free(frame_table[page].untyped, PAGE_BITS_4K);
    frame_table[page].cap == NULL;
}

