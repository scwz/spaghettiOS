#include "address_space.h"

struct addrspace *as_create(void) {
    return NULL;
}

void as_destroy(struct addrspace *as) {

}

int as_define_region(struct addrspace *as) {
    return 0;
}

int as_define_stack(struct addrspace *as) {
    return 0;
}

int as_define_heap(struct addrspace *as) {
    return 0;
}
