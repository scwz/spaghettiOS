#include "shared_buf.h"
#include "vmem_layout.h"
#include "mapping.h"
#include "proc.h"
#include "address_space.h"

cspace_t * cs;

void shared_buf_init(cspace_t * cspace){
    cs = cspace;
    seL4_Word first_frame;
    frame_alloc(&first_frame);
    seL4_Word vaddr;
    for(size_t i = 1; i < SHARED_BUF_PAGES; i++){
        frame_alloc(&vaddr);
    }
    assert(first_frame);
    assert(vaddr);
    shared_buf = (void *) first_frame;
    printf("shared buf: %lx\n", shared_buf);
}

void sos_map_buf(){
    struct region* reg = as_seek_region(curproc->as, (seL4_Word) PROCESS_SHARED_BUF_TOP);
    for(size_t i = 0; i < SHARED_BUF_PAGES; i++){
        seL4_Word page = vaddr_to_page_num(shared_buf + i*PAGE_SIZE_4K);
        struct frame_table_entry * fte = get_frame(page);
        seL4_CPtr slot = cspace_alloc_slot(cs);
        cspace_copy(cs, slot, cs, fte->cap, seL4_AllRights);
        sos_map_frame(cs, curproc->as->pt, slot, curproc->vspace, 
        reg->vbase + i*PAGE_SIZE_4K, seL4_AllRights, seL4_ARM_Default_VMAttributes, page, true);
        //printf("mapping vaddr: %lx, kernel vaddr: %lx\n", reg->vbase + i*PAGE_SIZE_4K, shared_buf + i*PAGE_SIZE_4K);
    }
}

static void check_len(size_t * len){
    if(*len > PAGE_SIZE_4K * SHARED_BUF_PAGES){
        *len = PAGE_SIZE_4K * SHARED_BUF_PAGES;
    }
}

size_t sos_copyin(seL4_Word kernel_vaddr, size_t len){
    check_len(&len);
    memcpy(shared_buf, kernel_vaddr, len);
    return len;
}
size_t sos_copyout(seL4_Word kernel_vaddr, size_t len){
    check_len(&len);
    memcpy(kernel_vaddr, shared_buf, len);
    return len;
}