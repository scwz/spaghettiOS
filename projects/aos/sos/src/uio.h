#pragma once

#include <stdint.h>

enum uio_rw {
    UIO_READ,
    UIO_WRITE
};

struct iovec {
    void *iov_base;
    size_t iov_len;
};

struct uio {
    struct iovec     *uio_iov;
    unsigned          uio_iovcnt;
    uint64_t          uio_offset;
    size_t            uio_resid;
    enum uio_rw       uio_rw;
    struct addrpsace *uio_space; 
};
