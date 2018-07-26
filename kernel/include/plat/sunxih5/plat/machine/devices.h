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
#ifndef __PLAT_MACHINE_DEVICES_H
#define __PLAT_MACHINE_DEVICES_H

#include <plat_mode/machine/devices.h>

#define GIC_PL390_CONTROLLER_PPTR   GIC_CONTROLLER_PPTR
#define GIC_PL390_DISTRIBUTOR_PPTR  GIC_DISTRIBUTOR_PPTR

#define GIC_DISTRIBUTOR_PADDR       GIC_PADDR + 0x1000
#define GIC_CONTROLLER0_PADDR       GIC_PADDR + 0x2000

#define   SRAM_A1_PADDR             0x00010000
#define   SRAM_C_PADDR              0x00018000
#define   SRAM_A2_PADDR             0x00044000
#define   DE_PADDR                  0x01000000
#define   CORE_SIGHT_DEBUG_PADDR    0x01400000
#define   CPU_MBIST_PADDR           0x01502000
#define   CPU_CFG_PADDR             0x01700000
#define   SYSTEM_CONTROL_PADDR      0x01C00000
#define   DMA_PADDR                 0x01C02000
#define   NFDC_PADDR                0x01C03000
#define   TSC_PADDR                 0x01C06000
#define   KEY_MEMORY_SPACE_PADDR    0x01C0B000
#define   TCON0_PADDR               0x01C0C000
#define   TCON1_PADDR               0x01C0D000
#define   VE_PADDR                  0x01C0E000
#define   SMHC0_PADDR               0x01C0F000
#define   SMHC1_PADDR               0x01C10000
#define   SMHC2_PADDR               0x01C11000
#define   SID_PADDR                 0x01C14000
#define   CRYPTO_ENGINE_PADDR       0x01C15000
#define   MSG_BOX_PADDR             0x01C17000
#define   SPINLOCK_PADDR            0x01C18000
#define   USB_OTG_DEVICE_PADDR      0x01C19000
#define   USB_OTG_EHCI_OHCI0_PADDR  0x01C1A000
#define   USB_HCI1_PADDR            0x01C1B000
#define   USB_HCI2_PADDR            0x01C1C000
#define   USB_HCI3_PADDR            0x01C1D000
#define   SMC_PADDR                 0x01C1E000
#define   CCU_PADDR                 0x01C20000
#define   OWA_PADDR                 0x01C21000
#define   I2S_PCM0_PADDR            0x01C22000
#define   SPC_PADDR                 0x01C23000
#define   THS_PADDR                 0x01C25000
#define   UART0_PADDR               0x01C28000
#define   TWI0_PADDR                0x01C2A000
#define   TWI1_PADDR                0x01C2B000
#define   SCR_PADDR                 0x01C2C000
#define   EMAC_PADDR                0x01C30000
#define   HSTMR_PADDR               0x01C60000
#define   DRAMCOM_PADDR             0x01C62000
#define   DRAMCTL0_PADDR            0x01C63000
#define   DRAMPHY0_PADDR            0x01C65000
#define   SPI0_PADDR                0x01C68000
#define   SPI1_PADDR                0x01C69000
#define   GIC_PADDR                 0x01C80000
#define   CSI_PADDR                 0x01CB0000
#define   DE_INTERLACER_PADDR       0x01E00000
#define   TVE_PADDR                 0x01E40000
#define   GPU_PADDR                 0x01E80000
#define   HDMI_PADDR                0x01EE0000
#define   RTC_PADDR                 0x01F00000
#define   R_WDOG_PADDR              0x01F01000
#define   R_CIR_RX_PADDR            0x01F02000
#define   R_PWM_PADDR               0x01F03000

#endif
