#include <autoconf.h>
#include <utils/util.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <utils/page.h>
#include <sos.h>

#include <cspace/cspace.h>
#include <aos/sel4_zf_logif.h>
#include <aos/debug.h>
#include "frametable.h"
#include "../proc/proc.h"

void * shared_buf_begin;
//shared_buf 0 is reserved for the kernel
void shared_buf_init(cspace_t * cspace);

void sos_map_buf(pid_t pid);

size_t sos_copyin(pid_t pid, seL4_Word kernel_vaddr, size_t len);
size_t sos_copyout(pid_t pid, seL4_Word kernel_vaddr, size_t len);
void share_buf_check_len(size_t * len);
void shared_buf_zero(pid_t pid);