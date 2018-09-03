
#include <sel4/sel4.h>
#include <stdint.h>
#include <clock/clock.h>
#include "time_syscalls.h"
#include "picoro.h"

/* usleep handler*/
static void usleep_handler(uint32_t id, void *data){
    resume((coro) data, NULL);
    seL4_SetMR(0, 0);
}

int syscall_usleep(void) {
    int msec = seL4_GetMR(1);

    // register timer uses microsecnds
    int timer = register_timer(msec * 1000, ONE_SHOT, usleep_handler, get_running());
    yield(NULL);
    return 1;
}

int syscall_time_stamp(void) {
    int64_t time = get_time();
    seL4_SetMR(0, time);
    return 1;
}
