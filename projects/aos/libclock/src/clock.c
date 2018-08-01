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
#include <clock/clock.h>
#include <utils/io.h>
#include <pqueue.h>

static volatile struct {
    uint32_t timer_mux;
    uint32_t timer_f;
    uint32_t timer_g;
    uint32_t timer_h;
    uint32_t timer_i;
} *timer;

static seL4_CPtr timer_irq_handler;
static int timer_initialised = 0;
static timer_callback_t cb;
static void *cb_data;
uint64_t start = 0;
uint64_t end = 0;

int start_timer(seL4_CPtr ntfn, seL4_CPtr irqhandler, void *device_vaddr)
{
    if (timer_initialised) {
        stop_timer();
    }

    // initalise timer
    timer = device_vaddr + (TIMER_MUX & MASK((size_t) seL4_PageBits));
    timer_irq_handler = irqhandler;
    timer_initialised = 1;

    // setup registers
    timer->timer_mux |= TIMER_F_EN | TIMER_F_INPUT_CLK | TIMEBASE_1000_US; 
    timer->timer_f |= 1;

    return CLOCK_R_OK;
}

uint32_t register_timer(uint64_t delay, timer_callback_t callback, void *data)
{
    timer->timer_f |= delay;
    start = timestamp_ms(timestamp_get_freq());
    cb = callback;
    cb_data = data;
    return 0;
}

int remove_timer(uint32_t id)
{
    return CLOCK_R_FAIL;
}

int timer_interrupt(void)
{
    if (TIMER_VAL(timer->timer_f) == 0) {
        cb(1, cb_data);
        end = timestamp_ms(timestamp_get_freq());
        printf("TIME %lu\n", end - start);
    }
    return CLOCK_R_OK;
}

int stop_timer(void)
{
    return CLOCK_R_FAIL;
}
