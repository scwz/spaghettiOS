#include <cpio/cpio.h>

#include <aos/debug.h>

#include "../mapping.h"
#include "../elfload.h"
#include "../ut/ut.h"
#include "../vm/address_space.h"
#include "../syscall/filetable.h"
#include "proc.h"
#include "../vm/vmem_layout.h"

extern char _cpio_archive[];
static cspace_t *cs;
static seL4_CPtr pep;

static int stack_write(seL4_Word *mapped_stack, int index, uintptr_t val)
{
    mapped_stack[index] = val;
    return index - 1;
}

/* set up System V ABI compliant stack, so that the process can
 * start up and initialise the C library */
static uintptr_t init_process_stack(cspace_t *cspace, seL4_CPtr local_vspace, char *elf_file)
{
    /* Create a stack frame */
    curproc->stack_ut = alloc_retype(cspace, &curproc->stack, seL4_ARM_SmallPageObject, seL4_PageBits);
    if (curproc->stack_ut == NULL) {
        ZF_LOGE("Failed to allocate stack");
        return 0;
    }

    /* virtual addresses in the target process' address space */
    uintptr_t stack_top = PROCESS_STACK_TOP;
    uintptr_t stack_bottom = PROCESS_STACK_TOP - PAGE_SIZE_4K;
    /* virtual addresses in the SOS's address space */
    void *local_stack_top  = (seL4_Word *) SOS_SCRATCH;
    uintptr_t local_stack_bottom = SOS_SCRATCH - PAGE_SIZE_4K;

    /* find the vsyscall table */
    uintptr_t sysinfo = *((uintptr_t *) elf_getSectionNamed(elf_file, "__vsyscall", NULL));
    if (sysinfo == 0) {
        ZF_LOGE("could not find syscall table for c library");
        return 0;
    }

    /* Map in the stack frame for the user app */
    seL4_Error err = map_frame(cspace, curproc->stack, curproc->vspace, stack_bottom,
                               seL4_AllRights, seL4_ARM_Default_VMAttributes);
    if (err != 0) {
        ZF_LOGE("Unable to map stack for user app");
        return 0;
    }

    /* allocate a slot to duplicate the stack frame cap so we can map it into our address space */
    seL4_CPtr local_stack_cptr = cspace_alloc_slot(cspace);
    if (local_stack_cptr == seL4_CapNull) {
        ZF_LOGE("Failed to alloc slot for stack");
        return 0;
    }

    /* copy the stack frame cap into the slot */
    err = cspace_copy(cspace, local_stack_cptr, cspace, curproc->stack, seL4_AllRights);
    if (err != seL4_NoError) {
        cspace_free_slot(cspace, local_stack_cptr);
        ZF_LOGE("Failed to copy cap");
        return 0;
    }

    /* map it into the sos address space */
    err = map_frame(cspace, local_stack_cptr, local_vspace, local_stack_bottom, seL4_AllRights,
                    seL4_ARM_Default_VMAttributes);
    if (err != seL4_NoError) {
        cspace_delete(cspace, local_stack_cptr);
        cspace_free_slot(cspace, local_stack_cptr);
        return 0;
    }

    int index = -2;

    /* null terminate the aux vectors */
    index = stack_write(local_stack_top, index, 0);
    index = stack_write(local_stack_top, index, 0);

    /* write the aux vectors */
    index = stack_write(local_stack_top, index, PAGE_SIZE_4K);
    index = stack_write(local_stack_top, index, AT_PAGESZ);

    index = stack_write(local_stack_top, index, sysinfo);
    index = stack_write(local_stack_top, index, AT_SYSINFO);

    /* null terminate the environment pointers */
    index = stack_write(local_stack_top, index, 0);

    /* we don't have any env pointers - skip */

    /* null terminate the argument pointers */
    index = stack_write(local_stack_top, index, 0);

    /* no argpointers - skip */

    /* set argc to 0 */
    stack_write(local_stack_top, index, 0);

    /* adjust the initial stack top */
    stack_top += (index * sizeof(seL4_Word));

    /* the stack *must* remain aligned to a double word boundary,
     * as GCC assumes this, and horrible bugs occur if this is wrong */
    assert(index % 2 == 0);
    assert(stack_top % (sizeof(seL4_Word) * 2) == 0);

    /* unmap our copy of the stack */
    err = seL4_ARM_Page_Unmap(local_stack_cptr);
    assert(err == seL4_NoError);

    /* delete the copy of the stack frame cap */
    err = cspace_delete(cspace, local_stack_cptr);
    assert(err == seL4_NoError);

    /* mark the slot as free */
    cspace_free_slot(cspace, local_stack_cptr);

    return stack_top;
}

/* Start the first process, and return true if successful
 *
 * This function will leak memory if the process does not start successfully.
 * TODO: avoid leaking memory once you implement real processes, otherwise a user
 *       can force your OS to run out of memory by creating lots of failed processes.
 */
bool start_first_process(cspace_t *cspace, char* app_name, seL4_CPtr ep)
{
    if (cs == NULL) {
        cs = cspace;
    }
    if (pep == NULL) {
        pep = ep;
    }
    curproc->as = as_create();
    curproc->fdt = fdt_create();

    /* Create a VSpace */
    curproc->vspace_ut = alloc_retype(cspace, &curproc->vspace, seL4_ARM_PageGlobalDirectoryObject,
                                              seL4_PGDBits);
    if (curproc->vspace_ut == NULL) {
        return false;
    }

    /* assign the vspace to an asid pool */
    seL4_Word err = seL4_ARM_ASIDPool_Assign(seL4_CapInitThreadASIDPool, curproc->vspace);
    if (err != seL4_NoError) {
        ZF_LOGE("Failed to assign asid pool");
        return false;
    }

    /* Create a simple 1 level CSpace */
    err = cspace_create_one_level(cspace, &curproc->cspace);
    if (err != CSPACE_NOERROR) {
        ZF_LOGE("Failed to create cspace");
        return false;
    }

    /* Create an IPC buffer */
    curproc->ipc_buffer_ut = alloc_retype(cspace, &curproc->ipc_buffer, seL4_ARM_SmallPageObject,
                                                  seL4_PageBits);
    if (curproc->ipc_buffer_ut == NULL) {
        ZF_LOGE("Failed to alloc ipc buffer ut");
        return false;
    }

    /* allocate a new slot in the target cspace which we will mint a badged endpoint cap into --
     * the badge is used to identify the process, which will come in handy when you have multiple
     * processes. */
    seL4_CPtr user_ep = cspace_alloc_slot(&curproc->cspace);
    if (user_ep == seL4_CapNull) {
        ZF_LOGE("Failed to alloc user ep slot");
        return false;
    }

    /* now mutate the cap, thereby setting the badge */
    err = cspace_mint(&curproc->cspace, user_ep, cspace, ep, seL4_AllRights, TTY_EP_BADGE);
    if (err) {
        ZF_LOGE("Failed to mint user ep");
        return false;
    }

    /* Create a new TCB object */
    curproc->tcb_ut = alloc_retype(cspace, &curproc->tcb, seL4_TCBObject, seL4_TCBBits);
    if (curproc->tcb_ut == NULL) {
        ZF_LOGE("Failed to alloc tcb ut");
        return false;
    }

    /* Configure the TCB */
    err = seL4_TCB_Configure(curproc->tcb, user_ep,
                             curproc->cspace.root_cnode, seL4_NilData,
                             curproc->vspace, seL4_NilData, PROCESS_IPC_BUFFER,
                             curproc->ipc_buffer);
    if (err != seL4_NoError) {
        ZF_LOGE("Unable to configure new TCB");
        return false;
    }

    /* Set the priority */
    err = seL4_TCB_SetPriority(curproc->tcb, seL4_CapInitThreadTCB, TTY_PRIORITY);
    if (err != seL4_NoError) {
        ZF_LOGE("Unable to set priority of new TCB");
        return false;
    }

    /* Provide a name for the thread -- Helpful for debugging */
    NAME_THREAD(curproc->tcb, app_name);

    /* parse the cpio image */
    ZF_LOGI( "\nStarting \"%s\"...\n", app_name);
    unsigned long elf_size;
    char* elf_base = cpio_get_file(_cpio_archive, app_name, &elf_size);
    if (elf_base == NULL) {
        ZF_LOGE("Unable to locate cpio header for %s", app_name);
        return false;
    }

    /* set up the stack */
    seL4_Word sp = init_process_stack(cspace, seL4_CapInitThreadVSpace, elf_base);
    //seL4_Word sp;
    
    /* load the elf image from the cpio file */
    err = elf_load(cspace, seL4_CapInitThreadVSpace, curproc->vspace, elf_base);
    if (err) {
        ZF_LOGE("Failed to load elf image");
        return false;
    }
    
    // setup/create region for stack
    as_define_stack(curproc->as);
    as_define_heap(curproc->as);

    as_define_region(curproc->as, PROCESS_SHARED_BUF_TOP - PAGE_SIZE_4K * SHARED_BUF_PAGES, PAGE_SIZE_4K * SHARED_BUF_PAGES, READ | WRITE);
    //map buffer
    sos_map_buf();

    /* Map in the IPC buffer for the thread */
    err = map_frame(cspace, curproc->ipc_buffer, curproc->vspace, PROCESS_IPC_BUFFER,
                    seL4_AllRights, seL4_ARM_Default_VMAttributes);
    if (err != 0) {
        ZF_LOGE("Unable to map IPC buffer for user app");
        return false;
    }
    // setup region for ipc buffer
    as_define_region(curproc->as, PROCESS_IPC_BUFFER, PAGE_SIZE_4K, READ | WRITE);
    
    /* Start the new process */
    seL4_UserContext context = {
        .pc = elf_getEntryPoint(elf_base),
        .sp = sp,
    };
    printf("Starting %s at %p\n", app_name, (void *) context.pc);
    err = seL4_TCB_WriteRegisters(curproc->tcb, 1, 0, 2, &context);
    ZF_LOGE_IF(err, "Failed to write registers");
    return err == seL4_NoError;
}

int proc_create(char *appname) {
    return start_first_process(cs, appname, pep);
}
