#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <sos.h>
#include "../vfs/vnode.h"

struct open_file {
    struct vnode *vn;
    uint64_t offset;
    int flags;
    size_t refcnt;
};

struct filetable {
    struct open_file *openfiles[PROCESS_MAX_FILES];
};

struct filetable *fdt_create(void);
void fdt_destroy(struct filetable *fdt, pid_t pid);
int fdt_place(struct filetable *fdt, struct vnode *vn, int mode, int *fd_ret);
int fdt_placeat(struct filetable *fdt, struct vnode *vn, int mode, int fd);
