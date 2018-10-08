#pragma once

#include "vnode.h"
struct device_entry {
    char name[20];
    struct vnode * vn;
    struct device_entry * next;
};

struct vnode * root;

int vfs_lookup(char *path, struct vnode **result, bool create);
int vfs_lookparent(char *path, struct vnode **result, char *buf, size_t buflen);

int vfs_open(char *path, int openflags, int mode, struct vnode **ret);
void vfs_close(struct vnode *vn);

void vfs_bootstrap(void);

