#include "shared_buf.h"
#include "vm/vmem_layout.h"



void shared_buf_init(){
    seL4_Word first_frame;
    frame_alloc(&first_frame);
    seL4_Word vaddr;
    for(size_t i = 1; i < SHARED_BUF_PAGES; i++){
        frame_alloc(&vaddr);
    }
    assert(first_frame);
    assert(vaddr);
    shared_buf = (void *) first_frame;
}

static void check_len(size_t * len){
    if(*len > PAGE_SIZE_4K * SHARED_BUF_PAGES){
        *len = PAGE_SIZE_4K * SHARED_BUF_PAGES;
    }
}

size_t user_copyin(seL4_Word user_vaddr, size_t len){
    check_len(&len);
    memcpy(PROCESS_SHARED_BUF_TOP - PAGE_SIZE_4K * SHARED_BUF_PAGES, user_vaddr, len);
    return len;
}
size_t user_copyout(seL4_Word user_vaddr, size_t len){
    check_len(&len);
    memcpy(user_vaddr, PROCESS_SHARED_BUF_TOP - PAGE_SIZE_4K * SHARED_BUF_PAGES, len);
    return len;
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
