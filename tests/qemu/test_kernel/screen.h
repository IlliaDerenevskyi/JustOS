/*
 * screen.h - the on-screen data list of the test kernel.
 *
 * Draws the same list that report.c writes to the serial port onto the
 * framebuffer, text only, with the built-in 8x16 font. Test code only.
 */

#ifndef JUSTOS_TESTS_QEMU_TEST_KERNEL_SCREEN_H
#define JUSTOS_TESTS_QEMU_TEST_KERNEL_SCREEN_H

#include <stddef.h>

#include "justos/boot/boot_types.h"

/* Characters in one column of the list. */
#define JP_TEST_SCREEN_COLUMN_CHARS 80

/*
 * Draws the list on the framebuffer described by context->framebuffer.
 * pixels is the writable memory of that framebuffer (the caller turns
 * base_address into a pointer). The list fills the screen column by column.
 * If it does not fit, the last drawn line says how many lines were left out
 * (they are complete on the serial port).
 * Returns the number of text lines drawn; 0 if the framebuffer state is not
 * DETECTED, the format is not RGB_888 or BGR_888, pixels is NULL, or not even
 * one line fits (then nothing is written).
 */
size_t jp_test_screen_show(const JPBootContext *context, void *pixels);

#endif /* JUSTOS_TESTS_QEMU_TEST_KERNEL_SCREEN_H */
