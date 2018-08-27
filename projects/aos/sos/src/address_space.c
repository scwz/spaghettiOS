
#include "address_space.h"
#include "pagetable.h"
#include "vmem_layout.h"

struct addrspace *as_create(void) {
    struct addrspace * as = malloc(sizeof(struct addrspace));
    if(as == NULL){
        return NULL;
    }
    as->regions = NULL;
    as->pt = page_table_init();
    return as;
}

void as_destroy(struct addrspace *as) {
    struct region* curr_region = as->regions;
    struct region* next;
    while(curr_region != NULL){
        next = curr_region->next;
        free(curr_region);
        curr_region = next;
    }
    free(as);
}

static bool valid_new_region(struct addrspace *as, struct region* new_region){
    struct region* curr_region = as->regions;
    while(curr_region != NULL){
        if(new_region->vbase >= curr_region->vbase && 
           new_region->vbase <= curr_region->vtop){
            return false;
        }
        if(new_region->vtop >= curr_region->vbase && 
           new_region->vtop <= curr_region->vtop){
            return false;
        }
        curr_region = curr_region->next;
    }
    return true;
}

struct region *as_seek_region(struct addrspace *as, seL4_Word vaddr) {
    struct region *curr = as->regions;

    while (curr != NULL) {
        if (vaddr >= curr->vbase && vaddr <= curr->vtop) {
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}

int as_define_region(struct addrspace *as, seL4_Word vbase, size_t size, perm_t accmode) {
    struct region* new_region = malloc(sizeof(struct region));
    new_region->vbase = vbase;
    new_region->vtop = vbase + size;
    new_region->accmode = accmode;
    
    if(new_region == NULL){
        return -1;
    }
    if (!valid_new_region(as, new_region)){
        return -1;
    }
    //add to head
    new_region->next = as->regions;
    as->regions = new_region;
    return 0;
}

int as_define_stack(struct addrspace *as) {
    //stub accmode and size
    int result = as_define_region(as, 
                            PROCESS_STACK_TOP - PAGE_SIZE_4K * STACK_PAGES, 
                            PAGE_SIZE_4K * STACK_PAGES, 
                            READ | WRITE);
    as->stack = as->regions;
    return result;
}

int as_define_heap(struct addrspace *as) {
    as_define_region(as, PROCESS_STACK_TOP + PAGE_SIZE_4K, PROCESS_MAX - PROCESS_STACK_TOP - PAGE_SIZE_4K, READ | WRITE);
    return 0;
}
