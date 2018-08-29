#pragma once

struct open_file {
    struct vnode *vn;
    uint64_t offset;
    int flags;
    size_t refcnt;
};

struct open_file **fdt_create(void);
void fdt_destroy(void);
