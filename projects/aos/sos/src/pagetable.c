#include "pagetable.h"
#include "vmem_layout.h"
#include "frametable.h"
#include "mapping.h"


static struct pgd* page_table;
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
    //printf("page_table; %lx, %lx %lx %lx\n",page_table, page_table->pud, page_table->pud[512].pd, page_table->pud[0].pd[0].pt);
    //memset(page_table, 0, sizeof(*page_table));
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

void vm_fault(seL4_Word faultaddress) {
    seL4_Word vaddr;
    seL4_Word page_num = frame_alloc(&vaddr);
    sos_map_frame(cspace, page_num, faultaddress);
}
