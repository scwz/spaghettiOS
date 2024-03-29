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

#include <sel4/sel4.h>
#include <cspace/cspace.h>
#include <elf/elf.h>
#include <elf.h>

int elf_load(pid_t pid, cspace_t *cspace, seL4_CPtr loader_vspace, seL4_CPtr loadee_vspace, char *elf_file);

// cannot use for init process
int elf_load_fs(pid_t pid, cspace_t *cspace, seL4_CPtr loader_vspace, seL4_CPtr loadee_vspace, char *path);

uintptr_t get_last_vsyscall_table(void);

uint64_t get_last_entry_point(void);
