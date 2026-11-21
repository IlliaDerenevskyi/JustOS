/*
 * boot_info.h - the read-only summary of the boot state (Stage 4).
 *
 * Purpose:
 *   Gathers the states that Stages 1 to 4 report into one copy and names the
 *   detection states (milestone0.md, "4 Boot Information Aggregation
 *   Contract"; file specifications 3 and 4).
 *
 * Limitations:
 *   An aggregator: it never searches, never guesses, never changes anything.
 *   A state that is not available is carried over exactly as its owner
 *   reported it (Rule 34).
 *
 * Depends on:
 *   justos/boot/boot_types.h, justos/boot/boot_status.h (jp_boot_status_get).
 *
 * Must not depend on:
 *   boot_ui.h, firmware.
 */

#ifndef JUSTOS_BOOT_BOOT_INFO_H
#define JUSTOS_BOOT_BOOT_INFO_H

#include "justos/boot/boot_types.h"

typedef struct JPBootSummary {
	JPArchitectureType architecture;          /* x86_64 or AArch64 */
	JPBootEnvironmentType environment;        /* UEFI or BIOS */
	JPDetectionState environment_state;
	JPDetectionState hardware_state;
	JPDetectionState memory_map_state;
	JPDetectionState framebuffer_state;
	JPKernelValidationState kernel_validation_state;
	JPKernelLoadState kernel_load_state;
	bool status_available;                    /* true only if Stage 3 reported a status */
	JPBootStatusState status;
	JPBootConfigState config_state;
	JPBootConfigSource config_source;
	JPBootSelectionOutcome selection_outcome;
} JPBootSummary;

JPBootResult jp_boot_info_summarize(const JPBootContext *context, JPBootSummary *summary);
const char *jp_boot_info_state_name(JPDetectionState state);

#endif /* JUSTOS_BOOT_BOOT_INFO_H */
