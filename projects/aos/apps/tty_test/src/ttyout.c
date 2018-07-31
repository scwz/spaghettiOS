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
    
    char* byte_ptr = (char*) vData;
    size_t bytes_to_write = count;
    seL4_IPCBuffer * buf = seL4_GetIPCBuffer();

    while(bytes_to_write){
        size_t write_length = 0;
        if (bytes_to_write > sizeof(seL4_Word) * 118){
            write_length = sizeof(seL4_Word) * 118;
        } else {
            write_length = bytes_to_write;
        }
        int words_needed = (write_length + sizeof(seL4_Word) - 1)/sizeof(seL4_Word);
        for(int i = 0; i < words_needed; i++){
            buf->msg[i+2] = ((seL4_Word *) byte_ptr)[i];
        }
        buf->msg[0] = 2; //syscall num
        buf->msg[1] = write_length; //number of bytes to write
        byte_ptr = byte_ptr + write_length;
        bytes_to_write = bytes_to_write - write_length;
        seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, words_needed+2);
        seL4_Send(SYSCALL_ENDPOINT_SLOT, tag);
    }
    return sos_debug_print(vData, count);
}

size_t sos_read(void *vData, size_t count)
{
    //implement this to use your syscall
    return 0;
}

