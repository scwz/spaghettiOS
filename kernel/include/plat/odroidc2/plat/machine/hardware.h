/*
 * Copyright 2016, General Dynamics C4 Systems
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(GD_GPL)
 */

#ifndef __PLAT_MACHINE_HARDWARE_H
#define __PLAT_MACHINE_HARDWARE_H

#include <config.h>
#include <basic_types.h>
#include <linker.h>
#include <plat/machine.h>
#include <plat/machine/devices.h>
#include <plat_mode/machine/hardware.h>

#define physBase            0x01000000

static const kernel_frame_t BOOT_RODATA kernel_devices[] = {
    {
        /*  GIC */
        GIC_CONTROLLER0_PADDR,
        GIC_CONTROLLER_PPTR,
        true  /* armExecuteNever */
    },
    {
        GIC_DISTRIBUTOR_PADDR,
        GIC_DISTRIBUTOR_PPTR,
        true  /* armExecuteNever */
#ifdef CONFIG_PRINTING
    },
    {
        /*  UART */
        UART_AO_PADDR,
        UART_AO_PPTR,
        true  /* armExecuteNever */
#endif
    },
    {
        WDOG_PADDR,
        WDOG_PPTR,
        true
    }
};

/* Available physical memory regions on platform (RAM minus kernel image). */
/* NOTE: Regions are not allowed to be adjacent! */

static const p_region_t BOOT_RODATA avail_p_regs[] = {
    { .start = physBase, .end = 0x80000000 }
};

static const p_region_t BOOT_RODATA dev_p_regs[] = {
    { USB_PADDR, USB_PADDR_END },
    { DAP_PADDR, DAP_PADDR_END },
    { CSSYS_PADDR, CSSYS_PADDR_END },
    { RESET_PADDR, RESET_PADDR + BIT(PAGE_BITS) },
    { AIU_PADDR, AIU_PADDR + BIT(PAGE_BITS) },
    { ASSIST_PADDR, ASSIST_PADDR + BIT(PAGE_BITS) },
    { PERIPHS_PADDR, PERIPHS_PADDR + BIT(PAGE_BITS) },
    { ISA_PADDR, ISA_PADDR + BIT(PAGE_BITS) },
    { AUDIN_PADDR, AUDIN_PADDR + BIT(PAGE_BITS) },
    { UART_PADDR, UART_PADDR + BIT(PAGE_BITS) },
    { TIMER_PADDR, TIMER_PADDR + BIT(PAGE_BITS) },
    { GIC_PADDR, GIC_PADDR_END },
    { RTI_PADDR, RTI_PADDR_END },
    { DOS_PADDR, DOS_PADDR_END },
    { BLKMV_PADDR, BLKMV_PADDR_END},
    { PERIPHS2_PADDR, PERIPHS2_PADDR_END },
    { DDR_TOP_PADDR, DDR_TOP_PADDR_END },
    { DMC_PADDR, DMC_PADDR_END },
    { HDMITX_PADDR, HDMITX_PADDR_END },
    { HIU_PADDR, HIU_PADDR_END },
    { USB1_PADDR, USB1_PADDR_END },
    { ETH_PADDR, ETH_PADDR_END },
    { SPI_PADDR, SPI_PADDR_END },
    { PDM_PADDR, PDM_PADDR_END },
    { HDCP22_PADDR, HDCP22_PADDR_END },
    { BT656_PADDR, BT646_PADDR_END },
    { SD_EMMC_PADDR, SD_EMMC_PADDR_END },
    { MALI_APB_PADDR, MALI_APB_PADDR_END },
    { VPU_PADDR, VPU_PADDR_END },
    { GE2D_PADDR, GE2D_PADDR_END},

};

/* Handle a platform-reserved IRQ. */
static inline void
handleReservedIRQ(irq_t irq)
{
    if (irq == KERNEL_UART_IRQ) {
        handleUartIRQ();
    } else {
        printf("Unknown reserved irq %d\n", irq);
    }
}

#endif
