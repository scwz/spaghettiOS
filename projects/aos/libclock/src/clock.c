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
#include <cspace/cspace.h>
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

/* taken from projects/aos/sos/src/network.c */
static seL4_CPtr init_irq(cspace_t *cspace, int irq_number, int edge_triggered,
                          seL4_CPtr ntfn) {
    seL4_CPtr irq_handler = cspace_alloc_slot(cspace);
    ZF_LOGF_IF(irq_handler == seL4_CapNull, "Failed to alloc slot for irq handler!");
    seL4_Error error = cspace_irq_control_get(cspace, irq_handler, seL4_CapIRQControl, irq_number, edge_triggered);
    ZF_LOGF_IF(error, "Failed to get irq handler for irq %d", irq_number);
    error = seL4_IRQHandler_SetNotification(irq_handler, ntfn);
    ZF_LOGF_IF(error, "Failed to set irq handler ntfn");
    seL4_IRQHandler_Ack(irq_handler);
    return irq_handler;
}

int start_timer(cspace_t *cspace, seL4_CPtr ntfn, void *device_vaddr)
{
    if (timer_initialised) {
        stop_timer();
    }

    pq = pqueue_init();

    // initalise timer
    timer = device_vaddr + (TIMER_MUX & MASK((size_t) seL4_PageBits));
    timer_irq_handler = init_irq(cspace, TIMER_F_IRQ, 1, ntfn);
    timer_initialised = 1;

    // setup registers
    timer->timer_mux |= TIMER_F_EN | TIMER_F_MODE | (TIMEBASE_1_US << TIMER_F_INPUT_CLK) ; 
    timer->timer_f |= TICK_10000_US;

    last_tick = timestamp_ms(timestamp_get_freq());
    printf("TIMER STARTED: %lu ms\n", last_tick);

    return CLOCK_R_OK;
}

uint32_t register_timer(uint64_t delay, job_type_t type, timer_callback_t callback, void *data)
{
    return pqueue_push(pq, 0, delay, type, callback, data);
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
    uint64_t curr_tick = 0;
    while (job != NULL && (((job->tick >= pq->time) && (job->tick < pq->time + TICK_10000_US)))) {
        curr_tick = timestamp_ms(timestamp_get_freq());
        printf("CALLBACK RECEIVED: %lu ms diff: %lu ms\n", curr_tick, curr_tick - last_tick);
        job->callback(job->id, job->data);
        pqueue_pop(pq);
        job = pqueue_peek(pq);
    }

    last_tick = curr_tick ? curr_tick : last_tick;
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
