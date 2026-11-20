/*
 * paging_hw.h - private access of paging.c to the machine.
 *
 * Purpose:
 *   The few things that C cannot do: turn a table address into a pointer,
 *   read and write the root register, set WXN. Host tests replace them with a
 *   fake (a buffer that stands for the table pool); the QEMU tests run the
 *   real ones (paging_hw_x86_64.c, paging_hw_aarch64.c).
 *
 * Limitations:
 *   Private to src/kernel.
 */

#ifndef JUSTOS_SRC_KERNEL_PAGING_HW_H
#define JUSTOS_SRC_KERNEL_PAGING_HW_H

#include <stdint.h>

/*
 * The pointer to the table page at `physical` (the identity map makes it the
 * same number). NULL when the page cannot be reached.
 */
uint64_t *jp_kernel_paging_table_at(uint64_t physical);

/* x86_64: CR3 with the low 12 bits cleared. AArch64: TTBR0_EL1 without the ASID and the low bits. */
uint64_t jp_kernel_paging_read_root(void);

/*
 * x86_64: mov cr3. AArch64: msr ttbr0_el1, then dsb ishst, tlbi vmalle1, dsb ish, isb.
 */
void jp_kernel_paging_write_root(uint64_t root);

/* AArch64 only: set SCTLR_EL1.WXN, then isb. */
void jp_kernel_aarch64_set_wxn(void);

#endif /* JUSTOS_SRC_KERNEL_PAGING_HW_H */
