
#include <sel4/sel4.h>
#include <stdint.h>

#include "../uio.h"
#include "../vfs/vfs.h"
#include "../proc/proc.h"
#include "file_syscalls.h"
#include "../vm/shared_buf.h"
#include "../fs/libnfs_vops.h"
#include <sos.h>

int syscall_write(struct proc *curproc) {
    size_t nbyte = seL4_GetMR(2);
    int fd = seL4_GetMR(1);
    if(curproc->fdt->openfiles[fd] == NULL || nbyte <= 0){
        seL4_SetMR(0, 0);
        return 1;
    }
    
    if(!(curproc->fdt->openfiles[fd]->flags & FM_WRITE)){
        printf("why\n");
        seL4_SetMR(0, 0);
        return 1;
    }
    struct vnode * vn = curproc->fdt->openfiles[fd]->vn; 
    struct uio * u = malloc(sizeof(struct uio));
    uio_init(u, UIO_WRITE, nbyte, curproc->fdt->openfiles[fd]->offset, curproc->pid);
    size_t bytes_written = VOP_WRITE(vn, u);
    curproc->fdt->openfiles[fd]->offset += bytes_written;
    free(u);
    seL4_SetMR(0, bytes_written);
    return 1;
}

int syscall_read(struct proc *curproc) {
    size_t nbyte = seL4_GetMR(2);
    int fd = seL4_GetMR(1);
    if(curproc->fdt->openfiles[fd] == NULL || nbyte <= 0){
        seL4_SetMR(0, 0);
        return 1;
    }
    
    if(!(curproc->fdt->openfiles[fd]->flags & FM_READ)){
        seL4_SetMR(0, 0);
        return 1;
    }
    struct vnode *vn = curproc->fdt->openfiles[fd]->vn; 
    assert(vn);
    struct uio *u = malloc(sizeof(struct uio));
    uio_init(u, UIO_READ, nbyte, curproc->fdt->openfiles[fd]->offset, curproc->pid);
    size_t bytes_read = VOP_READ(vn, u);
    printf("bytes_read %s, %d\n", curproc->shared_buf, bytes_read);
    curproc->fdt->openfiles[fd]->offset += bytes_read;
    free(u);
    seL4_SetMR(0, bytes_read);
    return 1;
}

int syscall_open(struct proc *curproc) {
    fmode_t mode = seL4_GetMR(1);
    size_t size = seL4_GetMR(2);
    struct vnode *res;
    char path[size];
    sos_copyout(curproc->pid, (seL4_Word) path, size);
    printf("OPENING %s\n", path);
    if(vfs_lookup(path, &res, 1, curproc->pid)){
        seL4_SetMR(0, 1);
        return 1;
    } 
    if(VOP_EACHOPEN(res, mode, curproc->pid)){
        seL4_SetMR(0, 1);
        return 1;
    }
    bool full = true;
    for(unsigned int i = 0; i < PROCESS_MAX_FILES; i ++){
        if(curproc->fdt->openfiles[i] == NULL){
            curproc->fdt->openfiles[i] = malloc(sizeof(struct open_file));
            curproc->fdt->openfiles[i]->vn = res;
            curproc->fdt->openfiles[i]->refcnt = 1;
            curproc->fdt->openfiles[i]->offset = 0;
            curproc->fdt->openfiles[i]->flags = mode;
            seL4_SetMR(0, 0);
            seL4_SetMR(1, i);
            full = false;
            //printf("i: %d, flags %d\n", i, mode);
            break;
        }
    } 
    if(full){
        seL4_SetMR(0, 1);
    }
    return 2;
}

int syscall_close(struct proc *curproc) {
    
    int fd = seL4_GetMR(1);
    struct open_file *of = curproc->fdt->openfiles[fd];
    VOP_RECLAIM(of->vn, curproc->pid);
    free(of);
    curproc->fdt->openfiles[fd] = NULL;
    seL4_SetMR(0, 0);
    //printf("CLOSING\n");
    return 1;
}

int syscall_getdirent(struct proc *curproc){
    int pos = seL4_GetMR(1);
    size_t nbyte = seL4_GetMR(2);
    char path[nbyte];
    sos_copyout(curproc->pid, (seL4_Word) path, nbyte);
    printf("stat path %s\n", path);
    struct  vnode * res;
    if (vfs_lookup("", &res, 0, curproc->pid)){
        seL4_SetMR(0, 0);
        return 1;
    }
    struct uio *u = malloc(sizeof(struct uio));
    uio_init(u, UIO_READ, pos, 0, curproc->pid);
    size_t bytes = VOP_GETDIRENTRY(res, u);
    seL4_SetMR(0, bytes);
    free(u);
    printf("end dirent\n");
    return 1;
}

int syscall_stat(struct proc *curproc){
    size_t nbyte = seL4_GetMR(1);
    char path[nbyte];
    sos_copyout(curproc->pid, (seL4_Word) path, nbyte);
    printf("stat path %s\n", path);
    sos_stat_t buf;
    struct vnode * res;
    if (vfs_lookup("", &res, 0, curproc->pid)){
        seL4_SetMR(0, -1);
        return 1;
    }
    if (nfs_get_statbuf(res, path, &buf)){
        seL4_SetMR(0, -1);
        return 1;
    }
    sos_copyin(curproc->pid, (seL4_Word) &buf, sizeof(buf));
    seL4_SetMR(0, 0);
    return 1;
}

