#include "pagetable.h"
#include "vmem_layout.h"
#include "frametable.h"
#include "mapping.h"
#include "proc.h"

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


struct page_table * page_table_init() {
    struct page_table * page_table;
    assert(sizeof(struct pgd) == PAGE_SIZE_4K);
    seL4_Word page = frame_alloc(&page_table);
    page = frame_alloc(&page_table->pgd);
    return page_table;
}


int page_table_insert(struct page_table * page_table, seL4_Word vaddr, seL4_Word page_num) {
    struct pgd * pgd = &page_table->pgd;
    struct pt_index ind = get_pt_index(vaddr);
    seL4_Word page = 0;
    struct pud* pud = pgd->pud[ind.l1];
    if(pud  == NULL){
        page = frame_alloc(&pud);
        if(pud == NULL){
            return -1;
        }
        pgd->pud[ind.l1] = pud;
    }
    struct pd* pd = pgd->pud[ind.l1]->pd[ind.l2];
    if(pd  == NULL){
        page = frame_alloc(&pd);
        if(pd == NULL){
            return -1;
        }
        pgd->pud[ind.l1]->pd[ind.l2] = pd;
    }
    struct pt* pt = pgd->pud[ind.l1]->pd[ind.l2]->pt[ind.l3];
    if(pt  == NULL){
        page = frame_alloc(&pt);
        if(pt == NULL){
            return -1;
        }
        pgd->pud[ind.l1]->pd[ind.l2]->pt[ind.l3] = pt;
    }
    pt->page[ind.l4] = page_num;
    return 0;
}

int page_table_remove(struct page_table* page_table, seL4_Word vaddr) {
    //atm dont free page table
    struct pgd * pgd = &page_table->pgd;
    struct pt_index ind = get_pt_index(vaddr);
    struct pud* pud = pgd->pud[ind.l1];
    if(pud  == NULL){
        return -1;
    }
    struct pd* pd = pgd->pud[ind.l1]->pd[ind.l2];
    if(pd  == NULL){
        return -1;
    }
    struct pt* pt = pgd->pud[ind.l1]->pd[ind.l2]->pt[ind.l3];
    if(pt  == NULL){
        return -1;
    }
    pt->page[ind.l4] = NULL;
    return 0;
}

void save_seL4_info(struct page_table* page_table, ut_t * ut, seL4_CPtr slot){
    struct seL4_page_objects_frame* frame;
    if(page_table->seL4_pages == NULL){
        seL4_Word page = frame_alloc(&(page_table->seL4_pages));
        if(page_table->seL4_pages == NULL){
            ZF_LOGE("frame alloc fail");
        }  else {
            page_table->seL4_pages->size = 0;
            page_table->seL4_pages->nextframe = NULL;
            frame = page_table->seL4_pages;
        }
    } else {
        frame = page_table->seL4_pages;
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
