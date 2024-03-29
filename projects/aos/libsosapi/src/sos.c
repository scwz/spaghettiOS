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

#define PAGE_SIZE_4K 4096

static void 
check_len(size_t *len){
    if (*len > PAGE_SIZE_4K * SHARE_BUF_SIZE) {
        *len = PAGE_SIZE_4K * SHARE_BUF_SIZE;
    }
}

static size_t
user_copyin(void *user_vaddr, size_t len)
{
    check_len(&len);
    memcpy(SHARE_BUF - PAGE_SIZE_4K * SHARE_BUF_SIZE, user_vaddr, len);
    return len;
}

static size_t 
user_copyout(void *user_vaddr, size_t len)
{
    check_len(&len);
    memcpy(user_vaddr, SHARE_BUF - PAGE_SIZE_4K * SHARE_BUF_SIZE, len);
    return len;
}

int 
sos_sys_open(const char *path, fmode_t mode)
{
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, 3);
    seL4_SetMR(0, SOS_SYS_OPEN);
    seL4_SetMR(1, mode);
    seL4_SetMR(2, strlen(path)+1);
    user_copyin(path, strlen(path)+1);
    seL4_Call(SOS_IPC_EP_CAP, tag);

    if (seL4_GetMR(0)) {
        return -1;
    }

    return seL4_GetMR(1);
}

int 
sos_sys_close(int file)  
{
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, 2);
    seL4_SetMR(0, SOS_SYS_CLOSE);
    seL4_SetMR(1, file);
    seL4_Call(SOS_IPC_EP_CAP, tag);

    return seL4_GetMR(0);
}

int
sos_sys_read(int file, char *buf, size_t nbyte)
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

int 
sos_sys_write(int file, const char *buf, size_t nbyte)
{
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, 3);
    seL4_SetMR(0, SOS_SYS_WRITE);
    nbyte = user_copyin(buf, nbyte);
    seL4_SetMR(1, file);
    seL4_SetMR(2, nbyte);
    seL4_Call(SOS_IPC_EP_CAP, tag);

    return seL4_GetMR(0);
}

int 
sos_getdirent(int pos, char *name, size_t nbyte)
{
    assert(pos >= 0);
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, 3);
    seL4_SetMR(0, SOS_SYS_GETDIRENT);
    nbyte = user_copyin(name, nbyte);
    seL4_SetMR(1, pos);
    seL4_SetMR(2, nbyte);
    seL4_Call(SOS_IPC_EP_CAP, tag);
    user_copyout(name, seL4_GetMR(0));
    return seL4_GetMR(0);
}

int 
sos_stat(const char *path, sos_stat_t *buf)
{
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, 2);
    seL4_SetMR(0, SOS_SYS_STAT);
    size_t nbyte = user_copyin(path, strlen(path)+1);
    seL4_SetMR(1, nbyte);
    seL4_Call(SOS_IPC_EP_CAP, tag);
    if (seL4_GetMR(0)) {
        return seL4_GetMR(0);
    }
    user_copyout(buf, sizeof(sos_stat_t));
    return seL4_GetMR(0);
}

pid_t 
sos_process_create(const char *path)
{
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, 2);
    seL4_SetMR(0, SOS_PROC_CREATE);

    size_t nbytes = user_copyin(path, strlen(path)+1);
    seL4_SetMR(1, nbytes);
    seL4_Call(SOS_IPC_EP_CAP, tag);

    return seL4_GetMR(0);
}

int 
sos_process_delete(pid_t pid)
{
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, 2);
    seL4_SetMR(0, SOS_PROC_DELETE);
    seL4_SetMR(1, pid);
    seL4_Call(SOS_IPC_EP_CAP, tag);

    return seL4_GetMR(0);
}

pid_t 
sos_my_id(void)
{
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, 1);
    seL4_SetMR(0, SOS_PROC_MY_ID);
    seL4_Call(SOS_IPC_EP_CAP, tag);

    return seL4_GetMR(0);
}

int
sos_process_status(sos_process_t *processes, unsigned max)
{
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, 2);
    seL4_SetMR(0, SOS_PROC_STATUS);
    seL4_SetMR(1, max);
    seL4_Call(SOS_IPC_EP_CAP, tag);

    size_t nactive = seL4_GetMR(0);
    user_copyout(processes, nactive * sizeof(sos_process_t));

    return nactive;
}

pid_t 
sos_process_wait(pid_t pid)
{
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, 2);
    seL4_SetMR(0, SOS_PROC_WAIT);
    seL4_SetMR(1, pid);
    seL4_Call(SOS_IPC_EP_CAP, tag);

    return seL4_GetMR(0);

}

void 
sos_sys_usleep(int msec)
{
    if (msec < 0) {
        return;
    }
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, 2);
    seL4_SetMR(0, SOS_SYS_USLEEP);
    seL4_SetMR(1, msec);
    seL4_Call(SOS_IPC_EP_CAP, tag);
}

int64_t 
sos_sys_time_stamp(void)
{
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, 1);

    seL4_SetMR(0, SOS_SYS_TIME_STAMP); 
    seL4_Call(SOS_IPC_EP_CAP, tag);

    return seL4_GetMR(0);
}

long 
sos_sys_brk(uintptr_t newbrk) 
{
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, 2);

    seL4_SetMR(0, SOS_SYS_BRK);
    seL4_SetMR(1, newbrk);
    seL4_Call(SOS_IPC_EP_CAP, tag);

    return seL4_GetMR(1);
}
