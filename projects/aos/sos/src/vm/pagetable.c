
#include <sos.h>

#include "address_space.h"
#include "pagetable.h"
#include "vmem_layout.h"
#include "frametable.h"
#include "../mapping.h"
#include "../proc/proc.h"
#include "pager.h"

static cspace_t *cspace;

static struct pt_index {
    uint16_t offset;
    uint16_t l4;
    uint16_t l3;
    uint16_t l2;
    uint16_t l1;
};

static struct pt_index get_pt_index(seL4_Word vaddr){
    struct pt_index ret;
    ret.offset = 0xFFF & vaddr;
    ret.l4 = (0x1FF) & (vaddr >> PAGE_BITS_4K);
    ret.l3 = (0x1FF) & (vaddr >> (PAGE_BITS_4K + 9));
    ret.l2 = (0x1FF) & (vaddr >> (PAGE_BITS_4K + 9*2));
    ret.l1 = (0x1FF) & (vaddr >> (PAGE_BITS_4K + 9*3));
    return ret;
}


struct page_table * page_table_init(void) {
    struct page_table * page_table;
    assert(sizeof(struct pgd) == PAGE_SIZE_4K);
    frame_alloc_important((seL4_Word *) &page_table);
    frame_alloc_important((seL4_Word *) &(page_table->pgd));
    return page_table;
}

seL4_Word page_entry_number(seL4_Word page){
    page_set_bits(&page, 0);
    return page;
}


void page_set_bits(seL4_Word * page_entry, uint8_t bits) {
    //clear bits
    seL4_Word mask = 0x00 << 16;
    mask |= 0xFFFF;
    mask = mask << 16;
    mask |= 0xFFFF;
    mask = mask << 16;
    mask |= 0xFFFF;
    mask = mask << 4;
    mask |= 0xF;

    //set the bits
    seL4_Word word = bits << 16;
    word = word << 16;
    word = word << 16;
    word = word << 4;
    *page_entry = word | (mask & *page_entry);
}
void page_update_entry(seL4_Word * page_entry, uint8_t bits, seL4_Word num){
    *page_entry = num;
    page_set_bits(page_entry, bits);
}

uint8_t page_get_bits(seL4_Word page_entry){
    page_entry = page_entry >> 16;
    page_entry = page_entry >> 16;
    page_entry = page_entry >> 16;
    page_entry = page_entry >> 4;
    return page_entry;
}

int page_table_insert(struct page_table * page_table, seL4_Word vaddr, seL4_Word page_num) {
    struct pgd * pgd = page_table->pgd;
    struct pt_index ind = get_pt_index(vaddr);
    struct pud* pud = pgd->pud[ind.l1];

    if(pud == NULL){
        frame_alloc_important((seL4_Word *) &pud);
        if(pud == NULL){
            return -1;
        }
        pgd->pud[ind.l1] = pud;
    }
    struct pd* pd = pgd->pud[ind.l1]->pd[ind.l2];
    if(pd == NULL){
        frame_alloc_important((seL4_Word *) &pd);
        if(pd == NULL){
            return -1;
        }
        pgd->pud[ind.l1]->pd[ind.l2] = pd;
    }
    struct pt* pt = pgd->pud[ind.l1]->pd[ind.l2]->pt[ind.l3];
    if(pt == NULL){
        frame_alloc_important((seL4_Word *) &pt);
        if(pt == NULL){
            return -1;
        }
        for(unsigned int i = 0; i < PAGE_INDEX_SIZE; i++){
            page_update_entry((seL4_Word *) &pt->page[i], P_INVALID, 0);
        }
        pgd->pud[ind.l1]->pd[ind.l2]->pt[ind.l3] = pt;
    }
    pt->page[ind.l4] = page_num;
    //printf("PAGETABLE, ADDR: %lx, l1: %lx l2: %lx l3:%lx l4:%lx, PAGENUM: %ld\n", vaddr, ind.l1, ind.l2, ind.l3, ind.l4, page_num);
    return 0;
}

seL4_Word * page_lookup(struct page_table* page_table, seL4_Word vaddr){
    struct pgd * pgd = page_table->pgd;
    struct pt_index ind = get_pt_index(vaddr);
    struct pud* pud = pgd->pud[ind.l1];
    if(pud  == NULL){
        return NULL;
    }
    struct pd* pd = pgd->pud[ind.l1]->pd[ind.l2];
    if(pd  == NULL){
        return NULL;
    }
    struct pt* pt = pgd->pud[ind.l1]->pd[ind.l2]->pt[ind.l3];
    if(pt  == NULL){
        return NULL;
    }
    //printf("LOOKUP, ADDR: %lx, l1: %lx l2: %lx l3:%lx l4:%lx\n", vaddr, ind.l1, ind.l2, ind.l3, ind.l4);
    return &pt->page[ind.l4];
}

int page_table_remove(struct page_table* page_table, seL4_Word vaddr) {
    //atm dont free page table
    seL4_Word * page = page_lookup(page_table, vaddr);
    if(page){
        return -1;
    }
    *page = 0;
    return 0;
}
void page_table_destroy(struct page_table * page_table, cspace_t * cspace){
    struct pgd * pgd = page_table->pgd;
    for(unsigned int i = 0; i < PAGE_INDEX_SIZE; i++){
        if(pgd->pud[i] != NULL){
            for(unsigned int j = 0; j < PAGE_INDEX_SIZE; j++){
                if(pgd->pud[i]->pd[j] != NULL){
                    for(unsigned int k = 0; k < PAGE_INDEX_SIZE; k++){
                        if(pgd->pud[i]->pd[j]->pt[k] != NULL){
                            for(unsigned int l = 0; l < PAGE_INDEX_SIZE; l++){
                                seL4_Word * pte = &(pgd->pud[i]->pd[j]->pt[k]->page[l]);
                                seL4_Word entry = page_entry_number(*pte);
                                seL4_Word bits = page_get_bits(*pte);
                                if(P_INVALID & bits){
                                    continue;
                                } else if (P_PAGEFILE & bits){
                                    pagefile_remove(entry);
                                } else {
                                    frame_free(entry);
                                }
                            }
                            frame_free(vaddr_to_page_num((seL4_Word) &(pgd->pud[i]->pd[j]->pt[k])));
                        }
                    }
                    frame_free(vaddr_to_page_num((seL4_Word) &(pgd->pud[i]->pd[j])));
                }
            }
            frame_free(vaddr_to_page_num((seL4_Word) &(pgd->pud[i])));
        }
    }
    struct seL4_page_objects_frame* frame;
    struct seL4_page_objects_frame* tmpframe;
    frame = page_table->seL4_pages;
    while (frame!= NULL ){
        for(int i = 0; i < 253; i++){
            struct seL4_page_objects object = frame->page_objects[i];
            cspace_delete(cspace, object.cap);
            cspace_free_slot(cspace, object.cap);
            ut_free(object.ut, PAGE_BITS_4K);
        }
        tmpframe = frame;
        frame = (struct seL4_page_objects_frame*) frame->nextframe;
        frame_free(vaddr_to_page_num((seL4_Word) &(tmpframe)));
    }
    frame_free(vaddr_to_page_num((seL4_Word) &(page_table->pgd)));
    frame_free(vaddr_to_page_num((seL4_Word) &(page_table)));
}

void save_seL4_info(struct page_table* page_table, ut_t * ut, seL4_CPtr slot){
    struct seL4_page_objects_frame* frame;
    if(page_table->seL4_pages == NULL){
        frame_alloc_important((seL4_Word*) &(page_table->seL4_pages));
        if(page_table->seL4_pages == NULL){
            ZF_LOGE("frame alloc fail");
        }  else {
            page_table->seL4_pages->size = 0;
            page_table->seL4_pages->nextframe =  NULL;
            frame = page_table->seL4_pages;
        }
    } else {
        frame = page_table->seL4_pages;
        while (frame->nextframe != NULL){
            frame = (struct seL4_page_objects_frame*) frame->nextframe;
        }
        if(frame->size == 253){
            frame_alloc_important((seL4_Word*) &(frame->nextframe));
            if(frame->nextframe == NULL){
                ZF_LOGE("frame alloc fail");
            }  else {
                frame = (struct seL4_page_objects_frame*) frame->nextframe;
                frame->size = 0;
                frame->nextframe = NULL;
            }
        }
    }
    seL4_Word ind = frame->size++;
    //printf("frame addr: %lx, frame size: %ld\n", frame, frame->size);
    frame->page_objects[ind].ut = ut;
    frame->page_objects[ind].cap = slot;
}

void vm_fault(pid_t pid) {
    struct proc *curproc = proc_get(pid);
    seL4_Word faultaddress = seL4_GetMR(1);
    
    struct addrspace *as = curproc->as;
    seL4_CPtr reply = cspace_alloc_slot(cspace);
    int err = cspace_save_reply_cap(cspace, reply);
    struct region *reg = as_seek_region(as, faultaddress);
    seL4_MessageInfo_t reply_msg = seL4_MessageInfo_new(0, 0, 0, 1); 

    if (reg != NULL) {
        //as->stack->vbase = PAGE_ALIGN_4K(faultaddress);
        seL4_Word * pte = page_lookup(as->pt, PAGE_ALIGN_4K(faultaddress));
        
        //if null
        uint8_t bits;
        seL4_Word entry;

        seL4_CPtr slot;
        struct frame_table_entry *frame_info;
        seL4_Word vaddr;
        seL4_Word page;

        if(!pte){ // lookup failed
            bits = P_INVALID;
        } else {
            assert(pte);
            entry = page_entry_number(*pte);
            bits = page_get_bits(*pte);
        }
        //printf("===vm fault at %llx: entry %llx bits: %llx!\n", faultaddress, pte, bits);
        //remap a deref'd page
        if(!bits){
            frame_info = get_frame(entry);
            assert(frame_info->ref_bit == 0);
            slot = frame_info->user_cap;
            seL4_Error err = seL4_ARM_Page_Map(slot, curproc->vspace, 
                                PAGE_ALIGN_4K(faultaddress), seL4_AllRights,
                                seL4_ARM_Default_VMAttributes);
            ZF_LOGE_IFERR(err, "failed to re-map frame");
            frame_info->ref_bit = 1;
        }
        //realloc a paged file
        else if(bits & P_PAGEFILE){
            page = frame_alloc(&vaddr);
            frame_info = get_frame(page);
            slot = cspace_alloc_slot(cspace);
            err = cspace_copy(cspace, slot, cspace, frame_info->cap, seL4_AllRights);
            //printf("err %d\n", err);
            err = sos_map_frame(cspace, as->pt,  slot,  curproc->vspace, 
                            PAGE_ALIGN_4K(faultaddress), seL4_AllRights, 
                            seL4_ARM_Default_VMAttributes, page, true); 
            ZF_LOGE_IFERR(err, "failed to map frame");
            //printf("err %d\n", err);
            err = pagein(entry, vaddr);
            //printf("err %d\n", err);
            frame_info->user_cap = slot;
            frame_info->pid = 0; //hardcoded atm;
            frame_info->user_vaddr = PAGE_ALIGN_4K(faultaddress);
            ZF_LOGE_IFERR(err, "failed to map frame");
            //seL4_Recv(1, 1);
        }
        else {
            page = frame_alloc(&vaddr);
            frame_info = get_frame(page);
            slot = cspace_alloc_slot(cspace);
            err = cspace_copy(cspace, slot, cspace, frame_info->cap, seL4_AllRights);
            //printf("cptr1: %lx, cptr2: %lx  \n", slot, frame_info->cap);
            ZF_LOGE_IFERR(err, "fail alloc");
            err = sos_map_frame(cspace, as->pt,  slot,  curproc->vspace, 
                            PAGE_ALIGN_4K(faultaddress), seL4_AllRights, 
                            seL4_ARM_Default_VMAttributes, page, true);
            
            ZF_LOGE_IFERR(err, "failed to map frame");
            frame_info->pid = 0; //hardcoded atm;
            frame_info->user_vaddr = PAGE_ALIGN_4K(faultaddress);
            frame_info->user_cap = slot;
            //seL4_Recv(1, 1);
        }

        seL4_SetMR(0, 0);
    }
    else {
        debug_print_fault(reply_msg, TTY_NAME);
        debug_dump_registers(curproc->tcb);

        ZF_LOGF("vm fault failed!");
    }
    
    seL4_Send(reply, reply_msg);
    cspace_free_slot(cspace, reply);
}

void vm_bootstrap(cspace_t *cs) {
    cspace = cs;
}
