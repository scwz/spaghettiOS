#include "vfs/vnode.h"
#include "uio.h"

struct device{
    bool readable;
    int (*open)(int flags);
    int (*write)( struct uio *uio);
    int (*read)( struct uio *uio);
    int (*close)();
};


struct vnode * dev_create_vnode();
void dev_uncreate_vnode(struct vnode *vn);
