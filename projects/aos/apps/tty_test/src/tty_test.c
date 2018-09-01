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
 *      Description: Simple milestone 0 test.
 *
 *      Author:			Godfrey van der Linden
 *      Original Author:	Ben Leslie
 *
 ****************************************************************************/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sel4/sel4.h>
#include <syscalls.h>
#include <utils/page.h>
#include <sos.h>

#include "ttyout.h"

#define NPAGES 270
#define TEST_ADDRESS 0x8000000000

/* called from pt_test */
static void
do_pt_test(int *buf)
{
    /* set */
    for (int i = 0; i < NPAGES; i++) {
	    buf[i * PAGE_SIZE_4K]  = i;
    }
    
    /* check */
    for (int i = 0; i < NPAGES; i++) {
	    assert(buf[i * PAGE_SIZE_4K] == i);
    }
}

static void
pt_test( void )
{
    /* need a decent sized stack */
    printf("testing decently sized stack....\n");
    int buf1[NPAGES * PAGE_SIZE_4K], *buf2 = NULL;
    /* check the stack is above phys mem */
    assert((void *) buf1 > (void *) TEST_ADDRESS);
    /* stack test */
    do_pt_test(buf1);
    printf("passed stack test!\n");

    printf("testing malloc works...\n");
    /* heap test */
    buf2 = malloc(NPAGES * PAGE_SIZE_4K);
    assert(buf2);
    do_pt_test(buf2);
    free(buf2);
    printf("passed malloc test!\n");
}

void test_m3(void)
{
    pt_test();
}

// Block a thread forever
// we do this by making an unimplemented system call.
static void
thread_block(void)
{
    /* construct some info about the IPC message tty_test will send
     * to sos -- it's 1 word long */
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, 1);
    /* Set the first word in the message to 0 */
    seL4_SetMR(0, 9999);
    /* Now send the ipc -- call will send the ipc, then block until a reply
     * message is received */
    seL4_Call(SYSCALL_ENDPOINT_SLOT, tag);
    /* Currently SOS does not reply -- so we never come back here */
}

int main(void)
{
    sosapi_init_syscall_table();
    
    /* initialise communication */
    ttyout_init();
    //char msg[10];
    //sos_sys_usleep(10000000);
    //test_m3();

    do {
        int fd = sos_sys_open("console", 0xF);
        printf("%d\n", fd);
        
        sos_sys_write(fd, "hello\n", 8);
        //sos_read(msg, 10);
        //sos_write(msg, 10);
        
#if 0
        printf("HELLO\tI\tAM\tTTY_TEST!\n");
        char *msg = "############### Testing tty_test!!! ##################\n";
        sos_write(msg, strlen(msg));

        msg = "The quick brown fox jumps over the lazy dog.\n";
        sos_write(msg, strlen(msg));

        msg = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Vestibulum aliquam quis nibh vitae efficitur. Nunc fringilla erat nec pharetra pharetra. Maecenas vehicula dignissim urna. Proin eu eleifend tortor, id pharetra enim. Aliquam varius velit vel velit sagittis vulputate. Phasellus augue neque, tempor eu luctus a, blandit id justo. Curabitur aliquam ex at turpis blandit, ac egestas neque pretium. Nam dui turpis, viverra venenatis molestie non, faucibus quis est. Nulla efficitur, turpis nec sollicitudin consectetur, velit diam pulvinar sapien, non hendrerit mauris ipsum nec turpis. Nulla facilisi. Nullam pharetra commodo imperdiet. Integer non tortor purus. Ut lorem orci, sodales ultrices efficitur a, viverra vitae mauris.\n";
        sos_write(msg, strlen(msg));

        msg = "############### Finished testing!!! ##################\n";
        sos_write(msg, strlen(msg));

        printf("GOODBYE\tI\tAM\tTTY_TEST!\n");
#endif
        thread_block();
        // sleep(1);	// Implement this as a syscall
    } while (1);

    return 0;
}
