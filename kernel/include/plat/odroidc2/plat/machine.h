/*
 * Copyright 2016, General Dynamics C4 Systems
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(GD_GPL)
 */

#ifndef __PLAT_MACHINE_H
#define __PLAT_MACHINE_H

#include <plat_mode/machine.h>

#define N_INTERRUPTS             251

enum IRQConstants {
    maxIRQ = 250
} platform_interrupt_t;

#define KERNEL_UART_IRQ         225
#define KERNEL_TIMER_IRQ        27
#define KERNEL_PMU_IRQ          168

#include <arch/machine/gic_pl390.h>

#endif  /* ! __PLAT_MACHINE_H */
