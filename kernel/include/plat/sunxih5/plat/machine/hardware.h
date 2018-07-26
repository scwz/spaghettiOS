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

#ifndef __PLAT_MACHINE_HARDWARE_H
#define __PLAT_MACHINE_HARDWARE_H

#include <config.h>
#include <basic_types.h>
#include <linker.h>
#include <plat/machine.h>
#include <plat/machine/devices.h>
#include <plat_mode/machine/hardware.h>

#define physBase            0x40000000

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
        UART0_PADDR,
        UART0_PPTR,
        true  /* armExecuteNever */
#endif
    }
};

/* Available physical memory regions on platform (RAM minus kernel image). */
/* NOTE: Regions are not allowed to be adjacent! */

static const p_region_t BOOT_RODATA avail_p_regs[] = {
    { .start = physBase, .end = 0x80000000 }
};

static const p_region_t BOOT_RODATA dev_p_regs[] = {
    { SRAM_A1_PADDR            , SRAM_A1_PADDR            + BIT(PAGE_BITS)}         ,
    { SRAM_C_PADDR             , SRAM_C_PADDR             + BIT(PAGE_BITS)}         ,
    { SRAM_A2_PADDR            , SRAM_A2_PADDR            + BIT(PAGE_BITS)}         ,
    { DE_PADDR                 , DE_PADDR                 + BIT(PAGE_BITS) * 1000u} ,
    { CORE_SIGHT_DEBUG_PADDR   , CORE_SIGHT_DEBUG_PADDR   + BIT(PAGE_BITS) * 32u}   ,
    { CPU_MBIST_PADDR          , CPU_MBIST_PADDR          + BIT(PAGE_BITS)}         ,
    { CPU_CFG_PADDR            , CPU_CFG_PADDR            + BIT(PAGE_BITS)}         ,
    { SYSTEM_CONTROL_PADDR     , SYSTEM_CONTROL_PADDR     + BIT(PAGE_BITS)}         ,
    { DMA_PADDR                , DMA_PADDR                + BIT(PAGE_BITS)}         ,
    { NFDC_PADDR               , NFDC_PADDR               + BIT(PAGE_BITS)}         ,
    { TSC_PADDR                , TSC_PADDR                + BIT(PAGE_BITS)}         ,
    { KEY_MEMORY_SPACE_PADDR   , KEY_MEMORY_SPACE_PADDR   + BIT(PAGE_BITS)}         ,
    { TCON0_PADDR              , TCON0_PADDR              + BIT(PAGE_BITS)}         ,
    { TCON1_PADDR              , TCON1_PADDR              + BIT(PAGE_BITS)}         ,
    { VE_PADDR                 , VE_PADDR                 + BIT(PAGE_BITS)}         ,
    { SMHC0_PADDR              , SMHC0_PADDR              + BIT(PAGE_BITS)}         ,
    { SMHC1_PADDR              , SMHC1_PADDR              + BIT(PAGE_BITS)}         ,
    { SMHC2_PADDR              , SMHC2_PADDR              + BIT(PAGE_BITS)}         ,
    { SID_PADDR                , SID_PADDR                + BIT(PAGE_BITS)}         ,
    { CRYPTO_ENGINE_PADDR      , CRYPTO_ENGINE_PADDR      + BIT(PAGE_BITS)}         ,
    { MSG_BOX_PADDR            , MSG_BOX_PADDR            + BIT(PAGE_BITS)}         ,
    { SPINLOCK_PADDR           , SPINLOCK_PADDR           + BIT(PAGE_BITS)}         ,
    { USB_OTG_DEVICE_PADDR     , USB_OTG_DEVICE_PADDR     + BIT(PAGE_BITS)}         ,
    { USB_OTG_EHCI_OHCI0_PADDR , USB_OTG_EHCI_OHCI0_PADDR + BIT(PAGE_BITS)}         ,
    { USB_HCI1_PADDR           , USB_HCI1_PADDR           + BIT(PAGE_BITS)}         ,
    { USB_HCI2_PADDR           , USB_HCI2_PADDR           + BIT(PAGE_BITS)}         ,
    { USB_HCI3_PADDR           , USB_HCI3_PADDR           + BIT(PAGE_BITS)}         ,
    { SMC_PADDR                , SMC_PADDR                + BIT(PAGE_BITS)}         ,
    { CCU_PADDR                , CCU_PADDR                + BIT(PAGE_BITS)}         ,
    { OWA_PADDR                , OWA_PADDR                + BIT(PAGE_BITS)}         ,
    { I2S_PCM0_PADDR           , I2S_PCM0_PADDR           + BIT(PAGE_BITS)}         ,
    { SPC_PADDR                , SPC_PADDR                + BIT(PAGE_BITS)}         ,
    { THS_PADDR                , THS_PADDR                + BIT(PAGE_BITS)}         ,
    { UART0_PADDR              , UART0_PADDR              + BIT(PAGE_BITS)}         ,
    { TWI0_PADDR               , TWI0_PADDR               + BIT(PAGE_BITS)}         ,
    { TWI1_PADDR               , TWI1_PADDR               + BIT(PAGE_BITS)}         ,
    { SCR_PADDR                , SCR_PADDR                + BIT(PAGE_BITS)}         ,
    { EMAC_PADDR               , EMAC_PADDR               + BIT(PAGE_BITS) * 16u}   ,
    { HSTMR_PADDR              , HSTMR_PADDR              + BIT(PAGE_BITS)}         ,
    { DRAMCOM_PADDR            , DRAMCOM_PADDR            + BIT(PAGE_BITS)}         ,
    { DRAMCTL0_PADDR           , DRAMCTL0_PADDR           + BIT(PAGE_BITS)}         ,
    { DRAMPHY0_PADDR           , DRAMPHY0_PADDR           + BIT(PAGE_BITS)}         ,
    { SPI0_PADDR               , SPI0_PADDR               + BIT(PAGE_BITS)}         ,
    { SPI1_PADDR               , SPI1_PADDR               + BIT(PAGE_BITS)}         ,
    { GIC_PADDR                , GIC_PADDR                + BIT(PAGE_BITS)}         ,
    { CSI_PADDR                , CSI_PADDR                + BIT(PAGE_BITS) * 80u}   ,
    { DE_INTERLACER_PADDR      , DE_INTERLACER_PADDR      + BIT(PAGE_BITS) * 32u}   ,
    { TVE_PADDR                , TVE_PADDR                + BIT(PAGE_BITS) * 16u}   ,
    { GPU_PADDR                , GPU_PADDR                + BIT(PAGE_BITS) * 48u}   ,
    { HDMI_PADDR               , HDMI_PADDR               + BIT(PAGE_BITS) * 32u}   ,
    { RTC_PADDR                , RTC_PADDR                + BIT(PAGE_BITS)}         ,
    { R_WDOG_PADDR             , R_WDOG_PADDR             + BIT(PAGE_BITS)}         ,
    { R_CIR_RX_PADDR           , R_CIR_RX_PADDR           + BIT(PAGE_BITS)}         ,
    { R_PWM_PADDR              , R_PWM_PADDR              + BIT(PAGE_BITS)}         ,
};

/* Handle a platform-reserved IRQ. */
static inline void
handleReservedIRQ(irq_t irq)
{
}

#endif
