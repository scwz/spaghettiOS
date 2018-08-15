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
static seL4_Word bot_paddr;
static size_t frame_table_size;
static cspace_t *cspace;

static seL4_Word paddr_to_page_num(seL4_Word paddr){
    return (top_paddr - paddr)/PAGE_SIZE_4K; 
}

static seL4_Word page_num_to_paddr(seL4_Word page){
    return -(page * PAGE_SIZE_4K) + top_paddr;
}

static ut_t *alloc_retype_map(seL4_CPtr *cptr, uintptr_t *vaddr, uintptr_t *paddr) {
    ut_t *ut = ut_alloc_4k_untyped(paddr);
    if(ut == NULL){
        *vaddr = (seL4_Word) NULL;
        ZF_LOGE("Out of memory");
        return NULL;
    }

    *cptr = cspace_alloc_slot(cspace);
    if (*cptr == seL4_CapNull) {
        *vaddr = (seL4_Word) NULL;
        ZF_LOGE("Cspace full");
        ut_free(ut, seL4_PageBits);
        return NULL;
    }

    seL4_Error err = cspace_untyped_retype(cspace, 
                                            ut->cap, 
                                            *cptr, 
                                            seL4_ARM_SmallPageObject, 
                                            seL4_PageBits);
    ZF_LOGE_IFERR(err, "Failed retype untyped");
    if (err != seL4_NoError) {
        *vaddr = (seL4_Word) NULL;
        ut_free(ut, seL4_PageBits);
        cspace_delete(cspace, *cptr);
        cspace_free_slot(cspace, *cptr);
        return NULL;
    }

    *vaddr = SOS_FRAME_TABLE + *paddr;
    err = map_frame(cspace, *cptr, seL4_CapInitThreadVSpace, *vaddr, 
                    seL4_AllRights, seL4_ARM_Default_VMAttributes);
    ZF_LOGE_IFERR(err, "Failed to map frame");
    if (err != seL4_NoError) {
        *vaddr = (seL4_Word) NULL;
        ut_free(ut, seL4_PageBits);
        cspace_delete(cspace, *cptr);
        cspace_free_slot(cspace, *cptr);
        return NULL;
    }
    return ut;
}

void frame_table_init(cspace_t *cs) {
    cspace = cs;

    frame_table_size = ut_size() / PAGE_SIZE_4K;
    size_t frame_table_pages = BYTES_TO_4K_PAGES(frame_table_size * sizeof(struct frame_table_entry));
    seL4_Word frame_table_vaddr;
    
    for(size_t i = 0; i < frame_table_pages; i++){
        seL4_CPtr cap;
        alloc_retype_map(&cap, &frame_table_vaddr, &top_paddr);
        
        //printf("%ld, %ld, %lx\n", top_paddr, i, frame_table_vaddr);
    }
    //init values (physical memory allocates downwards)
    top_paddr -= PAGE_SIZE_4K; // first page will be here
    bot_paddr = top_paddr - ut_size();
    printf("===%ld, %ld, %lx\n\n\n", frame_table_pages, sizeof(struct frame_table_entry), frame_table_size );
    
    // test
    assert(frame_table_vaddr);
    printf("frame_table_vaddr: %lx, value %lx\n", frame_table_vaddr, *(seL4_Word *)frame_table_vaddr);
    frame_table = (struct frame_table_entry *) frame_table_vaddr;
    
    /*
    for (size_t i = 0; i < frame_table_size; i++) {
        printf("i %ld\n", i);
        if (i == 35328 || i == 35329) {
            continue;
        }
        assert(frame_table[i].cap == seL4_CapNull);
    }
    */
    //*(seL4_Word *) &(frame_table[0x9000]) = 0x37;
    //*(seL4_Word *) &(frame_table[0x8aac]) = 0x37;
    //*(seL4_Word *) &(frame_table[0x8aab]) = 0x37; // this address is fucked
    
    //*(seL4_Word *) &(frame_table[999]) = 0x37;
    //*(seL4_Word *) &(frame_table[frame_table_size-1]) = 0x37;
    //printf("%lx\n", &(frame_table[frame_table_size+4000] ));
    //*(seL4_Word *) &(frame_table[frame_table_size+999]) = 0x37;
}

seL4_Word frame_alloc(seL4_Word *vaddr) {
    uintptr_t paddr;
    seL4_CPtr cap;
    ut_t *ut = alloc_retype_map(&cap, vaddr, &paddr);
    
    seL4_Word page_num = paddr_to_page_num(paddr);
    //printf("first: %lx, curr:%lx, diff:%lx, maxdiff: %lx, pagenum: %lx, maxpage: %lx\n", top_paddr, curr_paddr, top_paddr-curr_paddr,top_paddr-bot_paddr, page_num, paddr_to_page_num(bot_paddr));
    frame_table[page_num].cap = cap;
    return page_num;
}

void frame_free(seL4_Word page) {
    //printf("freeing paddr: %lx, %lx, %lx\n", page_num_to_paddr(page), paddr_to_page_num(bot_paddr), page);
    if(page > paddr_to_page_num(bot_paddr)){
        ZF_LOGE("Page does not exist");
        return;
    }
    if(frame_table[page].cap == seL4_CapNull){
        ZF_LOGE("Page is already free");
        return;
    }
    seL4_ARM_Page_Unmap(frame_table[page].cap);
    cspace_delete(cspace, frame_table[page].cap);
    cspace_free_slot(cspace, frame_table[page].cap);
    ut_free(paddr_to_ut(page_num_to_paddr(page)), PAGE_BITS_4K);
    frame_table[page].cap = seL4_CapNull;
}

