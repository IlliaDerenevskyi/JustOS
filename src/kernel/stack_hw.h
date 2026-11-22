/*
 * stack_hw.h - private access of stack.c to the stack pointer.
 *
 * Purpose:
 *   Reading the stack pointer is the one thing that C cannot do. Host tests
 *   replace it with a scripted value; the QEMU tests run the real one
 *   (stack_hw_x86_64.c, stack_hw_aarch64.c).
 *
 * Limitations:
 *   Private to src/kernel. Reads only; the stack pointer is never written.
 */

#ifndef JUSTOS_SRC_KERNEL_STACK_HW_H
#define JUSTOS_SRC_KERNEL_STACK_HW_H

#include <stdint.h>

/* x86_64: the value of RSP. AArch64: the value of SP. */
uint64_t jp_kernel_stack_current_pointer(void);

#endif /* JUSTOS_SRC_KERNEL_STACK_HW_H */
