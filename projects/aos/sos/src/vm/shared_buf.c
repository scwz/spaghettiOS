#include <sos.h>

#include "shared_buf.h"
#include "vmem_layout.h"
#include "../mapping.h"
#include "../proc/proc.h"
#include "address_space.h"

static cspace_t *cs;

static size_t
buf_offset(pid_t pid)
{
    return pid * SHARE_BUF_SIZE * PAGE_SIZE_4K;
}

static void * 
get_buf_from_pid(pid_t pid)
{
    return shared_buf_begin + buf_offset(pid); 
}

void 
shared_buf_init(cspace_t *cspace)
{
    cs = cspace;
    seL4_Word first_frame;
    frame_alloc_important(&first_frame);
    seL4_Word vaddr;
    for (size_t i = 1; i < SHARE_BUF_SIZE * MAX_PROCESSES; i++) {
        frame_alloc_important(&vaddr);
    }
    shared_buf_begin = (void *) first_frame;
    //printf("share buf: %lx\n", shared_buf_begin);
    assert(first_frame);
    assert(vaddr);
}

void
sos_map_buf(pid_t pid)
{
    struct proc *curproc = proc_get(pid);
    curproc->shared_buf = (void *) get_buf_from_pid(pid);
    struct region *reg = as_seek_region(curproc->as, (seL4_Word) PROCESS_SHARED_BUF_TOP);
    for (size_t i = 0; i < SHARE_BUF_SIZE; i++) {
        seL4_Word page = vaddr_to_page_num((seL4_Word) (get_buf_from_pid(pid) + i * PAGE_SIZE_4K));
        struct frame_table_entry *fte = get_frame(page);
        seL4_CPtr slot = cspace_alloc_slot(cs);
        cspace_copy(cs, slot, cs, fte->cap, seL4_AllRights);
        sos_map_frame(cs, curproc->as->pt, slot, curproc->vspace, 
                      reg->vbase + i * PAGE_SIZE_4K, seL4_AllRights, seL4_ARM_Default_VMAttributes, page, false);
        //printf("mapping vaddr: %lx, kernel vaddr: %lx\n", reg->vbase + i*PAGE_SIZE_4K, curproc->shared_buf + i*PAGE_SIZE_4K);
    }
}

void 
share_buf_check_len(size_t *len)
{
    if (*len > PAGE_SIZE_4K * SHARE_BUF_SIZE) {
        *len = PAGE_SIZE_4K * SHARE_BUF_SIZE;
    }
}

size_t 
sos_copyin(pid_t pid, seL4_Word kernel_vaddr, size_t len)
{
    share_buf_check_len(&len);
    memcpy(get_buf_from_pid(pid), (void *) kernel_vaddr, len);
    return len;
}

size_t 
sos_copyout(pid_t pid, seL4_Word kernel_vaddr, size_t len) 
{
    share_buf_check_len(&len);
    memcpy((void *) kernel_vaddr, get_buf_from_pid(pid), len);
    return len;
}
