/*
 * Copyright 2016, General Dynamics C4 Systems
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(GD_GPL)
 */

#include <config.h>
#include <stdint.h>
#include <util.h>
#include <machine/io.h>
#include <plat/machine/devices.h>

#define UART_THR   0x00
#define UART_LSR   0x14
#define UART_LSR_THRE BIT(5)

#define UART_REG(x) ((volatile uint32_t *)(UART0_PPTR + (x)))

#if defined(CONFIG_DEBUG_BUILD) || defined(CONFIG_PRINTING)
void
putDebugChar(unsigned char c)
{
    /* Wait until the transmit holding register is ready */
    while ((*UART_REG(UART_LSR) & UART_LSR_THRE) == 0);

    /* Add character to the buffer. */
    *UART_REG(UART_THR) = c;
}
#endif

#ifdef CONFIG_DEBUG_BUILD
unsigned char
getDebugChar(void)
{
    return 0;
}
#endif
