
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
char buffer[8192];
static int nbytes_read = 0;
static int reading = 1;

static void console_handler(struct serial *serial, char c) {
	//bufferWrite(sb_ptr, c);
    if (reading) {
        buffer[nbytes_read++] = c;
        if (c == '\n') {
            resume(curr, NULL);
        }
    }
}

int console_init(void) {
    serial = serial_init();
    serial_register_handler(serial, console_handler);
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
    reading = 1;
	curr = get_running();
    yield(NULL);

	char msg[uio->len];
	unsigned int i = 0;
    while (i < uio->len && i < nbytes_read) {
        msg[i] = buffer[i];
        i++;
	}
    printf("i %ld\n", i);
    printf("HANDLED %d\n", nbytes_read);
    reading = 0;
    nbytes_read = 0;

	msg[i] = '\0'; 
	printf("msg: %s\n", msg);
	sos_copyin(msg, i);
	return i;
}

int console_write(struct uio *uio) {
	return serial_send(serial, shared_buf, uio->len);
}
