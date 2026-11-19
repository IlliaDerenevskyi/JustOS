/*
 * x86_64_cpu.h - the processor instructions that x86_64.c cannot write in C.
 *
 * Purpose:
 *   Three functions implemented in assembly (x86_64_enter.S). Host tests
 *   replace them with fakes, so all other code of x86_64.c is tested on the
 *   host. Private to src/boot/architecture.
 *
 * Depends on:
 *   <stdint.h>, justos/boot/boot_types.h.
 */

#ifndef JUSTOS_BOOT_ARCHITECTURE_X86_64_CPU_H
#define JUSTOS_BOOT_ARCHITECTURE_X86_64_CPU_H

#include <stdint.h>

#include "justos/boot/boot_types.h"

#define JP_X86_64_EFER_LMA (1ULL << 10)    /* long mode is active */
#define JP_X86_64_CR0_PG (1ULL << 31)      /* paging is on */
#define JP_X86_64_RFLAGS_IF (1ULL << 9)    /* interrupts are enabled */

typedef struct JPX86CpuState {
	uint64_t rflags;
	uint64_t cr0;
	uint64_t cr4;
	uint64_t efer;
} JPX86CpuState;

/*
 * The four functions are written in assembly with the System V argument
 * registers (RDI, RSI, RDX, RCX, R8), also when the C code is built for UEFI,
 * where the C calling convention is a different one: the attribute makes the C
 * compiler use the System V convention for exactly these functions.
 */
#if defined(__x86_64__) || defined(_M_X64)
#define JP_X86_64_ASM_ABI __attribute__((sysv_abi))
#else
#define JP_X86_64_ASM_ABI
#endif

/* The cli instruction. */
void jp_x86_64_disable_interrupts(void) JP_X86_64_ASM_ABI;

/* Reads RFLAGS, CR0, CR4, and EFER. */
void jp_x86_64_read_cpu_state(JPX86CpuState *state) JP_X86_64_ASM_ABI;

/*
 * The register steps of the jump into the kernel: set EFER.NXE and CR0.WP,
 * load the GDT at gdt (limit 23) and the segment registers, load CR3 with
 * root, switch to the stack, push a zero return address, put context in RDI,
 * and jump to entry. Never returns.
 */
_Noreturn void jp_x86_64_jump_to_kernel(uint64_t root, uint64_t gdt, uint64_t stack,
	uint64_t entry, const JPBootContext *context) JP_X86_64_ASM_ABI;

/* cli and hlt in a loop. Never returns. */
_Noreturn void jp_x86_64_park(void) JP_X86_64_ASM_ABI;

#endif /* JUSTOS_BOOT_ARCHITECTURE_X86_64_CPU_H */
