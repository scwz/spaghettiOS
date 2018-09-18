#include "pager.h"
#include "../vfs/vfs.h"
#include "../vfs/vnode.h"
#include "../uio.h"

static struct pagefile_node {
    size_t index;
    struct pagefile_node * next;
};

//store free nodes
static struct pagefile_list {
    size_t size;
    struct pagefile_node * head;
};

struct pagefile_list * pf_list; 
struct vnode * pf_vnode;


static size_t next_free_node(){
    if(pf_list->head != NULL){ // try grab a node from list
        size_t ret = pf_list->head->index;
        struct pagefile_node * tmp = pf_list->head;
        pf_list->head = pf_list->head->next;
        free(tmp);
        return ret;
    }
    return pf_list->size++;
}

static int add_free_list(size_t entry){
    struct pagefile_node * new_node = malloc(sizeof(struct pagefile_node));
    new_node->index = entry;
    if(!new_node){
        return -1;
    }
    if(pf_list->head == NULL){
        pf_list->head = new_node;
        pf_list->head->next = NULL;
        return 0;
    }

    struct pagefile_node * curr = pf_list->head;
    if(entry <= curr->index){
        new_node->next = curr;
        pf_list->head = new_node;
        return 0;
    }

    while(curr != NULL ){
        if(entry >= curr->index || curr->next == NULL){
            new_node->next = curr->next;
            curr->next = new_node;
            break;
        }
    }
    return 0;
}

static int pagefile_open(){
    if(VOP_LOOKUP(root, "pagefile", &pf_vnode)){
        return -1;
    }
    if(VOP_EACHOPEN(pf_vnode, FM_READ | FM_WRITE)){
        return -1;
    }
    return 0;
}

static int pagefile_close(){
    VOP_RECLAIM(pf_vnode);
    
}

//TODO invalidate the correct frames
//set the pagetable page 
int pageout(seL4_Word page, seL4_Word entry){
    if(pf_vnode == NULL){
        if(pagefile_open()){
            return -1;
        }
    }
    struct frame_table_entry * fte = get_frame(page);
    size_t offset = next_free_node() * PAGE_SIZE_4K;
    struct uio u;
    uio_init(&u, UIO_WRITE, PAGE_SIZE_4K, offset);
    size_t bytes_written = sos_copyin(page_num_to_vaddr(page), PAGE_SIZE_4K);
    assert(bytes_written == 4096);
    bytes_written = VOP_WRITE(pf_vnode, &u);
    assert(bytes_written == 4096);
    return 0;
}

int pagein(seL4_Word entry, seL4_Word vaddr){
    if(pf_vnode == NULL){
        if(pagefile_open()){
            return -1;
        }
    }
    if(entry > pf_list->size){ //simple error check
        return -1;
    }
    seL4_Word kernel_vaddr;
    seL4_Word page = frame_alloc(&kernel_vaddr);
    struct uio u;
    size_t offset = entry * PAGE_SIZE_4K;
    uio_init(&u, UIO_READ, PAGE_SIZE_4K, offset);
    size_t bytes_read = VOP_READ(pf_vnode, &u);
    assert(bytes_read == 4096);
    bytes_read = sos_copyout(kernel_vaddr, PAGE_SIZE_4K);
    assert(bytes_read == 4096);
    
    add_free_list(entry);
    return 0;
}

void pager_bootsrap(){
    pf_list->size = 0;
    pf_list->head = NULL;
    pf_vnode = NULL;
}