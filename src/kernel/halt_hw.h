/*
 * halt_hw.h - private access of init.c to "stop the processor".
 *
 * Purpose:
 *   Stopping the processor is the one thing that C cannot do. Host tests
 *   replace it; the real functions are halt_hw_x86_64.c and
 *   halt_hw_aarch64.c.
 *
 * Limitations:
 *   Private to src/kernel.
 */

#ifndef JUSTOS_SRC_KERNEL_HALT_HW_H
#define JUSTOS_SRC_KERNEL_HALT_HW_H

/* Masks interrupts and waits for ever. Never returns. */
_Noreturn void jp_kernel_halt(void);

#endif /* JUSTOS_SRC_KERNEL_HALT_HW_H */
