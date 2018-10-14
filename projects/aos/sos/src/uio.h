#pragma once

#include <stdint.h>
#include "vm/shared_buf.h"

enum uio_rw {
    UIO_READ,
    UIO_WRITE
};
//kernel only uio in a shared buf

struct uio {
    enum uio_rw     uio_rw;
    size_t          len;  
    size_t          offset;  
    pid_t           pid;
};

size_t uiomove(void *ptr, size_t n, struct uio *uio);
void uio_init(struct uio * u, enum uio_rw rw, size_t len, size_t offset, pid_t pid);