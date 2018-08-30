/*
 * Copyright 2018, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(DATA61_GPL)
 */
#include <stdarg.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sos.h>

#include <sel4/sel4.h>

int sos_sys_open(const char *path, fmode_t mode)
{
    assert(!"You need to implement this");
    return 0;
}

int sos_sys_close(int file)  {
    assert(!"You need to implement this");
    return -1;
}

int sos_sys_read(int file, char *buf, size_t nbyte)
{
    assert(!"You need to implement this");
    return -1;
}

int sos_sys_write(int file, const char *buf, size_t nbyte)
{
    seL4_MessageInfo_t tag;
    size_t total_bytes = 0;
    size_t buf_len = 10 * sizeof(seL4_Word);
    size_t npackets = ((nbyte - 1) / buf_len) + 1;

    for (size_t i = 0; i < npackets; i++) {
        tag = seL4_MessageInfo_new(0, 0, 0, 2 + buf_len);

        // truncate junk data
        if (i == npackets - 1) {
            buf_len = nbyte - (i * buf_len);
        }

        // send syscall, message length, and message in message registers
        seL4_SetMR(0, 1);
        seL4_SetMR(1, buf_len);
        memcpy(seL4_GetIPCBuffer()->msg + 2,
                buf + (i * 10 * sizeof(seL4_Word)), 
                buf_len);

        seL4_Call(SOS_IPC_EP_CAP, tag);

        total_bytes += seL4_GetMR(0);
    }

    return total_bytes;
}

int sos_getdirent(int pos, char *name, size_t nbyte)
{
    assert(!"You need to implement this");
    return -1;
}

int sos_stat(const char *path, sos_stat_t *buf)
{
    assert(!"You need to implement this");
    return -1;
}

pid_t sos_process_create(const char *path)
{
    assert(!"You need to implement this");
    return -1;
}

int sos_process_delete(pid_t pid)
{
    assert(!"You need to implement this");
    return -1;
}

pid_t sos_my_id(void)
{
    assert(!"You need to implement this");
    return -1;

}

int sos_process_status(sos_process_t *processes, unsigned max)
{
    assert(!"You need to implement this");
    return -1;
}

pid_t sos_process_wait(pid_t pid)
{
    assert(!"You need to implement this");
    return -1;

}

void sos_sys_usleep(int msec)
{
    if(msec < 0){
        return;
    }
    seL4_MessageInfo_t tag;
    seL4_SetMR(0, SOS_SYS_USLEEP);
    seL4_SetMR(1, msec);
    tag = seL4_MessageInfo_new(0, 0, 0, 2);
    seL4_Call(SOS_IPC_EP_CAP, tag);
    //printf("end sleep\n");
}

int64_t sos_sys_time_stamp(void)
{
    assert(!"You need to implement this");
    return -1;
}

long sos_sys_brk(uintptr_t newbrk) {
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, 2);

    seL4_SetMR(0, 5);
    seL4_SetMR(1, newbrk);
    seL4_Call(SOS_IPC_EP_CAP, tag);

    return seL4_GetMR(1);
}
