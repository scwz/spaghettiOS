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
#pragma once

#include <stdint.h>
#include <sel4/sel4.h>
#include <cspace/cspace.h>
#include <clock/device.h>
#include <clock/timestamp.h>

/*
 * Return codes for driver functions
 */
#define CLOCK_R_OK     0        /* success */
#define CLOCK_R_UINT (-1)       /* driver not initialised */
#define CLOCK_R_CNCL (-2)       /* operation cancelled (driver stopped) */
#define CLOCK_R_FAIL (-3)       /* operation failed for other reason */

typedef uint64_t timestamp_t;
typedef void (*timer_callback_t)(uint32_t id, void *data);

typedef enum {
    PERIODIC, 
    ONE_SHOT
} job_type_t;

/*
 * Initialise driver. Performs implicit stop_timer() if already initialised.
 *    interrupt_ep:       A (possibly badged) async endpoint that the driver
                          should use for deliverying interrupts to
 *
 * Returns CLOCK_R_OK iff successful.
 */
int start_timer(cspace_t *cspace, seL4_CPtr ntfn, void *device_vaddr);

/*
 * Register a callback to be called after a given delay
 *    delay:  Delay time in microseconds before callback is invoked
 *    callback: Function to be called
 *    data: Custom data to be passed to callback function
 *
 * Returns 0 on failure, otherwise an unique ID for this timeout
 */
uint32_t register_timer(uint64_t delay, job_type_t type, timer_callback_t callback, void *data);

/*
 * Remove a previously registered callback by its ID
 *    id: Unique ID returned by register_time
 * Returns CLOCK_R_OK iff successful.
 */
int remove_timer(uint32_t id);

/*
 * Handle an interrupt message sent to 'interrupt_ep' from start_timer
 *
 * Returns CLOCK_R_OK iff successful
 */
int timer_interrupt(void);

/*
 * Stop clock driver operation.
 *
 * Returns CLOCK_R_OK iff successful.
 */
int stop_timer(void);

timestamp_t get_time(void);
