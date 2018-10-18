
#include <sel4/sel4.h>
#include <stdint.h>
#include <clock/clock.h>
#include "time_syscalls.h"
#include "../picoro/picoro.h"
#include "../proc/proc.h"

volatile static int timer_fired;

/* usleep handler*/
static void 
usleep_handler(uint32_t id, void *data) 
{
    timer_fired = 1;
    resume((coro) data, NULL);
    seL4_SetMR(0, 0);
}

int
syscall_usleep(struct proc *curproc) 
{
    int msec = seL4_GetMR(1);

    timer_fired = 0;
    // register timer uses microsecnds
    int timer = register_timer(msec * 1000, ONE_SHOT, usleep_handler, (void *) get_running());
    if (!timer) {
        seL4_SetMR(0, 1);
        return 1;
    }
    if (!timer_fired) {
        yield(NULL);
    }
    return 1;
}

int 
syscall_time_stamp(struct proc *curproc) 
{
    int64_t time = get_time();
    seL4_SetMR(0, time);
    return 1;
}
