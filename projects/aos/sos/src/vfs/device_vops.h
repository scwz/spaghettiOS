#include "vnode.h"
#include "../uio.h"

struct device{
    void * data;
    int (*open)(struct vnode * vn, int flags, pid_t pid);
    int (*write)( struct uio *uio);
    int (*read)( struct uio *uio);
    int (*close)(struct vnode * vn, pid_t pid);
};


struct vnode * dev_create_vnode();
void dev_uncreate_vnode(struct vnode *vn);
