
#include <sel4/sel4.h>
#include <stdint.h>
#include "time_syscalls.h"

/* usleep handler*/
static void usleep_handler(uint32_t id, void* reply_cptr){
#if 0
    seL4_MessageInfo_t reply_msg;
    seL4_CPtr reply = *((seL4_CPtr*) reply_cptr);
    free(reply_cptr);
    
    printf("handler called cptr: %ld\n", reply);
    seL4_SetMR(0, 0);
    reply_msg = seL4_MessageInfo_new(0, 0, 0, 1);
    seL4_Send(reply, reply_msg);
    cspace_free_slot(&cspace, reply);
#endif
}

int syscall_usleep(void) {
    return 0;
}

int syscall_time_stamp(void) {
    int64_t time = get_time();
    seL4_SetMR(0, time);
    return 1;
}
