/*
 * stage_lines.h - what the test kernel says about Stage 3 and Stage 4.
 *
 * Purpose:
 *   Writes the lines "JP|stage3.<key>=<value>" and "JP|stage4.<key>=<value>" (the serial protocol of
 *   milestone0.md) from what the real Stage 3 and Stage 4 left in the context. The test kernel only reads: it
 *   uses the public read functions of Stage 3 and Stage 4, never the fields behind them.
 *
 * Limitations:
 *   Test code, never part of the boot code. Freestanding: no libc, no heap.
 */

#ifndef JUSTOS_TESTS_QEMU_TEST_KERNEL_STAGE_LINES_H
#define JUSTOS_TESTS_QEMU_TEST_KERNEL_STAGE_LINES_H

#include "justos/boot/boot_types.h"
#include "report.h"

void jp_test_stage_lines_emit(const JPBootContext *context, const JPTestSink *sink);

#endif /* JUSTOS_TESTS_QEMU_TEST_KERNEL_STAGE_LINES_H */
