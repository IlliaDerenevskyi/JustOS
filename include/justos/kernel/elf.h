/*
 * elf.h - Stage 2 check of the loaded kernel image.
 *
 * Purpose:
 *   Checks that the kernel image state that Stage 1 recorded is consistent.
 *
 * Responsibilities:
 *   - ELF class and machine match the architecture; the type is supported;
 *   - every segment is loaded, mapped, and permissioned, has a valid range
 *     (no overflow, canonical address), and memory_size >= file_size;
 *   - the entry point lies in an executable segment.
 *
 * Limitations:
 *   Does not load, copy, relocate, map, or change the permissions of anything.
 *
 * Public API:
 *   jp_kernel_elf_initialize.
 *
 * Depends on:
 *   justos/boot/boot_types.h.
 *
 * Must not depend on:
 *   firmware, JPBootInfo, Stage 3, Stage 4.
 */

#ifndef JUSTOS_KERNEL_ELF_H
#define JUSTOS_KERNEL_ELF_H

#include "justos/boot/boot_types.h"

JPBootResult jp_kernel_elf_initialize(
	JPBootContext *context
);

#endif /* JUSTOS_KERNEL_ELF_H */
