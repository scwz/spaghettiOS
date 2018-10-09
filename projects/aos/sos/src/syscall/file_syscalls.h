
#pragma once

int syscall_write(struct proc *curproc);
int syscall_read(struct proc *curproc);
int syscall_open(struct proc *curproc);
int syscall_close(struct proc *curproc);
int syscall_stat(struct proc *curproc);
int syscall_getdirent(struct proc *curproc);
