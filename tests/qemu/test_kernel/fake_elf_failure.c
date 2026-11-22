/*
 * fake_elf_failure.c - makes Stage 2 fail on purpose, for the QEMU failure-path tests.
 *
 * It replaces src/kernel/elf.c in the variants "recovery" (the result JP_BOOT_FAILED, which Stage 3 can
 * recover from) and "panic" (JP_TEST_ELF_FAILURE_RESULT is JP_BOOT_ERROR_INVALID_PARAM, which it cannot)
 * of the test kernel, so that jp_kernel_init() runs CPU, Memory, Paging, Stack and Heap
 * for real and then fails at the ELF step. The entry stub must then take the
 * failure path: jp_kernel_stage2_failure() records the result and goes on to
 * jp_kernel_stage2_complete(). Test code only; never linked into production.
 */

#include "justos/kernel/elf.h"

/* The result that Stage 2 fails with; the build can choose another one with -DJP_TEST_ELF_FAILURE_RESULT=... */
#ifndef JP_TEST_ELF_FAILURE_RESULT
#define JP_TEST_ELF_FAILURE_RESULT JP_BOOT_FAILED
#endif

JPBootResult jp_kernel_elf_initialize(JPBootContext *context)
{
	(void)context;
	return JP_TEST_ELF_FAILURE_RESULT;
}
