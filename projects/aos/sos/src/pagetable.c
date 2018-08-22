#include "pagetable.h"
#include "vmem_layout.h"
#include "frametable.h"
#include "mapping.h"
#include "proc.h"

static struct pgd* page_table;
static struct seL4_page_objects_frame ** seL4_pages;
static cspace_t* cspace;

static struct pt_index {
    uint8_t offset;
    uint8_t l4;
    uint8_t l3;
    uint8_t l2;
    uint8_t l1;
};

static struct pt_index get_pt_index(seL4_Word vaddr){
    struct pt_index ret;
    ret.offset = 0xFFF & vaddr;
    ret.l4 = (0x1FF) & (vaddr >> 12);
    ret.l3 = (0x1FF) & (vaddr >> 12 + 9);
    ret.l2 = (0x1FF) & (vaddr >> 12 + 9*2);
    ret.l1 = (0x1FF) & (vaddr >> 12 + 9*3);
    return ret;
}


void page_table_init(cspace_t *cs) {
    cspace = cs;
    assert(sizeof(struct pgd) == PAGE_SIZE_4K);
    seL4_Word page = frame_alloc(&page_table);
    printf("sizeof page_table: %lx\n", sizeof(struct pgd));
    seL4_pages = malloc(sizeof(struct seL4_page_objects_frame *)*MAX_PROCESSES);
    //printf("page_table; %lx, %lx %lx %lx\n",page_table, page_table->pud, page_table->pud[512].pd, page_table->pud[0].pd[0].pt);
}


int page_table_insert(seL4_Word vaddr, seL4_Word page_num) {
    struct pt_index ind = get_pt_index(vaddr);
    seL4_Word page = 0;
    struct pud* pud = page_table->pud[ind.l1];
    if(pud  == NULL){
        page = frame_alloc(&pud);
        if(pud == NULL){
            return -1;
        }
        page_table->pud[ind.l1] = pud;
        page_table_insert(pud, page);
    }
    struct pd* pd = page_table->pud[ind.l1]->pd[ind.l2];
    if(pd  == NULL){
        page = frame_alloc(&pd);
        if(pd == NULL){
            return -1;
        }
        page_table->pud[ind.l1]->pd[ind.l2] = pd;
        page_table_insert(pd, page);
    }
    struct pt* pt = page_table->pud[ind.l1]->pd[ind.l2]->pt[ind.l3];
    if(pt  == NULL){
        page = frame_alloc(&pt);
        if(pt == NULL){
            return -1;
        }
        page_table->pud[ind.l1]->pd[ind.l2]->pt[ind.l3] = pt;
        page_table_insert(pt, page);
    }
    pt->page[ind.l4] = page_num;
    return 0;
}

int page_table_remove(seL4_Word vaddr) {
    //atm dont free page table

    return 0;
}

void save_seL4_info(uint8_t pid, ut_t * ut, seL4_CPtr slot){
    struct seL4_page_objects_frame* frame;
    if(seL4_pages[pid] == NULL){
        seL4_Word page = frame_alloc(&(seL4_pages[pid]));
        if(seL4_pages[pid] == NULL){
            ZF_LOGE("frame alloc fail");
        }  else {
            seL4_pages[pid]->size = 0;
            seL4_pages[pid]->nextframe = NULL;
            page_table_insert(seL4_pages[pid], page);
            frame = seL4_pages[pid];
        }
    } else {
        frame = seL4_pages[pid];
        while (frame->nextframe != NULL){
            frame = frame->nextframe;
        }
        if(frame->size == 253){
            seL4_Word page = frame_alloc(&(frame->nextframe));
            if(frame->nextframe == NULL){
                ZF_LOGE("frame alloc fail");
            }  else {
                frame = frame->nextframe;
                frame->size = 0;
                frame->nextframe = NULL;
                page_table_insert(frame, page);
            }
        }
    }
    seL4_Word ind = frame->size++;
    frame->page_objects[ind].ut = ut;
    frame->page_objects[ind].cap = slot;
}

void vm_fault(seL4_Word faultaddress) {
    printf("vm fault at %lx!\n", faultaddress);
}
