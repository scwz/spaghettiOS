
#pragma once

#include <stdint.h>
#include <sos.h>
#include "vnode.h"

int console_init(void);
int console_open(int flags);
int console_close(void);
int console_read(struct uio *uio);
int console_write(struct uio *uio);
