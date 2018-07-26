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

/* The uart is not at the start of the frame - find the offset */
#define UART0_AO_OFFSET (UART0_AO_PADDR - UART_AO_PADDR)
#define UART_WFIFO  0x0
#define UART_RFIFO  0x4
#define UART_CTRL   0x8
#define UART_STATUS 0xC
#define UART_MISC   0x10

#define UART_TX_FULL        BIT(21)
#define UART_RX_EMPTY       BIT(20)

#define UART_RX_IRQ BIT(27)
#define UART_RX_EN  BIT(13)
#define UART_TX_EN  BIT(12)

#define UART_REG(x) ((volatile uint32_t *)(UART_AO_PPTR + UART0_AO_OFFSET + (x)))

#define WDOG_OFFSET 0x8D0
#define WDOG_EN BIT(18)
#define WDOG_SYS_RESET_EN BIT(21)
#define WDOG_CLK_EN BIT(24)
#define WDOG_CLK_DIV_EN BIT(25)
#define WDOG_SYS_RESET_NOW BIT(26)

void init_serial(void)
{
    /* enable tx, rx and rx irq */
    *(UART_REG(UART_CTRL)) |= UART_TX_EN | UART_RX_EN | UART_RX_IRQ;
    /* send irq when 1 char is available */
    *(UART_REG(UART_MISC)) = 1;
}

static char reset[] = "reset";
static int index = 0;

void handleUartIRQ(void)
{
    /* while there are chars to process */
    while (!(*UART_REG(UART_STATUS) & UART_RX_EMPTY)) {
        char c = getDebugChar();
        putDebugChar(c);
        if (reset[index] == c) {
            index++;
        } else {
            index = 0;
        }
        if (index == strnlen(reset, 10)) {
            /* do the reset */
            volatile uint32_t *wdog = (volatile uint32_t *) (WDOG_PPTR + WDOG_OFFSET);
            *wdog = (WDOG_EN | WDOG_SYS_RESET_EN | WDOG_CLK_EN |
                     WDOG_CLK_DIV_EN | WDOG_SYS_RESET_NOW);
        }
    }
}

#if defined(CONFIG_DEBUG_BUILD) || defined(CONFIG_PRINTING)
void
putDebugChar(unsigned char c)
{
    while ((*UART_REG(UART_STATUS) & UART_TX_FULL));

    /* Add character to the buffer. */
    *UART_REG(UART_WFIFO) = c;
}
#endif

#ifdef CONFIG_DEBUG_BUILD
unsigned char
getDebugChar(void)
{
    return *(UART_REG(UART_RFIFO));
}
#endif
