
#include <sel4/sel4.h>
#include <stdint.h>

#include "../uio.h"
#include "../vfs/vfs.h"
#include "../proc/proc.h"
#include "file_syscalls.h"
#include "../shared_buf.h"

int syscall_write(void) {

    size_t nbyte = seL4_GetMR(2);
    int fd = seL4_GetMR(1);
    printf("write %d %d\n", fd, nbyte);
    if(fd < 4){ //send stdin etc. to console (make sure to open console)
        fd = 4;
    }
    struct vnode * vn = curproc->fdt->openfiles[fd]->vn; 
    struct uio * u = malloc(sizeof(struct uio));
    uio_init(u, UIO_WRITE, nbyte, curproc->fdt->openfiles[fd]->offset);
    size_t bytes_written = VOP_WRITE(vn, u);
    curproc->fdt->openfiles[fd]->offset += bytes_written;
    free(u);
    seL4_SetMR(0, bytes_written);
    return 1;
}

int syscall_read(void) {
    size_t nbyte = seL4_GetMR(2);
    int fd = seL4_GetMR(1);
    if(fd < 4){ //send stdin etc. to console (make sure to open console)
        fd = 4;
    }
    struct vnode *vn = curproc->fdt->openfiles[fd]->vn; 
    assert(vn);
    struct uio *u = malloc(sizeof(struct uio));
    uio_init(u, UIO_READ, nbyte, curproc->fdt->openfiles[fd]->offset);
    size_t bytes_read = VOP_READ(vn, u);
    printf("bytes_read %s, %d\n", shared_buf, bytes_read);
    curproc->fdt->openfiles[fd]->offset += bytes_read;
    free(u);
    seL4_SetMR(0, bytes_read);
    return 1;
}

int syscall_open(void) {

    fmode_t mode = seL4_GetMR(1);
    size_t size = seL4_GetMR(2);
    struct vnode *res;
    char path[size];
    sos_copyout(path, size);
    if(vfs_lookup(path, &res)){
        seL4_SetMR(0, 1);
        return 1;
    } 
    printf("OPENING\n");
    if(VOP_EACHOPEN(res, mode)){
        seL4_SetMR(0, 1);
        return 1;
    }
    bool full = true;
    for(unsigned int i = 4; i < 8; i ++){
        if(curproc->fdt->openfiles[i] == NULL){
            curproc->fdt->openfiles[i] = malloc(sizeof(struct open_file));
            curproc->fdt->openfiles[i]->vn = res;
            curproc->fdt->openfiles[i]->refcnt = 1;
            curproc->fdt->openfiles[i]->offset = 0;
            curproc->fdt->openfiles[i]->flags = mode;
            seL4_SetMR(0, 0);
            seL4_SetMR(1, i);
            full = false;
            printf("i: %d\n", i);
            break;
        }
    } 
    if(full){
        seL4_SetMR(0, 1);
    }
    printf("i : %d\n", seL4_GetMR(1));
    return 2;
}

int syscall_close(void) {
    
    int fd = seL4_GetMR(1);
    
    struct open_file *of = curproc->fdt->openfiles[fd];
    printf("CLOSING %d, %x\n", fd, of);
    VOP_RECLAIM(of->vn);
    free(of);
    curproc->fdt->openfiles[fd] = NULL;

    seL4_SetMR(0, 0);
    printf("CLOSING\n");
    return 1;
}
int syscall_getdirent(void){
    int pos = seL4_GetMR(1);
    size_t nbyte = seL4_GetMR(2);
    char path[nbyte];
    sos_copyout(path, nbyte);
    struct  vnode * res;
    if (vfs_lookup("", &res)){
        seL4_SetMR(0, 0);
        return 1;
    }
    struct uio *u = malloc(sizeof(struct uio));
    uio_init(u, UIO_READ, pos, 0);
    size_t bytes = VOP_GETDIRENTRY(res, u);
    printf("SHARED BUF: %s", shared_buf);
    seL4_SetMR(0, bytes);
    free(u);
    return 1;
}

int syscall_stat(void){
    size_t nbyte = seL4_GetMR(1);
    char path[nbyte];
    sos_copyout(path, nbyte);
    printf("stat path %d\n", path);
    struct  vnode * res;
    if (vfs_lookup(path, &res)){
        seL4_SetMR(0, 0);
        return 1;
    }
    sos_stat_t buf;
    if(VOP_STAT(res, &buf)){
        seL4_SetMR(0, -1);
        return 1;
    }
    sos_copyin(&buf, sizeof(buf));
    seL4_SetMR(0, 0);
    return 1;
}
