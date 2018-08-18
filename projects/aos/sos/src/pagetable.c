#include "pagetable.h"
#include "vmem_layout.h"
#include "frametable.h"


static struct pgd* page_table;
static cspace_t* cspace;

static int set_page(seL4_Word vaddr){
    uint16_t offset = 0xFFF & vaddr;
    uint16_t l4 = (0x1FF) & (vaddr >> 12);
    uint16_t l3 = (0x1FF) & (vaddr >> 12 + 9);
    uint16_t l2 = (0x1FF) & (vaddr >> 12 + 9*2);
    uint16_t l1 = (0x1FF) & (vaddr >> 12 + 9*3);
    page_table->pud[l1].pd[l2].pt[l3].page[l4] = frame_alloc(&vaddr);
}

static int set_cap(seL4_Word vaddr, seL4_CPtr cap, uint8_t level){
    uint16_t offset = 0xFFF & vaddr;
    uint16_t l4 = (0x1FF) & (vaddr >> 12);
    uint16_t l3 = (0x1FF) & (vaddr >> 12 + 9);
    uint16_t l2 = (0x1FF) & (vaddr >> 12 + 9*2);
    uint16_t l1 = (0x1FF) & (vaddr >> 12 + 9*3);
    switch(level){
        case 2:
            page_table->pud[l1].cap = cap;
            break;
        case 3:
            page_table->pud[l1].pd[l2].cap = cap;
            break;
        case 4:
            page_table->pud[l1].pd[l2].cap = cap;
            break;
        default:
            ZF_LOGE("Level not found");
            return -1;
    }
    return 1;
}

void page_table_init(cspace_t *cs) {
    cspace = cs;
    page_table = (struct pgd*) SOS_PAGE_TABLE + sizeof(struct pgd);
    printf("page_table; %lx, %lx %lx %lx\n",page_table, page_table->pud, page_table->pud[0].pd, page_table->pud[0].pd[0].pt);
    //memset(page_table, 0, sizeof(*page_table));
}

int page_table_insert(void) {
    return 0;
}

int page_table_insert_cap(seL4_Word vaddr, seL4_CPtr cap, uint8_t level){
    return set_cap(vaddr, cap, level);
}

int page_table_remove(void) {
    return 0;
}

void vm_fault(seL4_Word faultaddress) {
    set_page(faultaddress);
}
