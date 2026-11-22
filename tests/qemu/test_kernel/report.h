/*
 * report.h - the data list of the Milestone 0 test kernel.
 *
 * Purpose:
 *   Writes every item of the received JPBootContext as "JP|key=value" lines
 *   (the serial protocol of milestone0.md) and decides PASS or FAIL from the
 *   received data only.
 *
 * Limitations:
 *   Test code, never part of the boot code. Freestanding: no libc, no heap.
 *   The output is written character by character through a sink, so the same
 *   code feeds the serial port in QEMU and a buffer in host tests.
 *
 * Depends on:
 *   justos/boot/boot_types.h.
 */

#ifndef JUSTOS_TESTS_QEMU_TEST_KERNEL_REPORT_H
#define JUSTOS_TESTS_QEMU_TEST_KERNEL_REPORT_H

#include "justos/boot/boot_types.h"

typedef struct JPTestSink {
	void (*put_char)(void *user, char c);
	void *user;
} JPTestSink;

/* Writes the whole list and the RESULT line. Returns true for PASS. */
bool jp_test_report_emit(const JPBootContext *context, const JPTestSink *sink);

#endif /* JUSTOS_TESTS_QEMU_TEST_KERNEL_REPORT_H */
