
#pragma once

#include <stdint.h>
#include <sos.h>
#include "../vfs/vnode.h"

struct console {
    void *reader;
};

int console_init(void);
int console_open(struct vnode *vn, int flags, pid_t pid);
int console_close(struct vnode *vn, pid_t pid);
int console_read(struct uio *uio);
int console_write(struct uio *uio);
