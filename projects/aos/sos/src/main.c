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
#include <autoconf.h>
#include <utils/util.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <cspace/cspace.h>
#include <aos/sel4_zf_logif.h>
#include <aos/debug.h>

#include <sos.h>
#include <clock/clock.h>
#include <cpio/cpio.h>
#include <elf/elf.h>
#include <serial/serial.h>

#include "picoro/picoro.h"
#include "proc/proc.h"
#include "bootstrap.h"
#include "network.h"
#include "drivers/uart.h"
#include "ut/ut.h"
#include "vm/vmem_layout.h"
#include "mapping.h"
#include "elfload.h"
#include "syscall/syscalls.h"
#include "tests/tests.h"
#include "vm/frametable.h"
#include "vm/pagetable.h"
#include "vm/address_space.h"
#include "ringbuffer.h"
#include "shared_buf.h"
#include "vfs/vfs.h"
#include "syscall/file_syscalls.h"
#include "syscall/vm_syscalls.h"
#include "syscall/time_syscalls.h"

#include <aos/vsyscall.h>

/* To differentiate between signals from notification objects and and IPC messages,
 * we assign a badge to the notification object. The badge that we receive will
 * be the bitwise 'OR' of the notification object badge and the badges
 * of all pending IPC messages. */
#define IRQ_EP_BADGE         BIT(seL4_BadgeBits - 1)
/* All badged IRQs set high bet, then we use uniq bits to
 * distinguish interrupt sources */
#define IRQ_BADGE_NETWORK_IRQ  BIT(0)
#define IRQ_BADGE_NETWORK_TICK BIT(1)
#define IRQ_BADGE_TIMER        BIT(2)

/* The linker will link this symbol to the start address  *
 * of an archive of attached applications.                */
extern char __eh_frame_start[];
/* provided by gcc */
extern void (__register_frame)(void *);

/* root tasks cspace */
static cspace_t cspace;

static int (*syscall_table[])(void) = {
    NULL,
    syscall_write,
    syscall_read,
    syscall_open,
    syscall_close,
    syscall_brk,
    syscall_usleep,
    syscall_time_stamp,
};

//void handle_syscall(UNUSED seL4_Word badge, UNUSED int num_args)
void handle_syscall(void)
{

    /* allocate a slot for the reply cap */
    seL4_CPtr reply = cspace_alloc_slot(&cspace);
    /* get the first word of the message, which in the SOS protocol is the number
     * of the SOS "syscall". */
    seL4_Word syscall_number = seL4_GetMR(0);
    /* Save the reply capability of the caller. If we didn't do this,
     * we coud just use seL4_Reply to respond directly to the reply capability.
     * However if SOS were to block (seL4_Recv) to receive another message, then
     * the existing reply capability would be deleted. So we save the reply capability
     * here, as in future you will want to reply to it later. Note that after
     * saving the reply capability, seL4_Reply cannot be used, as the reply capability
     * is moved from the internal slot in the TCB to our cspace, and the internal
     * slot is now empty. */
    seL4_Error err = cspace_save_reply_cap(&cspace, reply);
    ZF_LOGF_IFERR(err, "Failed to save reply");

    if (syscall_number > 0 && syscall_number < 8) {
        int nwords = syscall_table[syscall_number]();
        seL4_MessageInfo_t reply_msg = seL4_MessageInfo_new(0, 0, 0, nwords);
        seL4_Send(reply, reply_msg);
    }
    else {
        ZF_LOGE("Unknown syscall %lu\n", syscall_number);
    }

    cspace_free_slot(&cspace, reply);
#if 0
    /* Process system call */
    switch (syscall_number) {
    case SOS_SYS_WRITE:
        ZF_LOGV("syscall: thread called sys_write (1)\n");
        // get data from message registers
        size_t nbyte = seL4_GetMR(2);
        int fd = seL4_GetMR(1);
        printf("%d, %d, %s", nbyte, fd, shared_buf);
        if(fd < 3 && fd > 0){
            fd = 3;
        }
        struct vnode * vn = curproc->fdt->openfiles[fd]; 
        struct uio * u = malloc(sizeof(struct uio));
        uio_init(u, WRITE, nbyte);
        size_t bytes_written = VOP_WRITE(vn, u);
        free(u);
        seL4_SetMR(0, bytes_written);
        /* send back the number of bytes transmitted */
        reply_msg = seL4_MessageInfo_new(0, 0, 0, 1);
        seL4_Send(reply, reply_msg);
        cspace_free_slot(&cspace, reply);
        break;
    case SOS_SYS_READ:
        ZF_LOGV("syscall: thread called sys_read (2)\n");
        // get data from message registers
        nbyte = seL4_GetMR(2);
        vn = curproc->fdt->openfiles[seL4_GetMR(1)]; 
        assert(vn);
        u = malloc(sizeof(struct uio));
        uio_init(u, READ, nbyte);
        size_t bytes_read = VOP_READ(vn, u);
        free(u);
        seL4_SetMR(0, bytes_read);
        /* send back the number of bytes transmitted */
        reply_msg = seL4_MessageInfo_new(0, 0, 0, 1);
        seL4_Send(reply, reply_msg);
        cspace_free_slot(&cspace, reply);
        break;
    case SOS_SYS_OPEN:
        ZF_LOGV("syscall: thread called sys_open (3)\n");
        fmode_t mode = seL4_GetMR(1);
        struct vnode *res;
        vfs_lookup(seL4_GetIPCBuffer()->msg + 2, &res); 
        VOP_EACHOPEN(res, 0);
        bool full = true;
        for(unsigned int i = 3; i < 8; i ++){
            if(curproc->fdt->openfiles[i] == NULL){
                curproc->fdt->openfiles[i] = res;
                seL4_SetMR(0, 0);
                seL4_SetMR(1, i);
                full = false;
                printf("i: %d\n", i);
                break;
            }
        } 
        if(full){
            seL4_SetMR(0, 1);
        }
        reply_msg = seL4_MessageInfo_new(0, 0, 0, 2);
        printf("i : %d\n", seL4_GetMR(1));
        seL4_Send(reply, reply_msg);
        cspace_free_slot(&cspace, reply);
        break;
    case SOS_SYS_CLOSE:
        ZF_LOGV("syscall: thread called sys_close (4)\n");
        break;
    case SOS_SYS_BRK:
        ZF_LOGV("syscall: thread called sys_brk (5)\n");

        seL4_Word newbrk = seL4_GetMR(1);

        seL4_Word hbase = curproc->as->heap->vbase;
        if (newbrk >= PROCESS_HEAP_BASE) {
            hbase = newbrk;
        }
        reply_msg = seL4_MessageInfo_new(0, 0, 0, 2);
        seL4_SetMR(0, 0);
        seL4_SetMR(1, hbase);
        seL4_Send(reply, reply_msg);

        cspace_free_slot(&cspace, reply);
        break;
    case SOS_SYS_USLEEP:
        ZF_LOGV("syscall: thread called sys_usleep (6)\n");
        
        int msec = seL4_GetMR(1);

        seL4_CPtr* reply_cap = malloc(sizeof(seL4_CPtr));
        *reply_cap = reply;

        int timer = register_timer(msec, ONE_SHOT, &usleep_handler, reply_cap);
        printf("timer %ld, %d, %ld", timer, msec, *reply_cap);
        if(!timer){
            ZF_LOGE("Timer failed to initialize %lu\n", syscall_number);
            reply_msg = seL4_MessageInfo_new(0, 0, 0, 1);
            seL4_SetMR(0, -1); //error
            seL4_Send(reply, reply_msg);
            cspace_free_slot(&cspace, reply);
        }
        
        break;
    case SOS_SYS_TIME_STAMP:
        ZF_LOGV("syscall: thread called sys_time_stamp (7)\n");

        int64_t time = get_time();

        reply_msg = seL4_MessageInfo_new(0, 0, 0, 1);
        seL4_SetMR(0, time);
        seL4_Send(reply, reply_msg);

        cspace_free_slot(&cspace, reply);
        break;
    default:
        ZF_LOGE("Unknown syscall %lu\n", syscall_number);
        cspace_free_slot(&cspace, reply);
        /* don't reply to an unknown syscall */
    }
#endif
}

NORETURN void syscall_loop(seL4_CPtr ep)
{
    while (1) {
        seL4_Word badge = 0;
        /* Block on ep, waiting for an IPC sent over ep, or
         * a notification from our bound notification object */
        seL4_MessageInfo_t message = seL4_Recv(ep, &badge);
        /* Awake! We got a message - check the label and badge to
         * see what the message is about */
        seL4_Word label = seL4_MessageInfo_get_label(message);

        if (badge & IRQ_EP_BADGE) {
            /* It's a notification from our bound notification
             * object! */
            if (badge & IRQ_BADGE_NETWORK_IRQ) {
                /* It's an interrupt from the ethernet MAC */
                network_irq();
            }
            if (badge & IRQ_BADGE_NETWORK_TICK) {
                /* It's an interrupt from the watchdog keeping our TCP/IP stack alive */
                network_tick();
            }
            if (badge & IRQ_BADGE_TIMER) {
                /* It's an interrupt from one of the timers */
                timer_interrupt();
            }
        } else if (label == seL4_Fault_VMFault) {
            seL4_Word faultaddress = seL4_GetMR(1);
            //printf("%lx, %lx, %lx\n", seL4_GetMR(1), seL4_GetMR(2), seL4_GetMR(3));
            vm_fault(&cspace, faultaddress);

            seL4_Word err = seL4_GetMR(0);
            if (err) {
                debug_print_fault(message, TTY_NAME);
                debug_dump_registers(curproc->tcb);

                ZF_LOGF("vm fault failed!");
            }
        } else if (label == seL4_Fault_NullFault) {
            /* It's not a fault or an interrupt, it must be an IPC
             * message from tty_test! */
            //handle_syscall(badge, seL4_MessageInfo_get_length(message) - 1);
            resume(coroutine((void *(*)(void *))handle_syscall), NULL);

        } else {
            /* some kind of fault */
            debug_print_fault(message, TTY_NAME);
            /* dump registers too */
            debug_dump_registers(curproc->tcb);

            ZF_LOGF("The SOS skeleton does not know how to handle faults!");
        }
    }
}

/* Allocate an endpoint and a notification object for sos.
 * Note that these objects will never be freed, so we do not
 * track the allocated ut objects anywhere
 */
static void sos_ipc_init(seL4_CPtr* ipc_ep, seL4_CPtr* ntfn)
{
    /* Create an notification object for interrupts */
    ut_t *ut = alloc_retype(&cspace, ntfn, seL4_NotificationObject, seL4_NotificationBits);
    ZF_LOGF_IF(!ut, "No memory for notification object");

    /* Bind the notification object to our TCB */
    seL4_Error err = seL4_TCB_BindNotification(seL4_CapInitThreadTCB, *ntfn);
    ZF_LOGF_IFERR(err, "Failed to bind notification object to TCB");

    /* Create an endpoint for user application IPC */
    ut = alloc_retype(&cspace, ipc_ep, seL4_EndpointObject, seL4_EndpointBits);
    ZF_LOGF_IF(!ut, "No memory for endpoint");
}

static inline seL4_CPtr badge_irq_ntfn(seL4_CPtr ntfn, seL4_Word badge)
{
    /* allocate a slot */
    seL4_CPtr badged_cap = cspace_alloc_slot(&cspace);
    ZF_LOGF_IF(badged_cap == seL4_CapNull, "Failed to allocate slot");

    /* mint the cap, which sets the badge */
    seL4_Error err = cspace_mint(&cspace, badged_cap, &cspace, ntfn, seL4_AllRights, badge | IRQ_EP_BADGE);
    ZF_LOGE_IFERR(err, "Failed to mint cap");

    /* return the badged cptr */
    return badged_cap;
}

/* called by crt */
seL4_CPtr get_seL4_CapInitThreadTCB(void)
{
    return seL4_CapInitThreadTCB;
}

/* tell muslc about our "syscalls", which will bve called by muslc on invocations to the c library */
void init_muslc(void)
{
    muslcsys_install_syscall(__NR_set_tid_address, sys_set_tid_address);
    muslcsys_install_syscall(__NR_writev, sys_writev);
    muslcsys_install_syscall(__NR_exit, sys_exit);
    muslcsys_install_syscall(__NR_rt_sigprocmask, sys_rt_sigprocmask);
    muslcsys_install_syscall(__NR_gettid, sys_gettid);
    muslcsys_install_syscall(__NR_getpid, sys_getpid);
    muslcsys_install_syscall(__NR_tgkill, sys_tgkill);
    muslcsys_install_syscall(__NR_tkill, sys_tkill);
    muslcsys_install_syscall(__NR_exit_group, sys_exit_group);
    muslcsys_install_syscall(__NR_ioctl, sys_ioctl);
    muslcsys_install_syscall(__NR_mmap, sys_mmap);
    muslcsys_install_syscall(__NR_brk,  sys_brk);
    muslcsys_install_syscall(__NR_clock_gettime, sys_clock_gettime);
    muslcsys_install_syscall(__NR_nanosleep, sys_nanosleep);
    muslcsys_install_syscall(__NR_getuid, sys_getuid);
    muslcsys_install_syscall(__NR_getgid, sys_getgid);
    muslcsys_install_syscall(__NR_openat, sys_openat);
    muslcsys_install_syscall(__NR_close, sys_close);
    muslcsys_install_syscall(__NR_socket, sys_socket);
    muslcsys_install_syscall(__NR_bind, sys_bind);
    muslcsys_install_syscall(__NR_listen, sys_listen);
    muslcsys_install_syscall(__NR_connect, sys_connect);
    muslcsys_install_syscall(__NR_accept, sys_accept);
    muslcsys_install_syscall(__NR_sendto, sys_sendto);
    muslcsys_install_syscall(__NR_recvfrom, sys_recvfrom);
    muslcsys_install_syscall(__NR_readv, sys_readv);
    muslcsys_install_syscall(__NR_getsockname, sys_getsockname);
    muslcsys_install_syscall(__NR_getpeername, sys_getpeername);
    muslcsys_install_syscall(__NR_fcntl, sys_fcntl);
    muslcsys_install_syscall(__NR_setsockopt, sys_setsockopt);
    muslcsys_install_syscall(__NR_getsockopt, sys_getsockopt);
    muslcsys_install_syscall(__NR_ppoll, sys_ppoll);
    muslcsys_install_syscall(__NR_madvise, sys_madvise);
}


NORETURN void *main_continued(UNUSED void *arg)
{
    /* Initialise other system compenents here */
    seL4_CPtr ipc_ep, ntfn;
    sos_ipc_init(&ipc_ep, &ntfn);

    /* run sos initialisation tests */
    run_tests(&cspace);

    /* Map the timer device (NOTE: this is the same mapping you will use for your timer driver -
     * sos uses the watchdog timers on this page to implement reset infrastructure & network ticks,
     * so touching the watchdog timers here is not recommended!) */
    void *timer_vaddr = sos_map_device(&cspace, PAGE_ALIGN_4K(TIMER_PADDR), PAGE_SIZE_4K);

    /* Initialise the network hardware. */
    printf("Network init\n");
    network_init(&cspace,
                 badge_irq_ntfn(ntfn, IRQ_BADGE_NETWORK_IRQ),
                 badge_irq_ntfn(ntfn, IRQ_BADGE_NETWORK_TICK),
                 timer_vaddr);

    /*printf("Serial init\n");
    serial_port = serial_init();
    serial_register_handler(serial_port, handler);
    stream_buf sb;
    bufferInit(sb, 1024, char);
    sb_ptr = &sb;
    */

    printf("Starting timers\n");
    start_timer(&cspace, badge_irq_ntfn(ntfn, IRQ_BADGE_TIMER), timer_vaddr);

    frame_table_init(&cspace);
    shared_buf_init(&cspace);
    vfs_bootstrap();
    //test_m1();
    //test_m2();

    /* Start the user application */
    printf("Start first process\n");
    bool success = start_first_process(&cspace, TTY_NAME, ipc_ep);
    ZF_LOGF_IF(!success, "Failed to start first process");
    printf("\nSOS entering syscall loop\n");
    syscall_loop(ipc_ep);
}
/*
 * Main entry point - called by crt.
 */
int main(void)
{
    init_muslc();

    /* bootinfo was set as an environment variable in _sel4_start */
    char *bi_string = getenv("bootinfo");
    ZF_LOGF_IF(!bi_string, "Could not parse bootinfo from env.");

    /* register the location of the unwind_tables -- this is required for
     * backtrace() to work */
    __register_frame(&__eh_frame_start);

    seL4_BootInfo *boot_info;
    if (sscanf(bi_string, "%p", &boot_info) != 1) {
        ZF_LOGF("bootinfo environment value '%s' was not valid.", bi_string);
    }

    debug_print_bootinfo(boot_info);

    printf("\nSOS Starting...\n");

    NAME_THREAD(seL4_CapInitThreadTCB, "SOS:root");

    /* Initialise the cspace manager, ut manager and dma */
    sos_bootstrap(&cspace, boot_info);

    /* switch to the real uart to output (rather than seL4_DebugPutChar, which only works if the
     * kernel is built with support for printing, and is much slower, as each character print
     * goes via the kernel)
     *
     * NOTE we share this uart with the kernel when the kernel is in debug mode. */
    uart_init(&cspace);
    update_vputchar(uart_putchar);

    /* test print */
    printf("SOS Started!\n");

    /* allocate a bigger stack and switch to it -- we'll also have a guard page, which makes it much
     * easier to detect stack overruns */
    seL4_Word vaddr = SOS_STACK;
    for (int i = 0; i < SOS_STACK_PAGES; i++) {
        seL4_CPtr frame_cap;
        ut_t *frame = alloc_retype(&cspace, &frame_cap, seL4_ARM_SmallPageObject, seL4_PageBits);
        ZF_LOGF_IF(frame == NULL, "Failed to allocate stack page");
        seL4_Error err = map_frame(&cspace, frame_cap, seL4_CapInitThreadVSpace,
                                   vaddr, seL4_AllRights, seL4_ARM_Default_VMAttributes);
        ZF_LOGF_IFERR(err, "Failed to map stack");
        vaddr += PAGE_SIZE_4K;
    }

    utils_run_on_stack((void *) vaddr, main_continued, NULL);

    UNREACHABLE();
}


