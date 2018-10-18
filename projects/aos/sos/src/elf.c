/*
 * Copyright 2018, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(DATA61_GPL)
 */
#include <utils/util.h>
#include <stdbool.h>
#include <sel4/sel4.h>
#include <fcntl.h>
#include <elf/elf.h>
#include <string.h>
#include <assert.h>
#include <cspace/cspace.h>
#include <sos.h>

#include "vm/vmem_layout.h"
#include "ut/ut.h"
#include "mapping.h"
#include "elfload.h"
#include "vm/frametable.h"
#include "proc/proc.h"
#include "vm/address_space.h"

#include "vfs/vfs.h"
#include "vfs/vnode.h"
#include "vm/shared_buf.h"

/*
 * Convert ELF permissions into seL4 permissions.
 */
static inline 
seL4_CapRights_t get_sel4_rights_from_elf(unsigned long permissions)
{
    bool canRead = permissions & PF_R || permissions & PF_X;
    bool canWrite = permissions & PF_W;

    if (!canRead && !canWrite) {
        return seL4_AllRights;
    }

    return seL4_CapRights_new(false, canRead, canWrite);
}

static int
perm_from_elf(unsigned long permissions)
{
    int p = 0;
    if (PF_X & permissions) p |= EXEC;
    if (PF_R & permissions) p |= READ;
    if (PF_W & permissions) p |= WRITE;
    return p;
}

static uintptr_t vsyscall_table;
static uint64_t entry_point;
/*
 * Load an elf segment into the given vspace.
 *
 * The content to load is either zeros or the content of the ELF
 * file itself, or both.
 * The split between file content and zeros is a follows.
 *
 * File content: [dst, dst + file_size)
 * Zeros:        [dst + file_size, dst + segment_size)
 *
 * Note: if file_size == segment_size, there is no zero-filled region.
 * Note: if file_size == 0, the whole segment is just zero filled.
 *
 * @param cspace        of the loader, to allocate slots with
 * @param loader        vspace of the loader
 * @param loadee        vspace to load the segment in to
 * @param src           pointer to the content to load
 * @param segment_size  size of segment to load
 * @param file_size     end of section that should be zero'd
 * @param dst           destination base virtual address to load
 * @param permissions   for the mappings in this segment
 * @return
 *
 */
static int 
load_segment_into_vspace(pid_t pid, cspace_t *cspace, seL4_CPtr loader, seL4_CPtr loadee,
                                    char *src, size_t segment_size,
                                    size_t file_size, uintptr_t dst, seL4_CapRights_t permissions, seL4_Word flags)
{
    assert(file_size <= segment_size);

    struct proc *curproc = proc_get(pid);

    /* We work a page at a time in the destination vspace. */
    unsigned int pos = 0;
    seL4_Error err = seL4_NoError;
    while (pos < segment_size) {

        seL4_Word loadee_vaddr = (ROUND_DOWN(dst, PAGE_SIZE_4K));
        seL4_Word loader_vaddr = ROUND_DOWN(SOS_ELF_VMEM + dst, PAGE_SIZE_4K);
        
        //loadee
        seL4_Word temp_vaddr;
        seL4_Word loadee_page = frame_alloc(&temp_vaddr);
        struct frame_table_entry* loadee_frame_info = get_frame(loadee_page);
        
        seL4_CPtr loadee_slot = cspace_alloc_slot(cspace);
        if (loadee_slot == seL4_CapNull) {
            ZF_LOGE("failed to alloc slot");
            return -1;
        }
        
        err = cspace_copy(cspace, loadee_slot, cspace, loadee_frame_info->cap, seL4_AllRights);
        if (err) {
            ZF_LOGE("failed to copy frame");
            cspace_free_slot(cspace, loadee_slot);
            return err;
        }

        err = sos_map_frame(cspace, curproc->as->pt,  loadee_slot,  loadee, 
                        loadee_vaddr, permissions, 
                        seL4_ARM_Default_VMAttributes, loadee_page, true);

        loadee_frame_info->user_cap = loadee_slot;
        loadee_frame_info->user_vaddr = loadee_vaddr;
        loadee_frame_info->pid = pid;

        if (err && err != seL4_DeleteFirst) {
            ZF_LOGE("failed to map frame");
            cspace_delete(cspace, loadee_slot);
            cspace_free_slot(cspace, loadee_slot);
            frame_free(loadee_page);
            return err;
        } 
        else if (err == seL4_DeleteFirst) {
            struct proc *p = proc_get(pid);
            struct region *r = as_seek_region(p->as, loadee_vaddr);
            if (r->accmode != perm_from_elf(flags)) {
                //do not map if different permissions
                ZF_LOGE("failed to map frame");
                cspace_delete(cspace, loadee_slot);
                cspace_free_slot(cspace, loadee_slot);
                frame_free(loadee_page);
            }
        }

        //loader
        seL4_CPtr loader_slot = cspace_alloc_slot(cspace);
        if (loader_slot == seL4_CapNull) {
            ZF_LOGE("failed to alloc slot");
            return -1;
        }

        err = cspace_copy(cspace, loader_slot, cspace, loadee_slot, seL4_AllRights);
        if (err) {
            ZF_LOGE("failed to copy frame");
            cspace_delete(cspace, loadee_slot);
            cspace_free_slot(cspace, loadee_slot);
            cspace_free_slot(cspace, loader_slot);
            return err;
        }
        err = sos_map_frame(cspace, curproc->as->pt, loader_slot, loader, 
                        loader_vaddr, seL4_AllRights, 
                        seL4_ARM_Default_VMAttributes, 0 , false);
        if (err) {
            ZF_LOGE("failed to map frame");
            cspace_delete(cspace, loadee_slot);
            cspace_delete(cspace, loader_slot);
            cspace_free_slot(cspace, loadee_slot);
            cspace_free_slot(cspace, loader_slot);
            return err;
        }
        /* finally copy the data */
        
        size_t nbytes = PAGE_SIZE_4K - (dst % PAGE_SIZE_4K);
        if (pos < file_size) {
            memcpy((void *) (loader_vaddr + (dst % PAGE_SIZE_4K)), src, MIN(nbytes, file_size - pos));
        }

        /* Note that we don't need to explicitly zero frames as seL4 gives us zero'd frames */

        /* Flush the caches */
        seL4_ARM_PageGlobalDirectory_Unify_Instruction(loader, loader_vaddr, loader_vaddr + PAGE_SIZE_4K);
        seL4_ARM_PageGlobalDirectory_Unify_Instruction(loadee, loadee_vaddr, loadee_vaddr + PAGE_SIZE_4K);

        pos += nbytes;
        dst += nbytes;
        src += nbytes;

        cspace_delete(cspace, loader_slot);
        cspace_free_slot(cspace, loader_slot);
    }
    return 0;
}

int
elf_load(pid_t pid, cspace_t *cspace, seL4_CPtr loader_vspace, seL4_CPtr loadee_vspace, char *elf_file)
{
    struct proc *curproc = proc_get(pid);
    /* Ensure that the file is an elf file. */
    if (elf_file == NULL || elf_checkFile(elf_file)) {
        ZF_LOGE("Invalid elf file");
        return -1;
    }
    int num_headers = elf_getNumProgramHeaders(elf_file);
    for (int i = 0; i < num_headers; i++) {

        /* Skip non-loadable segments (such as debugging data). */
        if (elf_getProgramHeaderType(elf_file, i) != PT_LOAD) {
            continue;
        }

        /* Fetch information about this segment. */
        char *source_addr = elf_file + elf_getProgramHeaderOffset(elf_file, i);
        size_t file_size = elf_getProgramHeaderFileSize(elf_file, i);
        size_t segment_size = elf_getProgramHeaderMemorySize(elf_file, i);
        uintptr_t vaddr = elf_getProgramHeaderVaddr(elf_file, i);
        seL4_Word flags = elf_getProgramHeaderFlags(elf_file, i);

        /* Copy it across into the vspace. */
        ZF_LOGD(" * Loading segment %p-->%p\n", (void *) vaddr, (void *)(vaddr + segment_size));
        //printf(" * Loading segment %p-->%p, f_size %d\n", (void *) vaddr, (void *)(vaddr + segment_size), file_size);
        int err = load_segment_into_vspace(pid, cspace, loader_vspace, loadee_vspace,
                                           source_addr, segment_size, file_size, vaddr,
                                           get_sel4_rights_from_elf(flags), flags);
        if (err) {
            ZF_LOGE("Elf loading failed!");
            return -1;
        }

        as_define_region(curproc->as, vaddr, segment_size, perm_from_elf(flags));
    }
    vsyscall_table = *((uintptr_t *) elf_getSectionNamed(elf_file, "__vsyscall", NULL));
    entry_point = elf_getEntryPoint(elf_file);
    return 0;
}

int 
elf_load_fs(pid_t pid, cspace_t *cspace, seL4_CPtr loader_vspace, seL4_CPtr loadee_vspace, char *path)
{
    struct vnode *vn;
    if (vfs_lookup(path, &vn, 0, KERNEL_PROC)) {
        ZF_LOGE("File not found!");
        return -1;
    }
    sos_stat_t buf;
    if (VOP_STAT(vn, &buf, KERNEL_PROC)) {
        ZF_LOGE("File does not exist!");
        return -1;
    }
    if (!(buf.st_fmode & FM_EXEC)) {
        ZF_LOGE("File is not executable!");
        return -1;
    }
    VOP_EACHOPEN(vn, O_RDONLY, KERNEL_PROC);
    struct proc *curproc = proc_get(pid);
    assert(vn);
    
    struct uio *u = malloc(sizeof(struct uio));
    uio_init(u, UIO_READ, PAGE_SIZE_4K, 0, KERNEL_PROC);
    
    char *elf_file = malloc(buf.st_size);
    //printf("size %ld ptr: %p\n", buf.st_size, elf_file);
    assert(elf_file != NULL);
    size_t read_offset = 0;
    while (read_offset < (buf.st_size)) {
        //printf("i: %d, size - offset %ld, read_offset %ld\n", i++, buf.st_size - read_offset, read_offset);
        u->offset = read_offset;
        u->len = MIN(PAGE_SIZE_4K, buf.st_size - read_offset);
        size_t bytes_read = VOP_READ(vn, u);
        sos_copyout(KERNEL_PROC, (seL4_Word)elf_file + read_offset, bytes_read);
        //printf("end cpyout\n");
        read_offset += bytes_read;
    }

    free(u);
    int num_headers = elf_getNumProgramHeaders(elf_file);
    for (int i = 0; i < num_headers; i++) {
        /* Skip non-loadable segments (such as debugging data). */
        if (elf_getProgramHeaderType(elf_file, i) != PT_LOAD) {
            continue;
        }
        /* Fetch information about this segment. */
        char *source_addr = elf_file + elf_getProgramHeaderOffset(elf_file, i);
        size_t file_size = elf_getProgramHeaderFileSize(elf_file, i);
        size_t segment_size = elf_getProgramHeaderMemorySize(elf_file, i);
        uintptr_t vaddr = elf_getProgramHeaderVaddr(elf_file, i);
        seL4_Word flags = elf_getProgramHeaderFlags(elf_file, i);

        /* Copy it across into the vspace. */
        ZF_LOGD(" * Loading segment %p-->%p\n", (void *) vaddr, (void *)(vaddr + segment_size));
        //printf(" * Loading segment %p-->%p, f_size %d\n", (void *) vaddr, (void *)(vaddr + segment_size), file_size);
        int err = load_segment_into_vspace(pid, cspace, loader_vspace, loadee_vspace,
                                           source_addr, segment_size, file_size, vaddr,
                                           get_sel4_rights_from_elf(flags), flags);
        if (err) {
            ZF_LOGE("Elf loading failed!");
            return -1;
        }

        as_define_region(curproc->as, vaddr, segment_size, perm_from_elf(flags));
    }
    vsyscall_table = *((uintptr_t *) elf_getSectionNamed(elf_file, "__vsyscall", NULL));
    entry_point = elf_getEntryPoint(elf_file);
    free(elf_file);
    return 0;
}

uintptr_t
get_last_vsyscall_table(void)
{
    return vsyscall_table;
}

uint64_t
get_last_entry_point(void)
{
    return entry_point;
}
