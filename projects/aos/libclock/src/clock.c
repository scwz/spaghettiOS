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

#define TICK_10000_US 10000

static volatile struct {
    uint32_t timer_mux;
    uint32_t timer_f;
    uint32_t timer_g;
    uint32_t timer_h;
    uint32_t timer_i;
} *timer;

static seL4_CPtr timer_irq_handler;
static int timer_initialised = 0;
static struct pqueue *pq = NULL;
static uint64_t last_tick = 0;

int start_timer(seL4_CPtr ntfn, seL4_CPtr irqhandler, void *device_vaddr)
{
    if (timer_initialised) {
        stop_timer();
    }

    pq = pqueue_init();

    // initalise timer
    timer = device_vaddr + (TIMER_MUX & MASK((size_t) seL4_PageBits));
    timer_irq_handler = irqhandler;
    timer_initialised = 1;

    // setup registers
    timer->timer_mux |= TIMER_F_EN | TIMER_F_INPUT_CLK | TIMEBASE_1_US | TIMER_F_MODE; 
    timer->timer_f |= TICK_10000_US;

    last_tick = timestamp_ms(timestamp_get_freq());
    printf("TIMER STARTED: %lu ms\n", last_tick);

    return CLOCK_R_OK;
}

uint32_t register_timer(uint32_t id, uint64_t delay, job_type_t type, timer_callback_t callback, void *data)
{
    return pqueue_push(pq, id, pq->time + delay, type, callback, data);
}

int remove_timer(uint32_t id)
{
    if (!timer_initialised) {
        return CLOCK_R_UINT;
    }
    return pqueue_remove(pq, id);
}

int timer_interrupt(void)
{
    if (!timer_initialised) {
        return CLOCK_R_UINT;
    }

    struct job *job = pqueue_peek(pq);
    uint64_t curr_tick = timestamp_ms(timestamp_get_freq());
    while (job != NULL && (((job->delay > pq->time) && (job->delay <= pq->time + TICK_10000_US)))) {
        printf("CALLBACK RECEIVED: %lu ms diff: %lu ms\n", curr_tick, curr_tick - last_tick);
        job->callback(job->id, job->data);
        pqueue_pop(pq);
        if (job->type == PERIODIC) {
            register_timer(job->id, job->delay, job->type, job->callback, job->data);
        }
        job = pqueue_peek(pq);
    }

    last_tick = curr_tick;
    pq->time += TICK_10000_US;

    seL4_IRQHandler_Ack(timer_irq_handler);

    return CLOCK_R_OK;
}

int stop_timer(void)
{
    if (!timer_initialised) {
        return CLOCK_R_UINT;
    }

    timer_initialised = 0;
    pqueue_destroy(pq);

    seL4_IRQHandler_Clear(timer_irq_handler);

    return CLOCK_R_OK;
}
