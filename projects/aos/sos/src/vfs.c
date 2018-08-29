#include "vfs.h"
#include "vnode.h"

int vfs_lookup(char *path, struct vnode **result) {
    return 0;
}

int vfs_lookparent(char *path, struct vnode **result, char *buf, size_t buflen) {
    return 0;
}

int vfs_open(char *path, int openflags, int mode, struct vnode **ret) {
    return 0;
}

void vfs_close(struct vnode *vn) {

}

void vfs_bootstrap(void) {

}
