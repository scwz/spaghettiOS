#include "vnode.h"


int
vnode_init(struct vnode *vn, const struct vnode_ops *ops, void *fsdata)
{
	assert(vn != NULL);
	assert(ops != NULL);

	vn->vn_ops = ops;
	vn->vn_refcount = 1;
	vn->vn_data = fsdata;
	return 0;
}

void
vnode_cleanup(struct vnode *vn)
{
	assert(vn->vn_refcount == 1);
	vn->vn_ops = NULL;
	vn->vn_refcount = 0;
	vn->vn_data = NULL;
}

void
vnode_incref(struct vnode *vn)
{
	vn->vn_refcount++;
}

void
vnode_decref(struct vnode *vn)
{
	bool destroy;
	int result;

	assert(vn->vn_refcount > 0);
	if (vn->vn_refcount > 1) {
		vn->vn_refcount--;
		destroy = false;
	}
	else {
		/* Don't decrement; pass the reference to VOP_RECLAIM. */
		destroy = true;
	}

	if (destroy) {
		result = VOP_RECLAIM(vn);
		if (result != 0) {
			// XXX: lame.
			ZF_LOGE("vfs: Warning: VOP_RECLAIM FAILED");
		}
	}
}

void
vnode_check(struct vnode *v, const char *opstr)
{
	/* not safe, and not really needed to check constant fields */
	/*vfs_biglock_acquire();*/

	if (v == NULL) {
		ZF_LOGE("vnode_check: vop_%s: null vnode\n", opstr);
	}
	if (v == (void *)0xdeadbeef) {
		ZF_LOGE("vnode_check: vop_%s: deadbeef vnode\n", opstr);
	}

	if (v->vn_ops == NULL) {
		ZF_LOGE("vnode_check: vop_%s: null ops pointer\n", opstr);
	}
	if (v->vn_ops == (void *)0xdeadbeef) {
		ZF_LOGE("vnode_check: vop_%s: deadbeef ops pointer\n", opstr);
	}


	if (v->vn_refcount < 0) {
		ZF_LOGE("vnode_check: vop_%s: negative refcount %d\n", opstr,
		      v->vn_refcount);
	}
	else if (v->vn_refcount == 0) {
		ZF_LOGE("vnode_check: vop_%s: zero refcount\n", opstr);
	}
	else if (v->vn_refcount > 0x100000) {
		printf("vnode_check: vop_%s: warning: large refcount %d\n",
			opstr, v->vn_refcount);
	}

	/*vfs_biglock_release();*/
}