#include "address_space.h"

struct addrspace *as_create(void) {
    struct addrspace * as = malloc(sizeof(struct addrspace));
    if(as == NULL){
        return NULL;
    }
    as->regions = NULL;
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
           new_region->vbase <= curr_region->vbase + curr_region->size){
            return false;
        }
        if(new_region->vbase + new_region->size >= curr_region->vbase && 
           new_region->vbase + new_region->size <= curr_region->vbase + curr_region->size){
            return false;
        }
    }
    return true;
}

int as_define_region(struct addrspace *as, seL4_Word vbase, size_t size, int accmode) {
    struct region* new_region = malloc(sizeof(struct region));
    new_region->vbase = vbase;
    new_region->size = size;
    new_region->accmode = accmode;
    
    if(new_region == NULL){
        return -1;
    }
    if (!valid_new_region(as, new_region)){
        return -1;
    }
    //add to head
    new_region->next = as->regions->next;
    as->regions = new_region;
    return 0;
}

int as_define_stack(struct addrspace *as) {
    //stub accmode and size
    as_define_region(as, 0, 0xFFFFFFFF, MODE_STACK);
    return 0;
}

int as_define_heap(struct addrspace *as) {
    as_define_region(as, 0xFFFFFFFF, 0xFFFFFFFF, MODE_HEAP);
    return 0;
}
