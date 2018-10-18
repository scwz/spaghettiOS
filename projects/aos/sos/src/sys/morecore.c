/*
 * Copyright 2018, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(DATA61_GPL)
 */
#include <autoconf.h>
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <sys/mman.h>
#include <errno.h>
#include <assert.h>
#include <utils/util.h>
#include "../vm/vmem_layout.h"
#include "../vm/address_space.h"
#include "../proc/proc.h"
#include "morecore.h"
#include "../mapping.h"
#include "../vm/pagetable.h"
/*
 * Statically allocated morecore area.
 *
 * This is rather terrible, but is the simplest option without a
 * huge amount of infrastructure.
 */
#define MORECORE_AREA_BYTE_SIZE 0x100000
char morecore_area[MORECORE_AREA_BYTE_SIZE];
cspace_t * cspace;

/* Pointer to free space in the morecore area. */
static uintptr_t morecore_base = (uintptr_t) &morecore_area;
static uintptr_t morecore_top = (uintptr_t) &morecore_area[MORECORE_AREA_BYTE_SIZE];

/* Actual morecore implementation
   returns 0 if failure, returns newbrk if success.
*/

long sys_brk(va_list ap)
{
    uintptr_t ret;
    uintptr_t newbrk = va_arg(ap, uintptr_t);

    /*if the newbrk is 0, return the bottom of the heap*/
    if (!newbrk) {
        ret = morecore_base;
    } else if (newbrk < morecore_top && newbrk > (uintptr_t)&morecore_area[0]) {
        ret = morecore_base = newbrk;
    } else {
        ret = 0;
    }

    return ret;
}

/* Large mallocs will result in muslc calling mmap, so we do a minimal implementation
   here to support that. We make a bunch of assumptions in the process */

long sys_mmap(va_list ap)
{
    void *ogaddr = va_arg(ap, void*);
    size_t length = va_arg(ap, size_t);
    UNUSED int prot = va_arg(ap, int);
    int flags = va_arg(ap, int);
    UNUSED int fd = va_arg(ap, int);
    UNUSED off_t offset = va_arg(ap, off_t);
    
    struct proc * kernel_proc = proc_get(0);
#if 0
    if (flags & MAP_ANONYMOUS) {
        /* Check that we don't try and allocate more than exists */
        if (length > morecore_top - morecore_base) {
            return -ENOMEM;
        }
        /* Steal from the top */
        morecore_top -= length;
        return morecore_top;
    }
#endif
    if(ogaddr != 0){
        ZF_LOGF("not implemented");
        return -1;
    }
    length += PAGE_SIZE_4K;
    seL4_Word addr = MMAP_BOT;
    while(addr < MMAP_TOP){
        //printf("ADDR: %lx\n", addr);
        struct region * reg = as_seek_region(kernel_proc->as, addr);
        // if address is free try to define region
        if(reg == NULL){
            if(!as_define_region(kernel_proc->as, addr, length, READ | WRITE)){
                break;
            }
            //if it fails, its because theres something in the top half or in the middle, try add the length
            addr += PAGE_ALIGN_4K(length);
        }
        addr = PAGE_ALIGN_4K(reg->vtop) + PAGE_SIZE_4K;
    }
    if(addr > MMAP_TOP || addr + length > MMAP_TOP) return -1;

    struct region* reg = as_seek_region(kernel_proc->as, addr);
    printf("reg %lx, %lx\n", reg->vbase, reg->vtop);
    for(size_t i = 0; i < BYTES_TO_4K_PAGES(length); i++){
        seL4_Word vaddr;
        seL4_Word page = frame_alloc_important(&vaddr);
        //printf("alloc page: %ld\n", page);
        struct frame_table_entry * fte = get_frame(page);
        seL4_CPtr slot = cspace_alloc_slot(cspace);
        cspace_copy(cspace, slot, cspace, fte->cap, seL4_AllRights);
        sos_map_frame(cspace, kernel_proc->as->pt, slot, kernel_proc->vspace, 
        reg->vbase + i*PAGE_SIZE_4K, seL4_AllRights, seL4_ARM_Default_VMAttributes, page, true);
        fte->user_cap = slot;
    }
    return reg->vbase;
}

long sys_munmap(va_list ap){
    void* addr = va_arg(ap, void*);
    size_t length = va_arg(ap, size_t);
    printf("munmap: size %ld\n", length);
    struct proc * kernel_proc = proc_get(0);
    struct region* reg = as_seek_region(kernel_proc->as, addr);
    PAGE_ALIGN_4K(reg->vtop);
    uint8_t bits;
    seL4_Word entry;
    for(size_t i = 0; i < BYTES_TO_4K_PAGES(length); i++){
        seL4_Word * pte = page_lookup(kernel_proc->as->pt, PAGE_ALIGN_4K(reg->vbase + i *PAGE_SIZE_4K));
        
        if(!pte){ // lookup failed
            bits = P_INVALID;
        } else {
            assert(pte);
            entry = page_entry_number(*pte);
            bits = page_get_bits(*pte);
        }
        //printf("Munmapping: %lx; pte %ld, bits %lx, entry %lx\n", reg->vbase + i * PAGE_SIZE_4K, *pte);
        if(!bits){
            frame_free(entry);
        }
        else {
            return -1;
        }
        page_update_entry(pte, P_INVALID, 0);
    }
    as_destroy_region(kernel_proc->as, addr);
    return 0;
}

long sys_madvise(UNUSED va_list ap) {
    return 0;
}

void morecore_bootstrap(cspace_t * cs){
    cspace = cs;
}

void mmap_tests(void ){
    //MMAP TESTS
    void *addr = malloc(1146881);
    COMPILER_MEMORY_FENCE();
    free(addr);
    printf("MALLOC TEST1: addr %p\n", addr);
    COMPILER_MEMORY_FENCE();
    addr = malloc(1146881);
    COMPILER_MEMORY_FENCE();
    free(addr);
    printf("MALLOC TEST2: addr %p\n", addr);
    void* addr2 = malloc(1146881);
    addr = malloc(1146881);
    printf("MALLOC TEST3: addr %p\n", addr);
    free(addr2);
    free(addr);
    printf("MALLOC TEST4: addr %p\n", addr);
}