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
    seL4_MessageInfo_t tag;
    seL4_SetMR(0, SOS_SYS_OPEN);
    seL4_SetMR(1, mode);
    strcpy(seL4_GetIPCBuffer()->msg + 2, path);
    tag = seL4_MessageInfo_new(0, 0, 0, strlen(path) + 2);
    seL4_Call(SOS_IPC_EP_CAP, tag);
    if(seL4_GetMR(0)){
        return -1;
    }
    return seL4_GetMR(1);
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
    seL4_SetMR(0, SOS_SYS_WRITE);
    seL4_SetMR(1, file);
    seL4_SetMR(2, nbyte);
    //user_copyin(buf, nbyte);
    tag = seL4_MessageInfo_new(0, 0, 0, 3);
    seL4_Call(SOS_IPC_EP_CAP, tag);
    return seL4_GetMR(1);
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
    int64_t time;
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, 1);

    seL4_SetMR(0, SOS_SYS_TIME_STAMP); 
    seL4_Call(SOS_IPC_EP_CAP, tag);

    return seL4_GetMR(0);
}

long sos_sys_brk(uintptr_t newbrk) {
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, 2);

    seL4_SetMR(0, SOS_SYS_BRK);
    seL4_SetMR(1, newbrk);
    seL4_Call(SOS_IPC_EP_CAP, tag);

    return seL4_GetMR(1);
}
