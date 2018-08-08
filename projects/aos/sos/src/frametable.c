#include <autoconf.h>
#include <utils/util.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <cspace/cspace.h>
#include <aos/sel4_zf_logif.h>
#include <aos/debug.h>

#include <clock/clock.h>
#include <cpio/cpio.h>
#include <elf/elf.h>
#include <serial/serial.h>

#include "frametable.h"

void frame_table_init(void) {

}

seL4_Word frame_alloc(seL4_Word *vaddr) {
    (void) vaddr;
    return 0;
}

void frame_free(seL4_Word page) {

    (void) page;
}

