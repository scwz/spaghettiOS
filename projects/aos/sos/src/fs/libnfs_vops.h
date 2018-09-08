#include "../vfs/vnode.h"

struct vnode * nfs_bootstrap();
struct vnode * nfs_create_vnode();
void nfs_uncreate_vnode(struct vnode *vn);