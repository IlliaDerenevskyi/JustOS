/*
 * boot_ui.h - the boot screens (Stage 4).
 *
 * Purpose:
 *   The interactive menu of phase A and the text screens of phase B
 *   (milestone0.md, "4 Boot UI Contract"; file specifications 5 and 6).
 *
 * Responsibilities:
 *   - jp_boot_ui_select (phase A only): draws the menu into the checked
 *     framebuffer, reads keys, ends with APPLIED (F10) or DISCARDED (Esc);
 *   - jp_boot_ui_present_summary, jp_boot_ui_present_failure (phase B only):
 *     lines of text through jp_boot_diagnostic_output.
 *
 * Limitations:
 *   Which functions exist in which program is decided by the phase macros of
 *   "4 Decided rules" (JP_BOOT_STAGE4_PHASE_A / JP_BOOT_STAGE4_PHASE_B).
 *
 * Depends on:
 *   justos/boot/boot_types.h, boot_info.h.
 *
 * Must not depend on:
 *   firmware (except the keyboard input of Stage 1), the heap.
 */

#ifndef JUSTOS_BOOT_BOOT_UI_H
#define JUSTOS_BOOT_BOOT_UI_H

#include "justos/boot/boot_info.h"
#include "justos/boot/boot_types.h"

JPBootResult jp_boot_ui_select(JPBootContext *context, JPBootSelectionOutcome *outcome);
JPBootResult jp_boot_ui_present_summary(JPBootContext *context, const JPBootSummary *summary);
JPBootResult jp_boot_ui_present_failure(JPBootContext *context);

#endif /* JUSTOS_BOOT_BOOT_UI_H */
