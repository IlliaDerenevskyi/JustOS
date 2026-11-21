/*
 * fake_stage3_lines.h - what the Stage 4 stand-ins of the test kernel say about Stage 3.
 *
 * The lines "JP|stage3.<key>=<value>" go through jp_boot_diagnostic_output(),
 * the real Stage 3 output (the serial port, and the screen when it is valid),
 * so a missing line in the serial log means that the real output failed.
 * Test code only.
 */

#ifndef JUSTOS_TESTS_QEMU_TEST_KERNEL_FAKE_STAGE3_LINES_H
#define JUSTOS_TESTS_QEMU_TEST_KERNEL_FAKE_STAGE3_LINES_H

#include "justos/boot/boot_types.h"

/* Sends one line through the Stage 3 output. */
void jp_test_stage3_line(JPBootContext *context, const char *text);

/* Finalizes nothing; sends the lines status, attempt, events, error, and report (FINAL or NOT_FINAL). */
void jp_test_stage3_report(JPBootContext *context);

#endif /* JUSTOS_TESTS_QEMU_TEST_KERNEL_FAKE_STAGE3_LINES_H */
