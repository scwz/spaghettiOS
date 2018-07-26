/*
 * Copyright 2016, General Dynamics C4 Systems
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(GD_GPL)
 */

/*
 definitions for memory addresses of HiSilicon hi6220 platform
 listed in the order of ascending memory address, similar to the datasheet
 the size must all be powers of 2 and page aligned (4k or 1M).
*/

#ifndef __PLAT_MACHINE_DEVICES_H
#define __PLAT_MACHINE_DEVICES_H

#include <plat_mode/machine/devices.h>

#define GIC_PL390_CONTROLLER_PPTR   GIC_CONTROLLER_PPTR
#define GIC_PL390_DISTRIBUTOR_PPTR  GIC_DISTRIBUTOR_PPTR

#define GIC_DISTRIBUTOR_PADDR       GIC_PADDR + 0x1000
#define GIC_CONTROLLER0_PADDR       GIC_PADDR + 0x2000

#define UART0_PADDR 0xc11084c0
#define UART1_PADDR 0xc11084dc
#define UART2_PADDR 0xc1108700

#define UART_AO_PADDR  0xc8100000
#define UART0_AO_PADDR 0xc81004c0
#define UART2_AO_PADDR 0xc81004e0

#define WDOG_PADDR     0xc1109000

#define USB_PADDR           0xC0000000
#define USB_PADDR_END       0xC0200000
#define DAP_PADDR           0xC0200000
#define DAP_PADDR_END       0xC0400000
#define CSSYS_PADDR         0xC0400000
#define CSSYS_PADDR_END     0xC0800000
#define RESET_PADDR         0xC0804000
#define AIU_PADDR           0xC0805000
#define ASSIST_PADDR        0xC0807000
#define PERIPHS_PADDR       0xC0808000
#define ISA_PADDR           0xC0809000
#define AUDIN_PADDR         0xC080A000
#define UART_PADDR          0xC1108000
#define TIMER_PADDR         0xC1109000
#define GIC_PADDR           0xC4300000
#define GIC_PADDR_END       0xC4308000
#define RTI_PADDR           0xC8100000
#define RTI_PADDR_END       0xC8200000
#define DOS_PADDR           0xC8820000
#define DOS_PADDR_END       0xC8830000
#define BLKMV_PADDR         0xC8832000
#define BLKMV_PADDR_END     0xC8834000
#define PERIPHS2_PADDR      0xC8834000
#define PERIPHS2_PADDR_END  0xC8836000
#define DDR_TOP_PADDR       0xC8836000
#define DDR_TOP_PADDR_END   0xC8838000
#define DMC_PADDR           0xC8838000
#define DMC_PADDR_END       0xC883A000
#define HDMITX_PADDR        0xC883A000
#define HDMITX_PADDR_END    0xC883C000
#define HIU_PADDR           0xC883C000
#define HIU_PADDR_END       0xC883E000
#define USB1_PADDR          0xC9000000
#define USB1_PADDR_END      0xC9200000
#define ETH_PADDR           0xC9410000
#define ETH_PADDR_END       0xC9420000
#define SPI_PADDR           0xCC000000
#define SPI_PADDR_END       0xD0000000
#define PDM_PADDR           0xD0042000
#define PDM_PADDR_END       0xD0044000
#define HDCP22_PADDR        0xD0044000
#define HDCP22_PADDR_END    0xD0045000
#define BT656_PADDR         0xD0048000
#define BT646_PADDR_END     0xD0060000
#define SD_EMMC_PADDR       0xD0070000
#define SD_EMMC_PADDR_END   0xD0076000
#define MALI_APB_PADDR      0xD00C0000
#define MALI_APB_PADDR_END  0xD0100000
#define VPU_PADDR           0xD0100000
#define VPU_PADDR_END       0xD0140000
#define GE2D_PADDR          0xD0160000
#define GE2D_PADDR_END      0xD0170000

// TODO UART

#endif
