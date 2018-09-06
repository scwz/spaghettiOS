
#include <sel4/sel4.h>
#include <stdint.h>

#include "../uio.h"
#include "../vfs/vfs.h"
#include "../proc/proc.h"
#include "file_syscalls.h"

int syscall_write(void) {

    size_t nbyte = seL4_GetMR(2);
    int fd = seL4_GetMR(1);
    printf("write %d %d\n", fd, nbyte);
    if(fd < 4){ //send stdin etc. to console (make sure to open console)
        fd = 4;
    }
    struct vnode * vn = curproc->fdt->openfiles[fd]; 
    struct uio * u = malloc(sizeof(struct uio));
    uio_init(u, UIO_WRITE, nbyte);
    size_t bytes_written = VOP_WRITE(vn, u);
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
    struct vnode *vn = curproc->fdt->openfiles[fd]; 
    assert(vn);
    struct uio *u = malloc(sizeof(struct uio));
    uio_init(u, UIO_READ, nbyte);
    size_t bytes_read = VOP_READ(vn, u);
    printf("bytes_read %s, %d\n", shared_buf, bytes_read);
    free(u);
    seL4_SetMR(0, bytes_read);
    return 1;
}

int syscall_open(void) {

    fmode_t mode = seL4_GetMR(1);
    struct vnode *res;
    vfs_lookup(seL4_GetIPCBuffer()->msg + 2, &res); 
    VOP_EACHOPEN(res, 0xF);
    bool full = true;
    for(unsigned int i = 4; i < 8; i ++){
        if(curproc->fdt->openfiles[i] == NULL){
            curproc->fdt->openfiles[i] = res;
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
    VOP_RECLAIM(of->vn);
    free(of);
    curproc->fdt->openfiles[fd] = NULL;

    seL4_SetMR(0, 0);
    return 1;
}
