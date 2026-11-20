/*
 * fake_stage2_complete.c - stands in for the Stage 4 continuation.
 *
 * Stage 2 ends by calling jp_kernel_stage2_complete(context), which Stage 4
 * (boot_manager.c) implements. Stage 4 does not exist yet, so the test kernel
 * provides this stand-in: it goes straight to kernel_main, the kernel that
 * Stage 4 would start. Test code only; never linked into production.
 */

#include "justos/kernel/init.h"

void kernel_main(JPBootContext *context) __attribute__((noreturn));

_Noreturn void jp_kernel_stage2_complete(JPBootContext *context)
{
	kernel_main(context);
}
