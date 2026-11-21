/*
 * fake_elf_failure.c - makes Stage 2 fail on purpose, for the QEMU failure-path test.
 *
 * It replaces src/kernel/elf.c in the "stage2_failure" variant of the test
 * kernel, so that jp_kernel_init() runs CPU, Memory, Paging, Stack and Heap
 * for real and then fails at the ELF step. The entry stub must then take the
 * failure path: jp_kernel_stage2_failure() records the result and goes on to
 * jp_kernel_stage2_complete(). Test code only; never linked into production.
 */

#include "justos/kernel/elf.h"

JPBootResult jp_kernel_elf_initialize(JPBootContext *context)
{
	(void)context;
	return JP_BOOT_FAILED;
}
