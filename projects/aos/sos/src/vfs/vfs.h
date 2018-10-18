#pragma once

#include <sos.h>

#include "vnode.h"

struct device_entry {
    char name[N_NAME];
    struct vnode *vn;
    struct device_entry *next;
};

struct vnode *root;

int vfs_lookup(char *path, struct vnode **result, bool create, pid_t pid);
int vfs_lookparent(char *path, struct vnode **result, char *buf, size_t buflen);

int vfs_open(char *path, int mode, struct vnode **ret, pid_t pid);
void vfs_close(struct vnode *vn, pid_t pid);

void vfs_bootstrap(void);

