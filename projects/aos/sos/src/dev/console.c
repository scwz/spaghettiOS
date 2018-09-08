
#include <stdint.h>
#include <sos.h>
#include <serial/serial.h>

#include "console.h"
#include "../ringbuffer.h"
#include "../picoro/picoro.h"
#include "../vfs/vnode.h"
#include "../vfs/device_vops.h"
#include "../proc/proc.h"

static struct serial *serial;
static coro curr;
ringBuffer_typedef(char, stream_buf);
static stream_buf *sb_ptr;
char buffer[8192];

/*
static const vnode_ops console_vnode_ops = {
    .vop_magic = VOP_MAGIC,

    .vop_open = console_open,
    .vop_close = console_close,
    .vop_read = console_read,
    .vop_write = console_write
};
*/
static int nbytes_read = 0;

static void console_handler(struct serial *serial, char c) {
	//bufferWrite(sb_ptr, c);
    buffer[nbytes_read++] = c;
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

    /*
	stream_buf sb;
	sb_ptr = malloc(sizeof(stream_buf));
	bufferInit(sb, 8192, char);
	memcpy(sb_ptr, &sb, sizeof(stream_buf));
    */

    return 0;
}

int console_open(struct vnode * vn,int flags) {
    struct device * d = vn->vn_data;
    struct console * c = d->data;
    printf("flags %x, c->reader = %d\n", flags, c->reader);
    if(flags  & FM_READ){
        if(c->reader != NULL){
            return -1;
        }
        c->reader = curproc;
    }
    return 0;
}

int console_close(struct vnode * vn) {
    struct device * d = vn->vn_data;
    struct console * c = d->data;
    if(c->reader == curproc){
        c->reader = NULL;
    }
    return 0;
}

int console_read(struct uio *uio) {
	curr = get_running();
    yield(NULL);

	char msg[uio->len];
	unsigned int i = 0;
    while (i < uio->len) {
	//while (!isBufferEmpty(sb_ptr) && i < uio->len) {
		char c;
		//bufferRead(sb_ptr, c);
		//msg[i++] = c;
        msg[i] = buffer[i];
        i++;
	}
    nbytes_read = 0;
    printf("LEN %ld\n", uio->len);
    printf("HANDLED %d\n", nbytes_read);

	msg[i] = '\0'; 
	printf("msg: %s\n", msg);
	sos_copyin(msg, i);
	return i;
}

int console_write(struct uio *uio) {
	return serial_send(serial, shared_buf, uio->len);
}
