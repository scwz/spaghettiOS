
#pragma once

#include "../vfs/vnode.h"
#include <sos.h>

struct vnode_nfs_data {
	char *path;
	struct device_entry *dev_list;
	struct vnode *next;
	struct nfsfh *nfsfh;
};

struct vnode *nfs_bootstrap();
struct vnode *nfs_create_vnode();
void nfs_uncreate_vnode(struct vnode *vn);
int nfs_get_statbuf(struct vnode *dir, char *path, sos_stat_t *statbuf, pid_t pid);
