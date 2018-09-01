#include "uio.h"
#include "shared_buf.h"

//uio only works in kernel, use sharebuffer commands in userland
void uio_init(struct uio * u, enum uio_rw rw, size_t len)
{
	u->uio_rw = rw;
    u->len = len;
}

size_t
uiomove(void *ptr, size_t n, struct uio *u)
{
    //can only copy in max buff size anyway, do not loop
    if(n > u->len){
        n = u->len;
    }

    if (u->uio_rw == UIO_READ) {
        sos_copyin(ptr, n);
    } else {
        sos_copyout(ptr, n);
    }
    return n;
}