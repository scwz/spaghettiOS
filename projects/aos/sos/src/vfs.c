#include "vfs.h"

int vfs_lookup(char *path, struct vnode **result) {

}

int vfs_lookparent(char *path, struct vnode **result, char *buf, size_t buflen) {

}

int vfs_open(char *path, int openflags, int mode, struct vnode **ret) {

}

void vfs_close(struct vnode *vn) {

}

void vfs_bootstrap(void) {

}
