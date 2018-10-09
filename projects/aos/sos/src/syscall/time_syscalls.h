
#pragma once

#include "../proc/proc.h"

int syscall_usleep(struct proc *curproc);
int syscall_time_stamp(struct proc *curproc);
