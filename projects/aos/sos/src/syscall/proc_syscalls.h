
#pragma once

#include "../proc/proc.h"

int syscall_proc_create(struct proc *curproc);
int syscall_proc_delete(struct proc *curproc);
int syscall_proc_my_id(struct proc *curproc);
int syscall_proc_status(struct proc *curproc);
int syscall_proc_wait(struct proc *curproc);
