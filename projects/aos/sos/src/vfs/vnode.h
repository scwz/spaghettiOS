#pragma once

#include <stdint.h>
#include <stdlib.h>
#include "../uio.h"

/* file modes */
#define FM_EXEC  1
#define FM_WRITE 2
#define FM_READ  4

struct vnode {
    int vn_refcount;
    void *vn_data;
    const struct vnode_ops *vn_ops;
};

#define VOP_MAGIC   (0xa2b3c4d5)

struct vnode_ops {
    unsigned long vop_magic;        /* should always be VOP_MAGIC */
    
    int (*vop_eachopen)(struct vnode *object, int flags_from_open);
    int (*vop_reclaim)(struct vnode *vnode);

    int (*vop_read)(struct vnode *file, struct uio *uio);
    int (*vop_getdirentry)(struct vnode *dir, struct uio *uio);
    int (*vop_write)(struct vnode *file, struct uio *uio);
    int (*vop_stat)(struct vnode *object, int statbuf); // statbuf is a placeholder for struct stat *statbuf

    int (*vop_creat)(struct vnode *dir, const char *name, int excl, int mode, struct vnode **result);

    int (*vop_lookup)(struct vnode *dir, char *pathname, struct vnode **result);
    int (*vop_lookparent)(struct vnode *dir, char *pathname, struct vnode **result, char *buf, size_t len);
};

void vnode_incref(struct vnode *);
void vnode_decref(struct vnode *);

#define VOP_INCREF(vn) 			vnode_incref(vn)
#define VOP_DECREF(vn) 			vnode_decref(vn)

#define __VOP(vn, sym) (vnode_check(vn, #sym), (vn)->vn_ops->vop_##sym)

#define VOP_EACHOPEN(vn, flags)             (__VOP(vn, eachopen)(vn, flags))
#define VOP_RECLAIM(vn)                     (__VOP(vn, reclaim)(vn))

#define VOP_READ(vn, uio)                   (__VOP(vn, read)(vn, uio))
#define VOP_GETDIRENTRY(vn, uio)            (__VOP(vn, getdirentry)(vn, uio))
#define VOP_WRITE(vn, uio)                  (__VOP(vn, write)(vn, uio))
#define VOP_STAT(vn, ptr)                   (__VOP(vn, stat)(vn, ptr))

#define VOP_CREAT(vn,nm,excl,mode,res)      (__VOP(vn, creat)(vn,nm,excl,mode,res)) 

#define VOP_LOOKUP(vn, name, res)           (__VOP(vn, lookup)(vn, name, res))
#define VOP_LOOKPARENT(vn,name,res,bf,ln)   (__VOP(vn, lookparent)(vn,name,res,bf,ln))

void vnode_check(struct vnode *, const char *op);
int vnode_init(struct vnode *, const struct vnode_ops *ops, void *fsdata);

void vnode_cleanup(struct vnode *);
