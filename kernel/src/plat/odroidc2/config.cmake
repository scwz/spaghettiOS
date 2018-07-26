#
# Copyright 2017, Data61
# Commonwealth Scientific and Industrial Research Organisation (CSIRO)
# ABN 41 687 119 230.
#
# This software may be distributed and modified according to the terms of
# the GNU General Public License version 2. Note that NO WARRANTY is provided.
# See "LICENSE_GPLv2.txt" for details.
#
# @TAG(DATA61_GPL)
#

cmake_minimum_required(VERSION 3.7.2)

if(KernelPlatformOdroidc2)
    set(KernelArmCortexA53 ON)
    set(KernelArchArmV8a ON)
    config_set(KernelPlatform PLAT "odroidc2")
    set(KernelArmMachFeatureModifiers "+crc" CACHE INTERNAL "")
    if(KernelSel4ArchAarch64)
        set(KernelHaveFPU ON)
    endif()
endif()

add_sources(
    DEP "KernelPlatformOdroidc2"
    CFILES
        src/plat/odroidc2/machine/hardware.c
        src/plat/odroidc2/machine/io.c
        src/arch/arm/machine/generic_timer.c
)
