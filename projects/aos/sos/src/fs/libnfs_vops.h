#include "../vfs/vnode.h"

struct vnode_nfs_data {
	char * path;
	struct vnode * dev_list;
	struct vnode * next;
};

struct vnode * nfs_bootstrap();
struct vnode * nfs_create_vnode();
void nfs_uncreate_vnode(struct vnode *vn);