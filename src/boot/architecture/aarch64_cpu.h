/*
 * aarch64_cpu.h - the processor instructions that aarch64.c cannot write in C.
 *
 * Purpose:
 *   Four functions implemented in assembly (aarch64_enter.S). Host tests
 *   replace them with fakes, so all other code of aarch64.c is tested on the
 *   host. Private to src/boot/architecture.
 *
 * Depends on:
 *   <stdint.h>, justos/boot/boot_types.h.
 */

#ifndef JUSTOS_BOOT_ARCHITECTURE_AARCH64_CPU_H
#define JUSTOS_BOOT_ARCHITECTURE_AARCH64_CPU_H

#include <stdint.h>

#include "justos/boot/boot_types.h"

#define JP_AARCH64_DAIF_ALL_MASKED 0xFULL      /* D, A, I, F */
#define JP_AARCH64_SCTLR_MMU (1ULL << 0)       /* the MMU of the current level is on */

typedef struct JPA64CpuState {
	uint64_t current_el;     /* 1, 2, or 3 */
	uint64_t daif;           /* the four mask bits (bit 3 D, 2 A, 1 I, 0 F) */
	uint64_t sctlr;          /* SCTLR_EL1, or SCTLR_EL2 when running at EL2 */
	uint64_t midr;           /* MIDR_EL1 */
} JPA64CpuState;

/* msr daifset, #0xf */
void jp_aarch64_disable_interrupts(void);

/* Reads the exception level, DAIF, the system control register, and MIDR_EL1. */
void jp_aarch64_read_cpu_state(JPA64CpuState *state);

/*
 * The register steps of the jump into the kernel (see milestone0.md): MAIR_EL1,
 * TCR_EL1, TTBR1_EL1 and TTBR0_EL1, the drop from EL2 to EL1 when needed,
 * SCTLR_EL1 with the MMU and caches on, the stack, the context in X0, and the
 * branch to the entry. Never returns.
 */
_Noreturn void jp_aarch64_jump_to_kernel(uint64_t ttbr1, uint64_t ttbr0, uint64_t stack,
	uint64_t entry, const JPBootContext *context);

/* Masks interrupts and waits for events in a loop. Never returns. */
_Noreturn void jp_aarch64_park(void);

#endif /* JUSTOS_BOOT_ARCHITECTURE_AARCH64_CPU_H */
