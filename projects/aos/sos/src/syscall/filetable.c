
#include <sos.h>
#include "filetable.h"

struct filetable *fdt_create(void) {
    struct filetable *fdt = malloc(sizeof(struct filetable));

    for (int i = 0; i < PROCESS_MAX_FILES; i++) {
        fdt->openfiles[i] = NULL;
    }

    return fdt;
}

void fdt_destroy(struct filetable * fdt, pid_t pid) {
    for (int i = 0; i < PROCESS_MAX_FILES; i++) {
        if(fdt->openfiles[i]){
            struct vnode * vn = fdt->openfiles[i]->vn;
            VOP_RECLAIM(vn, pid);
            free(fdt->openfiles[i]);
        }
    }
    free(fdt);
}
