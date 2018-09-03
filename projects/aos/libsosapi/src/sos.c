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

#define SHARE_BUF             (0xD0000000)
#define SHARE_BUF_SIZE        256

static void check_len(size_t * len){
    if(*len > 4096 * SHARE_BUF_SIZE){
        *len = 4096 * SHARE_BUF_SIZE;
    }
}

static size_t user_copyin(void* user_vaddr, size_t len){
    check_len(&len);
    memcpy(SHARE_BUF - 4096 * SHARE_BUF_SIZE, user_vaddr, len);
    return len;
}
static size_t user_copyout(void* user_vaddr, size_t len){
    check_len(&len);
    memcpy(user_vaddr, SHARE_BUF - 4096 * SHARE_BUF_SIZE, len);
    return len;
}

int sos_sys_open(const char *path, fmode_t mode)
{
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, strlen(path) + 2);
    seL4_SetMR(0, SOS_SYS_OPEN);
    seL4_SetMR(1, mode);
    strcpy(seL4_GetIPCBuffer()->msg + 2, path);
    seL4_Call(SOS_IPC_EP_CAP, tag);

    if(seL4_GetMR(0)){
        return -1;
    }

    return seL4_GetMR(1);
}

int sos_sys_close(int file)  {
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, 2);
    seL4_SetMR(0, SOS_SYS_CLOSE);
    seL4_SetMR(1, file);
    seL4_Call(SOS_IPC_EP_CAP, tag);

    return 0;
}

int sos_sys_read(int file, char *buf, size_t nbyte)
{   
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, 3);
    seL4_SetMR(0, SOS_SYS_READ);
    seL4_SetMR(1, file);
    seL4_SetMR(2, nbyte);
    seL4_Call(SOS_IPC_EP_CAP, tag);

    size_t bytes_read = seL4_GetMR(0);
    user_copyout(buf, bytes_read);

    return bytes_read;
}

int sos_sys_write(int file, const char *buf, size_t nbyte)
{
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, 3);
    seL4_SetMR(0, SOS_SYS_WRITE);
    nbyte = user_copyin(buf, nbyte);
    seL4_SetMR(1, file);
    seL4_SetMR(2, nbyte);
    seL4_Call(SOS_IPC_EP_CAP, tag);

    return seL4_GetMR(0);
}

int sos_getdirent(int pos, char *name, size_t nbyte)
{
    assert(!"sos_getdirent not implemented!");
    return -1;
}

int sos_stat(const char *path, sos_stat_t *buf)
{
    assert(!"sos_stat not implemented!");
    return -1;
}

pid_t sos_process_create(const char *path)
{
    assert(!"sos_process_create not implemented!");
    return -1;
}

int sos_process_delete(pid_t pid)
{
    assert(!"sos_process_delete not implemented!");
    return -1;
}

pid_t sos_my_id(void)
{
    assert(!"sos_my_id not implemented!");
    return -1;

}

int sos_process_status(sos_process_t *processes, unsigned max)
{
    assert(!"sos_process_status not implemented!");
    return -1;
}

pid_t sos_process_wait(pid_t pid)
{
    assert(!"sos_process_wait not implemented!");
    return -1;

}

void sos_sys_usleep(int msec)
{
    if(msec < 0){
        return;
    }
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, 2);
    seL4_SetMR(0, SOS_SYS_USLEEP);
    seL4_SetMR(1, msec);
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
