/*
 * diagnostic_hw.h - private machine access of the Stage 3 diagnostic output.
 *
 * Purpose:
 *   Reading and writing the serial port, and stopping the processor, are
 *   things that C cannot do. Host tests replace these functions with fakes;
 *   the QEMU tests run the real ones (diagnostic_hw_x86_64.c,
 *   diagnostic_hw_aarch64.c). No logic: one or two instructions each.
 *
 * Limitations:
 *   Private to src/boot.
 */

#ifndef JUSTOS_BOOT_DIAGNOSTIC_HW_H
#define JUSTOS_BOOT_DIAGNOSTIC_HW_H

#include <stdint.h>

/*
 * x86_64: one byte read from the port 0x3F8 + offset.
 * AArch64: one 32-bit register read at the address 0x09000000 + offset.
 */
uint32_t jp_diagnostic_serial_read(uint32_t offset);

/* The same registers, written. */
void jp_diagnostic_serial_write(uint32_t offset, uint32_t value);

/* Masks interrupts and waits for ever. Never returns. */
_Noreturn void jp_diagnostic_halt(void);

#endif /* JUSTOS_BOOT_DIAGNOSTIC_HW_H */
