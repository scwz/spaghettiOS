
#include <fcntl.h>

#include "pager.h"
#include "../vfs/vfs.h"
#include "../vfs/vnode.h"
#include "../uio.h"
#include "pagetable.h"
#include "../proc/proc.h"
#include "address_space.h"

struct pagefile_node {
    size_t index;
    struct pagefile_node *next;
};

//store free nodes
struct pagefile_list {
    size_t size;
    struct pagefile_node *head;
};

struct pagefile_list *pf_list; 
struct vnode *pf_vnode;

// open file
static int 
pagefile_open()
{
    return vfs_open("pagefile", FM_READ | FM_WRITE, &pf_vnode, KERNEL_PROC);
}

static size_t 
next_free_node()
{
    if (pf_list->head != NULL) { // try grab a node from list
        size_t ret = pf_list->head->index;
        struct pagefile_node * tmp = pf_list->head;
        pf_list->head = pf_list->head->next;
        free(tmp);
        return ret;
    }
    return pf_list->size++;
}

static int 
add_free_list(size_t entry)
{
    struct pagefile_node *new_node = malloc(sizeof(struct pagefile_node));
    new_node->index = entry;
    if (!new_node) {
        return -1;
    }
    if (pf_list->head == NULL) {
        pf_list->head = new_node;
        pf_list->head->next = NULL;
        return 0;
    }

    struct pagefile_node *curr = pf_list->head;
    if (entry <= curr->index) {
        new_node->next = curr;
        pf_list->head = new_node;
        return 0;
    }

    while (curr != NULL ) {
        if (entry >= curr->index || curr->next == NULL) {
            new_node->next = curr->next;
            curr->next = new_node;
            break;
        }
    }
    return 0;
}

// set the pagetable page 
int 
pageout(seL4_Word page)
{
    if (pf_vnode == NULL) {
        if (pagefile_open()) {
            return -1;
        }
    }
    // get page table entry
    struct frame_table_entry *fte = get_frame(page);
    assert(fte->pid >= 0 && fte->pid <= MAX_PROCESSES);
    assert(!fte->important);
    //printf("PID: %d, pt: %lx, vaddr: %lx, pn: %ld\n", fte->pid, procs[fte->pid]->as->pt, page_num_to_vaddr(page), page);
    struct page_table *pagetable = proc_get(fte->pid)->as->pt;
    seL4_Word *pte = page_lookup(pagetable, fte->user_vaddr);
    assert(pte);
    // set page table entry to be a pagefile index
    size_t ind = next_free_node();

    page_update_entry(pte, P_PAGEFILE, ind);
    assert(page_entry_number(*pte) == ind);

    // write out
    size_t offset = ind * PAGE_SIZE_4K;
    // write
    struct uio *u = malloc(sizeof(struct uio));
    uio_init(u, UIO_WRITE, PAGE_SIZE_4K, offset, 0);
    size_t bytes_written = sos_copyin(KERNEL_PROC, page_num_to_vaddr(page), PAGE_SIZE_4K);
    assert(bytes_written == PAGE_SIZE_4K);
    bytes_written = VOP_WRITE(pf_vnode, u);   
    assert(bytes_written == PAGE_SIZE_4K);
    free(u);
    return 0;
}

int 
pagein(seL4_Word entry, seL4_Word kernel_vaddr)
{
    //printf("PAGEIN entry %d, list->size %d\n", entry, pf_list->size);
    if (entry > pf_list->size) { //simple error check
        return -1;
    }
    
    //read and write to kernel_vaddr;
    struct uio *u = malloc(sizeof(struct uio));
    size_t offset = entry * PAGE_SIZE_4K;
    uio_init(u, UIO_READ, PAGE_SIZE_4K, offset, 0);
    size_t bytes_read = VOP_READ(pf_vnode, u);
    assert(bytes_read == PAGE_SIZE_4K);
    bytes_read = sos_copyout(KERNEL_PROC, kernel_vaddr, PAGE_SIZE_4K);
    assert(bytes_read == PAGE_SIZE_4K);
    free(u);
    add_free_list(entry);
    return 0;
}

// remove entry
int
pagefile_remove(seL4_Word entry)
{
    if (entry > pf_list->size) { //simple error check
        return -1;
    }
    add_free_list(entry);
    return 0;
}

void 
pager_bootstrap(void) 
{
    pf_list = malloc(sizeof(struct pagefile_list));
    pf_list->size = 0;
    pf_list->head = NULL;
    pf_vnode = NULL;
}
