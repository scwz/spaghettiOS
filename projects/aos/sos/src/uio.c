#include "uio.h"

//uio only works in kernel, use sharebuffer commands in userland
void 
uio_init(struct uio *u, enum uio_rw rw, size_t len, size_t offset, pid_t pid)
{
	u->uio_rw = rw;
    u->len = len;
    u->offset = offset;
    u->pid = pid;
}

size_t
uiomove(void *ptr, size_t n, struct uio *u)
{
    //can only copy in max buff size anyway, do not loop
    if(n > u->len){
        n = u->len;
    }

    if (u->uio_rw == UIO_READ) {
        sos_copyin(u->pid, (seL4_Word) ptr, n);
    } else {
        sos_copyout(u->pid, (seL4_Word) ptr, n);
    }
    return n;
}
