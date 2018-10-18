
#include <sel4/sel4.h>

#include "../proc/proc.h"
#include "../vm/address_space.h"
#include "vm_syscalls.h"
#include "../vm/vmem_layout.h"

int 
syscall_brk(struct proc *curproc) 
{
    seL4_Word newbrk = seL4_GetMR(1);

    seL4_Word hbase = curproc->as->heap->vbase;
    if (newbrk >= PROCESS_HEAP_BASE) {
        hbase = newbrk;
    }
    seL4_SetMR(0, 0); 
    seL4_SetMR(1, hbase);
    return 2;
}
