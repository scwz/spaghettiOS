#include <cpio/cpio.h>
#include <clock/clock.h>
#include <time.h>
#include <string.h>

#include <aos/debug.h>

#include "../mapping.h"
#include "../elfload.h"
#include "../ut/ut.h"
#include "../vm/address_space.h"
#include "../syscall/filetable.h"
#include "proc.h"
#include "../vm/vmem_layout.h"
#include "../picoro/picoro.h"
#include "../vfs/vfs.h"

static char const * init_app_name = "tty_test";

struct proc *procs[MAX_PROCESSES];

extern char _cpio_archive[];
static cspace_t *cspace;
static seL4_CPtr ep; //endpoint
static pid_t curr_pid = 0;
static struct proc_reap_node * reap_list = NULL;

static int proc_destroy(pid_t pid);
static int 
stack_write(seL4_Word *mapped_stack, int index, uintptr_t val)
{
    mapped_stack[index] = val;
    return index - 1;
}

/* set up System V ABI compliant stack, so that the process can
 * start up and initialise the C library */
static uintptr_t 
init_process_stack(struct proc *proc, seL4_CPtr local_vspace)
{
    /* Create a stack frame */
    proc->stack_ut = alloc_retype(cspace, &proc->stack, seL4_ARM_SmallPageObject, seL4_PageBits);
    if (proc->stack_ut == NULL) {
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
    uintptr_t sysinfo = get_last_vsyscall_table();
    if (sysinfo == 0) {
        ZF_LOGE("could not find syscall table for c library");
        return 0;
    }

    /* Map in the stack frame for the user app */
    seL4_Error err = map_frame(cspace, proc->stack, proc->vspace, stack_bottom,
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
    err = cspace_copy(cspace, local_stack_cptr, cspace, proc->stack, seL4_AllRights);
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

static pid_t 
find_next_pid(void) 
{
    pid_t pid = curr_pid;
    do {
        if (procs[pid] == NULL) {
            curr_pid = (pid + 1 % MAX_PROCESSES);
            return pid;
        }
        pid = (pid + 1) % MAX_PROCESSES;
    } while (pid != curr_pid);

    return -1;
}

static struct proc * 
kernel_proc(void)
{
    struct proc *kernel = malloc(sizeof(struct proc));
    kernel->stime = get_time() / NS_IN_MS; 
    kernel->size = 0;
    pid_t pid = 0;

    strcpy(kernel->name, "spaghettiOS");

    kernel->tcb_ut = NULL;
    kernel->tcb = seL4_CapNull;
    kernel->vspace_ut = NULL;
    kernel->vspace = seL4_CapNull;
    kernel->stack_ut = NULL;
    kernel->stack = seL4_CapNull;

    kernel->as = as_create(); // for mmap
    kernel->fdt = NULL;
    kernel->pid = pid;
    kernel->wait_list = NULL;
    kernel->child_list = NULL;
    kernel->coro_count = 0;
    kernel->protected_proc = false;
    kernel->shared_buf = shared_buf_begin;
    procs[0] = kernel;
    return kernel;
}

bool 
proc_bootstrap(cspace_t *cs, seL4_CPtr pep) 
{
    cspace = cs;
    ep = pep;
    struct proc * kernel = kernel_proc();
    assert(kernel);
    proc_start((char *)init_app_name);
    return true;
}

pid_t
proc_start(char *app_name)
{
    struct proc *new = proc_create(app_name);
    if(new == NULL){
        return -1;
    }
    seL4_Word err;

    /* allocate a new slot in the target cspace which we will mint a badged endpoint cap into --
     * the badge is used to identify the process, which will come in handy when you have multiple
     * processes. */
    seL4_CPtr user_ep = cspace_alloc_slot(&new->cspace);
    if (user_ep == seL4_CapNull) {
        ZF_LOGE("Failed to alloc user ep slot");
        new->state = ZOMBIE;
        proc_destroy(new->pid);
        return -1;
    }

    /* now mutate the cap, thereby setting the badge */
    err = cspace_mint(&new->cspace, user_ep, cspace, ep, seL4_AllRights, SET_PID_BADGE(new->pid));
    if (err) {
        ZF_LOGE("Failed to mint user ep");
        new->state = ZOMBIE;
        proc_destroy(new->pid);
        return -1;
    }

    /* Create a new TCB object */
    new->tcb_ut = alloc_retype(cspace, &new->tcb, seL4_TCBObject, seL4_TCBBits);
    if (new->tcb_ut == NULL) {
        ZF_LOGE("Failed to alloc tcb ut");
        new->state = ZOMBIE;
        proc_destroy(new->pid);
        return -1;
    }

    /* Configure the TCB */
    err = seL4_TCB_Configure(new->tcb, user_ep,
                             new->cspace.root_cnode, seL4_NilData,
                             new->vspace, seL4_NilData, PROCESS_IPC_BUFFER,
                             new->ipc_buffer);
    if (err != seL4_NoError) {
        ZF_LOGE("Unable to configure new TCB");
        new->state = ZOMBIE;
        proc_destroy(new->pid);
        return -1;
    }

    /* Set the priority */
    err = seL4_TCB_SetPriority(new->tcb, seL4_CapInitThreadTCB, TTY_PRIORITY);
    if (err != seL4_NoError) {
        ZF_LOGE("Unable to set priority of new TCB");
        new->state = ZOMBIE;
        proc_destroy(new->pid);
        return -1;
    }

    /* Provide a name for the thread -- Helpful for debugging */
    NAME_THREAD(new->tcb, app_name);

    /* load the elf image*/
    if(new->pid == INIT_PROC){
        unsigned long elf_size;
        char* elf_base = cpio_get_file(_cpio_archive, app_name, &elf_size);
        if (elf_base == NULL) {
            ZF_LOGE("Unable to locate cpio header for %s", app_name);
            new->state = ZOMBIE;
            proc_destroy(new->pid);
            return -1;
        }
        err = elf_load(new->pid, cspace, seL4_CapInitThreadVSpace, new->vspace, elf_base);
        if (err) {
            ZF_LOGE("Failed to load elf image");
            new->state = ZOMBIE;
            proc_destroy(new->pid);
            return -1;
        }
    } else {
        err = elf_load_fs(new->pid, cspace, seL4_CapInitThreadVSpace, new->vspace, app_name);
        if (err) {
            ZF_LOGE("Failed to load elf image");
            new->state = ZOMBIE;
            proc_destroy(new->pid);
            return -1;
        }
    }
    
    /* set up the stack */
    seL4_Word sp = init_process_stack(new, seL4_CapInitThreadVSpace);
    
    // setup/create region for stack
    if(as_define_stack(new->as)) {
        ZF_LOGE("Failed to define stack");
        new->state = ZOMBIE;
        proc_destroy(new->pid);
        return -1;
    }
    if(as_define_heap(new->as)) {
        ZF_LOGE("Failed to define heap");
        new->state = ZOMBIE;
        proc_destroy(new->pid);
        return -1;
    }
    //define buffer region
    if(as_define_shared_buffer(new->as)) {
        ZF_LOGE("Failed to define shared_buffer");
        new->state = ZOMBIE;
        proc_destroy(new->pid);
        return -1;
    }
    //map buffer
    sos_map_buf(new->pid);

    /* Map in the IPC buffer for the thread */
    err = map_frame(cspace, new->ipc_buffer, new->vspace, PROCESS_IPC_BUFFER,
                    seL4_AllRights, seL4_ARM_Default_VMAttributes);
    if (err != 0) {
        ZF_LOGE("Unable to map IPC buffer for user app");
        new->state = ZOMBIE;
        proc_destroy(new->pid);
        return -1;
    }
    // setup region for ipc buffer
    if(as_define_region(new->as, PROCESS_IPC_BUFFER, PAGE_SIZE_4K, READ | WRITE)) {
        ZF_LOGE("Failed to define IPC buffer");
        new->state = ZOMBIE;
        proc_destroy(new->pid);
        return -1;
    }
    
    /* Start the new process */
    seL4_UserContext context = {
        .pc = get_last_entry_point(),
        .sp = sp,
    };
    new->state = RUNNING;
    printf("Starting %s at %p\n", app_name, (void *) context.pc);
    err = seL4_TCB_WriteRegisters(new->tcb, 1, 0, 2, &context);
    if(err){
        ZF_LOGE("Failed to write registers");
        new->state = ZOMBIE;
        proc_destroy(new->pid);
        return -1;
    }
    return new->pid;
}

struct proc *
proc_create(char *app_name) 
{
    struct proc *new = malloc(sizeof(struct proc));
    new->stime = get_time() / NS_IN_MS; 
    new->size = 0;
    pid_t pid = find_next_pid();

    strcpy(new->name, app_name);

    new->tcb_ut = NULL;
    new->tcb = seL4_CapNull;
    new->vspace_ut = NULL;
    new->vspace = seL4_CapNull;
    new->stack_ut = NULL;
    new->stack = seL4_CapNull;

    new->as = as_create();
    new->fdt = fdt_create();
    new->pid = pid;
    new->wait_list = NULL;
    new->child_list = NULL;
    new->coro_count = 0;
    new->protected_proc = false;
    new->shared_buf = NULL;

    if (pid == INIT_PROC || pid == KERNEL_PROC) {
        new->protected_proc = true;
    }

    procs[pid] = new;
    
    /* Create a VSpace */
    new->vspace_ut = alloc_retype(cspace, &new->vspace, seL4_ARM_PageGlobalDirectoryObject,
                                              seL4_PGDBits);
    if (new->vspace_ut == NULL) {
        return NULL;
    }

    /* assign the vspace to an asid pool */
    seL4_Word err = seL4_ARM_ASIDPool_Assign(seL4_CapInitThreadASIDPool, new->vspace);
    if (err != seL4_NoError) {
        ZF_LOGE("Failed to assign asid pool");
        return NULL;
    }

    /* Create a simple 1 level CSpace */
    err = cspace_create_one_level(cspace, &new->cspace);
    if (err != CSPACE_NOERROR) {
        ZF_LOGE("Failed to create cspace");
        return NULL;
    }

    /* Create an IPC buffer */
    new->ipc_buffer_ut = alloc_retype(cspace, &new->ipc_buffer, seL4_ARM_SmallPageObject,
                                                  seL4_PageBits);
    if (new->ipc_buffer_ut == NULL) {
        ZF_LOGE("Failed to alloc ipc buffer ut");
        return NULL;
    }

    // stdout
    struct vnode *res;
    vfs_lookup("console", &res, 0, pid);
    VOP_EACHOPEN(res, FM_WRITE, new->pid);
    new->fdt->openfiles[1] = malloc(sizeof(struct open_file));
    new->fdt->openfiles[1]->vn = res;
    new->fdt->openfiles[1]->refcnt = 1;
    new->fdt->openfiles[1]->offset = 0;
    new->fdt->openfiles[1]->flags = FM_WRITE;

    // stderr
    vfs_lookup("console", &res, 0, pid);
    VOP_EACHOPEN(res, FM_WRITE, new->pid);
    new->fdt->openfiles[2] = malloc(sizeof(struct open_file));
    new->fdt->openfiles[2]->vn = res;
    new->fdt->openfiles[2]->refcnt = 1;
    new->fdt->openfiles[2]->offset = 0;
    new->fdt->openfiles[2]->flags = FM_WRITE;

    return new;
}

static int
proc_wait_wakeup(pid_t pid)
{
    struct proc_wait_node* curr = procs[pid]->wait_list;
    struct proc_wait_node* tmp;

    while(curr != NULL){
        struct proc *wake_proc = proc_get(curr->pid_to_wake);
        if(wake_proc && wake_proc->state == WAITING){
            printf("owner: %d, wake: %d\n", curr->owner, curr->pid_to_wake);
            resume(wake_proc->wake_co, curr);
        }
        tmp = curr;
        curr = curr->next;
        free(tmp);
    }
    return 0;
}

static int 
destroy_child_list(pid_t pid)
{
    struct proc_child_node *curr = procs[pid]->child_list;
    struct proc_child_node *tmp;
    while (curr != NULL) {
        struct proc *curproc = proc_get(curr->child);
        //reparent orphans
        if (curproc != NULL) {
            proc_wait_list_add(curr->child, 1);
            add_child(1, curr->child);
            printf("reparenting... %d\n", curr->child);
        }
        tmp = curr;
        curr = curr->next;
        free(tmp);
    }
    return 0;
}

int
add_child(pid_t parent, pid_t child)
{
    struct proc_child_node *node = malloc(sizeof(struct proc_wait_node));
    if (node == NULL) {
        return -1;
    }
    node->child = child;
    node->next = procs[parent]->child_list;
    procs[parent]->child_list = node;
    return 0;
}

int
wait_all_child(pid_t parent)
{
    struct proc_child_node *curr = procs[parent]->child_list;
    while (curr != NULL) {
        if (proc_get(curr->child) != NULL) {
            proc_wait_list_add(parent, curr->child); // add all children
        }
        curr = curr->next;
    }
    return 0;
}

//add to end of the list
static int 
add_reap_list(pid_t pid)
{
    struct proc_reap_node *node = malloc(sizeof(struct proc_wait_node));
    if (node == NULL) {
        return -1;
    }
    node->pid = pid;
    node->next = NULL;
    struct proc_reap_node *curr = reap_list;
    if (curr == NULL) {
        reap_list = node;
        return 0;
    }
    while (curr->next != NULL) {
        curr = curr->next;
    }
    curr->next = node;
    return 0;
}

int
zombiefy(pid_t pid) {
    struct proc *p = proc_get(pid);
    if (p == NULL || p->state == ZOMBIE) {
        ZF_LOGE("Proc already destroyed");
        return -1;
    }
    if(p->protected_proc) {
        ZF_LOGE("proc cannot be deleted");
        return -1;
    }
    p->state = ZOMBIE;
    assert(add_reap_list(pid) == 0);
    destroy_child_list(pid); //reparent immediately
    proc_wait_wakeup(pid);
    seL4_TCB_Suspend(p->tcb);
    return 0;
}

static int
proc_destroy(pid_t pid)
{
    struct proc *p = proc_get(pid);
    assert(p->state == ZOMBIE);
    if (p->coro_count != 0) {
        return -1;
    }
    if (p->fdt) {
        fdt_destroy(p->fdt, pid);
    }
    if (p->coro_count != 0) {
        return -1;
    }
    if(p->shared_buf){
        sos_unmap_buf(pid);
    }
    if(p->as->pt){
        page_table_destroy(p->as->pt, cspace);
    }
    if (p->as) {
        as_destroy(p->as);
    }
    if (p->vspace_ut) {
        ut_free(p->vspace_ut, seL4_PGDBits);
    }
    if (p->ipc_buffer_ut) {
        ut_free(p->ipc_buffer_ut, seL4_PageBits);
    }
    
    cspace_delete(cspace, p->tcb);
    cspace_delete(cspace, p->vspace);
    cspace_delete(cspace, p->ipc_buffer);
    cspace_delete(cspace, p->stack);
    cspace_destroy(&p->cspace);
    procs[pid] = NULL;
    //cspace_delete(&p->cspace, user_ep);
    free(p);
    return 0;
}

//only try reap head
void 
reap(void)
{
    if (reap_list == NULL) return;
    struct proc_reap_node *head = reap_list;
    if (proc_destroy(head->pid) == 0) {
        reap_list = head->next;
        free(head);
    }
}

int
proc_wait_list_add(pid_t pid, pid_t pid_to_add)
{
    struct proc_wait_node *node = malloc(sizeof(struct proc_wait_node));
    if (node == NULL) {
        return -1;
    }
    node->owner = pid;
    node->pid_to_wake = pid_to_add;
    node->next = procs[pid]->wait_list;
    procs[pid]->wait_list = node;
    return 0;
}

struct proc *
proc_get(pid_t pid) 
{
    return procs[pid];
}
