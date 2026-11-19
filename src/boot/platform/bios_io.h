/*
 * bios_io.h - the one processor instruction that bios.c cannot write in C.
 *
 * Purpose:
 *   Reading a byte from an I/O port (the PS/2 keyboard controller). It is
 *   implemented in bios_entry.c with the instruction "inb"; the host test
 *   replaces it with a fake, so bios.c itself is plain C. Private to
 *   src/boot/platform.
 *
 * Depends on:
 *   <stdint.h>.
 */

#ifndef JUSTOS_BOOT_PLATFORM_BIOS_IO_H
#define JUSTOS_BOOT_PLATFORM_BIOS_IO_H

#include <stdint.h>

/* The inb instruction: reads one byte from the I/O port. */
uint8_t jp_bios_port_read8(uint16_t port);

#endif /* JUSTOS_BOOT_PLATFORM_BIOS_IO_H */
