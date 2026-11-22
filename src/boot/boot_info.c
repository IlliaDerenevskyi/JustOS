/*
 * boot_info.c - the read-only summary of the boot state (Stage 4).
 *
 * Purpose:
 *   Implements the Boot Information Aggregation Contract (milestone0.md, "4
 *   Boot Information Aggregation Contract" and file specifications 3 and 4).
 *
 * Responsibilities:
 *   Copy each state, unchanged, from the module that owns it into the
 *   summary; name a detection state.
 *
 * Limitations:
 *   Keeps no state and writes nothing in the context. A state that is not
 *   available is carried over exactly as reported (Rule 34); the summary
 *   never holds a guessed value.
 *
 * Depends on:
 *   justos/boot/boot_info.h, justos/boot/boot_status.h.
 *
 * Must not depend on:
 *   firmware, the heap, boot_ui.h.
 */

#include "justos/boot/boot_info.h"

#include "justos/boot/boot_status.h"

/* ---- Private data ---- */

/* The display name of each JPDetectionState, in the order of the enumeration. */
static const char *const detection_state_names[] = {
	"UNKNOWN",             /* JP_DETECTION_STATE_UNKNOWN */
	"NOT DETECTED",        /* JP_DETECTION_STATE_NOT_DETECTED */
	"NOT INITIALIZED",     /* JP_DETECTION_STATE_NOT_INITIALIZED */
	"DETECTED",            /* JP_DETECTION_STATE_DETECTED */
	"FAILED"               /* JP_DETECTION_STATE_FAILED */
};

#define JP_DETECTION_STATE_NAME_COUNT (sizeof detection_state_names / sizeof detection_state_names[0])

/* ---- Public API ---- */

JPBootResult jp_boot_info_summarize(const JPBootContext *context, JPBootSummary *summary)
{
	JPBootStatusState status;

	if (context == NULL || summary == NULL) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}

	/* Clear field by field: the kernel has no memset. */
	summary->architecture = JP_ARCH_UNKNOWN;
	summary->environment = JP_BOOT_ENVIRONMENT_UNKNOWN;
	summary->environment_state = JP_DETECTION_STATE_UNKNOWN;
	summary->hardware_state = JP_DETECTION_STATE_UNKNOWN;
	summary->memory_map_state = JP_DETECTION_STATE_UNKNOWN;
	summary->framebuffer_state = JP_DETECTION_STATE_UNKNOWN;
	summary->status_available = false;
	summary->status.status = JP_BOOT_STATUS_BOOTING;
	summary->status.stage = JP_DIAGNOSTIC_STAGE_UNKNOWN;
	summary->status.severity = JP_BOOT_SEVERITY_INFO;
	summary->status.recoverability = JP_BOOT_RECOVERABILITY_RECOVERABLE;
	summary->status.original_result = JP_BOOT_SUCCESS;
	summary->status.architecture = JP_ARCH_UNKNOWN;

	summary->architecture = context->handoff.architecture;
	summary->environment = context->environment.type;
	summary->environment_state = context->environment.state;
	summary->hardware_state = context->hardware.overall_state;
	summary->memory_map_state = context->memory_map.state;
	summary->framebuffer_state = context->framebuffer.state;
	summary->kernel_validation_state = context->kernel.validation_state;
	summary->kernel_load_state = context->kernel.load_state;

	if (jp_boot_status_get(context, &status) == JP_BOOT_SUCCESS) {
		summary->status = status;
		summary->status_available = true;
	}

	summary->config_state = context->stage4.config.state;
	summary->config_source = context->stage4.config.source;
	summary->selection_outcome = context->stage4.selection_outcome;
	return JP_BOOT_SUCCESS;
}

const char *jp_boot_info_state_name(JPDetectionState state)
{
	if ((unsigned int)state < JP_DETECTION_STATE_NAME_COUNT) {
		return detection_state_names[state];
	}
	return "UNKNOWN";
}
