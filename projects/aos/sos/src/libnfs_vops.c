#include "vfs.h"
#include "vnode.h"

static int nfs_eachopen(struct vnode *v, int flags)
{
	struct device *d = v->vn_data;

    
	return 0;
}

static
int
nfs_write(struct vnode *v, struct uio *uio)
{
	struct device *d = v->vn_data;
	int result;

	//just write here

	return 0;
}

static
int
nfs_read(struct vnode *v, struct uio *uio)
{
	struct device *d = v->vn_data;
	int result;
    // do the read here
	return 0;
}

static int nfs_getdirent(){
    return 0;
}

static int nfs_stat(){
    return 0;
}

static int nfs_lookup(){
    return 0;
}

static int nfs_lookparent(){
    return 0;
}

static const struct vnode_ops nfs_vnode_ops = {
	.vop_magic = VOP_MAGIC,

	.vop_eachopen = nfs_eachopen,

	.vop_read = nfs_read,
    .vop_getdirentry = nfs_getdirent,

	.vop_write = nfs_write,
	
	.vop_stat = nfs_stat,
	.vop_lookup = nfs_lookup,
	.vop_lookparent = nfs_lookparent,
};

struct vnode *
nfs_create_vnode()
{
	int result;
	struct vnode *v;

	v = malloc(sizeof(struct vnode));
	if (v==NULL) {
		return NULL;
	}

	result = vnode_init(v, &nfs_vnode_ops,  NULL);
	if (result != 0) {
		ZF_LOGV("While creating vnode for device: vnode_init: %s\n",
		      strerror(result));
	}

	return v;
}

void
nfs_uncreate_vnode(struct vnode *vn)
{
	assert(vn->vn_ops == &nfs_vnode_ops);
	vnode_cleanup(vn);
	free(vn);
}