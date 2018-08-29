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
/****************************************************************************
 *
 *      $Id:  $
 *
 *      Description: Simple milestone 0 code.
 *      		     Libc will need sos_write & sos_read implemented.
 *
 *      Author:      Ben Leslie
 *
 ****************************************************************************/

#include <stdarg.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "ttyout.h"

#include <sel4/sel4.h>

void ttyout_init(void)
{
    /* Perform any initialisation you require here */
}

static size_t sos_debug_print(const void *vData, size_t count)
{
    size_t i;
    const char *realdata = vData;
    for (i = 0; i < count; i++) {
        seL4_DebugPutChar(realdata[i]);
    }
    return count;
}

size_t sos_write(void *vData, size_t count)
{
    seL4_MessageInfo_t tag;
    size_t total_bytes = 0;
    size_t buf_len = 10 * sizeof(seL4_Word);
    size_t npackets = ((count - 1) / buf_len) + 1;
    char *msg = vData;

    for (size_t i = 0; i < npackets; i++) {
        tag = seL4_MessageInfo_new(0, 0, 0, 2 + buf_len);

        // truncate junk data
        if (i == npackets - 1) {
            buf_len = count - (i * buf_len);
        }

        // send syscall, message length, and message in message registers
        seL4_SetMR(0, 1);
        seL4_SetMR(1, buf_len);
        memcpy(seL4_GetIPCBuffer()->msg + 2,
                msg + (i * 10 * sizeof(seL4_Word)), 
                buf_len);

        seL4_Call(SYSCALL_ENDPOINT_SLOT, tag);

        total_bytes += seL4_GetMR(0);
    }

    return total_bytes;
}

size_t sos_read(void *vData, size_t count)
{
    //implement this to use your syscall
    seL4_MessageInfo_t tag;
    size_t total_bytes = 0;
    size_t buf_len = 10 * sizeof(seL4_Word);
    size_t npackets = ((count - 1) / buf_len) + 1;
    int index = 0;
    char msg[count];

    tag = seL4_MessageInfo_new(0, 0, 0, 2);
    seL4_SetMR(0, 2);
    seL4_SetMR(1, count);
    seL4_Call(SYSCALL_ENDPOINT_SLOT, tag);

    total_bytes = seL4_GetMR(0);
    printf("%ld bytes received\n", total_bytes);

    return total_bytes;
}

