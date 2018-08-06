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
#define DIFF(a, b) ((a > b) ? (a - b) : (b - a))

static volatile struct {
    uint32_t timer_mux;
    uint32_t timer_f;
    uint32_t timer_g;
    uint32_t timer_h;
    uint32_t timer_i;
} *timer;

static volatile struct {
    uint32_t lo;
    uint32_t hi;
} *timer_e;

static seL4_CPtr timer_irq_handler_f, timer_irq_handler_g;
static int timer_initialised = 0;
static struct pqueue *pq = NULL;
static uint64_t last_tick = 0;

int start_timer(seL4_CPtr ntfn_f, seL4_CPtr irqhandler_f, seL4_CPtr ntfn_g, seL4_CPtr ihg, void *device_vaddr)
{
    if (timer_initialised) {
        stop_timer();
    }

    pq = pqueue_init();

    // initalise timer
    timer = device_vaddr + (TIMER_MUX & MASK((size_t) seL4_PageBits));
    timer_e = device_vaddr + (TIMER_E_LO_PADDR & MASK((size_t) seL4_PageBits));
    timer_irq_handler_f = irqhandler_f;
    timer_irq_handler_g = ihg;
    timer_initialised = 1;

    // setup registers
    timer->timer_mux |= TIMER_F_EN | TIMER_F_MODE | (TIMEBASE_1_US << TIMER_F_INPUT_CLK );
    timer->timer_mux |= TIMER_G_EN | (TIMEBASE_1_US << TIMER_G_INPUT_CLK );
    timer->timer_mux |= (0b001 << 8);
    timer_e->lo = 1;
//    timer->timer_f |= TIMER_MAX;
    last_tick = timestamp_ms(timestamp_get_freq());
    return CLOCK_R_OK;
}


uint32_t register_timer(uint64_t delay, job_type_t type, timer_callback_t callback, void *data)
{   
    printf("TIMER registered");
    uint32_t retval = pqueue_push(pq, 0, delay, type, callback, data);
    struct job * job = pqueue_peek(pq);
    timer->timer_g = (job->delay > TIMER_MAX) ? TIMER_MAX : job->delay;
#if 0
    if(job->tick - pq->time < TIMER_MAX){
        printf("G TIMER registered");
        timer->timer_g = (uint16_t) job->delay;
        printf("timer g %u\n\n", TIMER_VAL(timer->timer_g));
        COMPILER_MEMORY_FENCE();
    }
#endif
    return retval;

}

int remove_timer(uint32_t id)
{
    if (!timer_initialised) {
        return CLOCK_R_UINT;
    }
    return pqueue_remove(pq, id);
}

static int in_interval(uint64_t curr_time, struct job *job) {
    return (DIFF(curr_time, job->start) >= DIFF(job->delay, 10000)) &&
            (DIFF(curr_time, job->start) < job->delay + 10000);
}

int timer_interrupt(void)
{
    if (!timer_initialised) {
        return CLOCK_R_UINT;
    }
    struct job *job = pqueue_peek(pq);
    if (job == NULL) {
        return CLOCK_R_FAIL;
    }
    uint64_t curr_time = get_timer_e_val();
    uint64_t curr_tick = 0;
//    printf("C %lu\n", curr_time);
//    printf("DIFF %lu\n", DIFF(curr_time, job->start));
    if (in_interval(curr_time, job)) {
        printf("HELLO THERE\n");
        while (job != NULL && in_interval(curr_time, job)) {
            curr_tick = timestamp_ms(timestamp_get_freq());
            printf("CALLBACK RECEIVED: %lu ms diff: %lu ms\n", curr_tick, curr_tick - last_tick);
            job->callback(job->id, job->data);
            pqueue_pop(pq);
            job = pqueue_peek(pq);
        }
        if (job != NULL) {
            printf("%lu %lu \n", curr_time, job->start);
            timer->timer_g = (job->delay > TIMER_MAX) ? TIMER_MAX : job->delay;
        }
    }
    else {
        uint64_t diff = DIFF(curr_time, job->delay);
//        printf(" hi%lu\n", diff);
        timer->timer_g = (diff > TIMER_MAX) ? TIMER_MAX : diff;
    }
#if 0
    if(job == NULL){
        return CLOCK_R_FAIL;
    }
      
    if(job->tick - pq->time < TIMER_MAX){
        pq->time += job->tick - pq->time;
    } else {
        pq->time += TIMER_MAX;
    }
    printf("===TIMER Intterupt %lu\n", pq->time);
    printf("job->tick = %lu\n", job->tick); 
    printf("TIMER E %u %u\n\n", timer_e->lo, timer_e->hi);

    uint64_t curr_tick = 0;
    if(job->tick - pq->time <= TIMER_MAX){
        while (job != NULL && job->tick - pq->time <= 4) {
            curr_tick = timestamp_ms(timestamp_get_freq());
            printf("CALLBACK RECEIVED: %lu ms diff: %lu ms\n", curr_tick, curr_tick - last_tick);
            job->callback(job->id, job->data);
            pqueue_pop(pq);
            job = pqueue_peek(pq);
        }
        if(job->tick - pq->time < TIMER_MAX){
            printf("timer g actiaved %lu\n\n", job->tick - pq->time);
            timer->timer_g = (uint16_t) (job->tick - pq->time);
        }
    }
    
#endif
    last_tick = curr_tick ? curr_tick : last_tick;

    seL4_IRQHandler_Ack(timer_irq_handler_f);
    seL4_IRQHandler_Ack(timer_irq_handler_g);

    return CLOCK_R_OK;
}

int stop_timer(void)
{
    if (!timer_initialised) {
        return CLOCK_R_UINT;
    }

    timer_initialised = 0;
    pqueue_destroy(pq);

    seL4_IRQHandler_Clear(timer_irq_handler_f);

    return CLOCK_R_OK;
}

uint64_t get_timer_e_val(void) {
    return timer_e->lo | timer_e->hi; 
}
