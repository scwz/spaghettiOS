
#include <sos.h>
#include "filetable.h"

struct filetable *
fdt_create(void) 
{
    struct filetable *fdt = malloc(sizeof(struct filetable));

    for (int i = 0; i < PROCESS_MAX_FILES; i++) {
        fdt->openfiles[i] = NULL;
    }

    return fdt;
}

void 
fdt_destroy(struct filetable *fdt, pid_t pid) 
{
    for (int i = 0; i < PROCESS_MAX_FILES; i++) {
        if (fdt->openfiles[i]) {
            struct vnode *vn = fdt->openfiles[i]->vn;
            VOP_RECLAIM(vn, pid);
            free(fdt->openfiles[i]);
            fdt->openfiles[i] = NULL;
        }
    }
    free(fdt);
}

int 
fdt_place(struct filetable *fdt, struct vnode *vn, int mode, int *fd_ret)
{
    struct open_file *of = malloc(sizeof(struct open_file));
    of->vn = vn;
    of->refcnt = 1;
    of->offset = 0;
    of->flags = mode;

    for (unsigned int i = 0; i < PROCESS_MAX_FILES; i ++) {
        if (fdt->openfiles[i] == NULL) {
            fdt->openfiles[i] = of;
            *fd_ret = i;
            return 0;
        }
    } 

    free(of);
    return -1;
}

int
fdt_placeat(struct filetable *fdt, struct vnode *vn, int mode, int fd) 
{
    struct open_file *of = malloc(sizeof(struct open_file));
    of->vn = vn;
    of->refcnt = 1;
    of->offset = 0;
    of->flags = mode;

    fdt->openfiles[fd] = of;

    return -1;

}
