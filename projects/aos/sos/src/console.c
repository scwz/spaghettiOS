
#include <stdint.h>
#include <sos.h>
#include <serial/serial.h>
#include "console.h"
#include "ringbuffer.h"
#include "picoro.h"
#include "vnode.h"

static struct serial *serial;
static coro curr;
ringBuffer_typedef(char, stream_buf);
static stream_buf *sb_ptr;

/*
static const vnode_ops console_vnode_ops = {
    .vop_magic = VOP_MAGIC,

    .vop_open = console_open,
    .vop_close = console_close,
    .vop_read = console_read,
    .vop_write = console_write
};
*/

static void console_handler(struct serial *serial, char c) {
	bufferWrite(sb_ptr, c)
    if (c == '\n') {
       	resume(curr, NULL);
	}
}

int console_init(void) {
    /*
    struct vnode *vn = malloc(sizeof(struct vnode *));
    vnode_init(vn, &console_vnode_ops, NULL);
    */
    
    serial = serial_init();
    serial_register_handler(serial, console_handler);

	stream_buf sb;
	sb_ptr = malloc(sizeof(stream_buf));
	bufferInit(sb, 1024, char);
	memcpy(sb_ptr, &sb, sizeof(stream_buf));

    return 0;
}

int console_open(int flags) {
    // does nothing atm
    (void) flags;
    return 0;
}

int console_close(void) {
    // should never be closed
    return 0;
}

int console_read(struct uio *uio) {
	curr = get_running();
    yield(NULL);

	char msg[uio->len];
	int i = 0;
	while (!isBufferEmpty(sb_ptr) && i < uio->len) {
		char c;
		bufferRead(sb_ptr, c);
		msg[i++] = c;
	}

	msg[i++] = '\0'; 
	printf("msg: %s\n", msg);
	sos_copyin(msg, i);
	return i;
}

int console_write(struct uio *uio) {
	return serial_send(serial, shared_buf, uio->len);
}
