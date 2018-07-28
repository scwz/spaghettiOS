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
#include <math.h>

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
    size_t nbytes = count;
    size_t bytes_sent = 0;
    size_t buf_size = sizeof(seL4_Word);
    size_t npackets = ((nbytes - 1) / sizeof(seL4_Word)) + 1;
    char *msg = vData;

    for (size_t i = 0; i < npackets; i++) {
        sos_debug_print("sending...\n", 11);
        tag = seL4_MessageInfo_new(0, 0, 0, 2);

        seL4_SetMR(0, 1);
        memcpy((char *)seL4_GetIPCBuffer()->msg + sizeof(seL4_Word), &msg[i * sizeof(seL4_Word)], buf_size);

        seL4_Call(SYSCALL_ENDPOINT_SLOT, tag);
    }
#if 0
    while (nbytes > 0) {
        if (nbytes < buf_size) {
            buf_size = nbytes;
        }
        tag = seL4_MessageInfo_new(0, 0, 0, 2);

        // invoke write syscall
        seL4_SetMR(0, 1);
        memcpy((char *)seL4_GetIPCBuffer()->msg + sizeof(seL4_Word), &msg[count - nbytes], buf_size);

        seL4_Call(SYSCALL_ENDPOINT_SLOT, tag);

        bytes_sent = seL4_GetMR(0);
        nbytes -= bytes_sent;
        char *debug;
        sprintf(debug, "%lu bytes\n", nbytes);
        sos_debug_print(debug, 8);
    }
    for (int i = 0; i < count; i++) {
        tag = seL4_MessageInfo_new(0, 0, 0, 2);
        // invoke write syscall 
        seL4_SetMR(0, 1); 
        // send a character 
        seL4_SetMR(1, msg[i]);
        seL4_Call(SYSCALL_ENDPOINT_SLOT, tag);
    }
#endif

    return count;
}

size_t sos_read(void *vData, size_t count)
{
    //implement this to use your syscall
    return 0;
}

