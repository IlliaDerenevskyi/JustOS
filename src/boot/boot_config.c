/*
 * boot_config.c - the boot configuration (Stage 4).
 *
 * Purpose:
 *   Implements the Boot Configuration Contract (milestone0.md, "4 Boot
 *   Configuration Contract" and file specifications 1 and 2).
 *
 * Responsibilities:
 *   Set up the default configuration, check a candidate, apply it (all or
 *   nothing), freeze, and copy the configuration out.
 *
 * Limitations:
 *   Writes only context->stage4.config. After FROZEN nothing is written. The
 *   code copies field by field and byte by byte: the kernel has no memcpy or
 *   memset (the compiler must not be made to emit them).
 *
 * Depends on:
 *   justos/boot/boot_config.h.
 *
 * Must not depend on:
 *   firmware, Stage 3, the other Stage 4 modules, the heap.
 */

#include "justos/boot/boot_config.h"

/* ---- Private constants ---- */

#define JP_CONFIG_FIRST_PRINTABLE 0x20U     /* the space */
#define JP_CONFIG_LAST_PRINTABLE 0x7EU      /* the tilde */

/* ---- Private helpers ---- */

static bool is_printable(char c)
{
	unsigned char u = (unsigned char)c;

	return u >= JP_CONFIG_FIRST_PRINTABLE && u <= JP_CONFIG_LAST_PRINTABLE;
}

/* The text ends with a NUL byte inside its capacity and holds only printable characters; no_space also forbids the space. */
static bool text_is_valid(const char *text, size_t capacity, bool no_space)
{
	for (size_t i = 0; i < capacity; i++) {
		if (text[i] == '\0') {
			return true;
		}
		if (!is_printable(text[i]) || (no_space && text[i] == ' ')) {
			return false;
		}
	}
	return false;     /* no end marker */
}

static void clear_text(char *text, size_t capacity)
{
	for (size_t i = 0; i < capacity; i++) {
		text[i] = '\0';
	}
}

static void copy_text(char *to, const char *from, size_t capacity)
{
	for (size_t i = 0; i < capacity; i++) {
		to[i] = from[i];
	}
}

static bool text_is_empty(const char *text, size_t capacity)
{
	for (size_t i = 0; i < capacity; i++) {
		if (text[i] != '\0') {
			return false;
		}
	}
	return true;
}

static bool same_id(const char *a, const char *b)
{
	for (size_t i = 0; i < JP_BOOT_CONFIG_ENTRY_ID_CAPACITY; i++) {
		if (a[i] != b[i]) {
			return false;
		}
		if (a[i] == '\0') {
			return true;
		}
	}
	return true;
}

static bool entry_is_unused(const JPBootConfigEntry *entry)
{
	return text_is_empty(entry->id, sizeof entry->id) &&
	       text_is_empty(entry->parameters, sizeof entry->parameters) &&
	       !entry->enabled;
}

/* The rules of validity, in the order of the specification. The candidate is not touched. */
static bool candidate_is_valid(const JPBootConfig *candidate)
{
	if (!text_is_valid(candidate->kernel_parameters, sizeof candidate->kernel_parameters, false)) {
		return false;
	}
	if (candidate->entry_count > JP_BOOT_CONFIG_ENTRY_CAPACITY) {
		return false;
	}
	for (size_t i = 0; i < candidate->entry_count; i++) {
		const JPBootConfigEntry *entry = &candidate->entries[i];

		if (entry->id[0] == '\0' || !text_is_valid(entry->id, sizeof entry->id, true) ||
		    !text_is_valid(entry->parameters, sizeof entry->parameters, false)) {
			return false;
		}
		for (size_t earlier = 0; earlier < i; earlier++) {
			if (same_id(entry->id, candidate->entries[earlier].id)) {
				return false;
			}
		}
	}
	for (size_t i = candidate->entry_count; i < JP_BOOT_CONFIG_ENTRY_CAPACITY; i++) {
		if (!entry_is_unused(&candidate->entries[i])) {
			return false;
		}
	}
	return true;
}

static void copy_entry(JPBootConfigEntry *to, const JPBootConfigEntry *from)
{
	copy_text(to->id, from->id, sizeof to->id);
	copy_text(to->parameters, from->parameters, sizeof to->parameters);
	to->enabled = from->enabled;
}

/* The editable contents: everything except the state and the source. */
static void copy_editable(JPBootConfig *to, const JPBootConfig *from)
{
	copy_text(to->kernel_parameters, from->kernel_parameters, sizeof to->kernel_parameters);
	to->boot_logging = from->boot_logging;
	to->verbose_output = from->verbose_output;
	to->safe_mode = from->safe_mode;
	to->recovery_requested = from->recovery_requested;
	to->entry_count = from->entry_count;
	for (size_t i = 0; i < JP_BOOT_CONFIG_ENTRY_CAPACITY; i++) {
		copy_entry(&to->entries[i], &from->entries[i]);
	}
}

/* ---- Public API ---- */

JPBootResult jp_boot_config_initialize(JPBootContext *context)
{
	JPBootConfig *config;

	if (context == NULL) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	config = &context->stage4.config;
	if (config->state != JP_BOOT_CONFIG_STATE_UNSET) {
		return JP_BOOT_FAILED;
	}
	clear_text(config->kernel_parameters, sizeof config->kernel_parameters);
	config->boot_logging = true;
	config->verbose_output = false;
	config->safe_mode = false;
	config->recovery_requested = false;
	config->entry_count = 0;
	for (size_t i = 0; i < JP_BOOT_CONFIG_ENTRY_CAPACITY; i++) {
		clear_text(config->entries[i].id, sizeof config->entries[i].id);
		clear_text(config->entries[i].parameters, sizeof config->entries[i].parameters);
		config->entries[i].enabled = false;
	}
	config->source = JP_BOOT_CONFIG_SOURCE_DEFAULT;
	config->state = JP_BOOT_CONFIG_STATE_VALIDATED;     /* last: the configuration is complete now */
	return JP_BOOT_SUCCESS;
}

JPBootResult jp_boot_config_apply(JPBootContext *context, const JPBootConfig *candidate)
{
	if (context == NULL || candidate == NULL) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	if (context->stage4.config.state != JP_BOOT_CONFIG_STATE_VALIDATED) {
		return JP_BOOT_FAILED;
	}
	if (!candidate_is_valid(candidate)) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	copy_editable(&context->stage4.config, candidate);
	context->stage4.config.source = JP_BOOT_CONFIG_SOURCE_INTERACTIVE;
	return JP_BOOT_SUCCESS;
}

JPBootResult jp_boot_config_freeze(JPBootContext *context)
{
	if (context == NULL) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	if (context->stage4.config.state == JP_BOOT_CONFIG_STATE_UNSET) {
		return JP_BOOT_FAILED;
	}
	context->stage4.config.state = JP_BOOT_CONFIG_STATE_FROZEN;
	return JP_BOOT_SUCCESS;
}

JPBootResult jp_boot_config_get(const JPBootContext *context, JPBootConfig *config)
{
	if (context == NULL || config == NULL) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	if (context->stage4.config.state == JP_BOOT_CONFIG_STATE_UNSET) {
		return JP_BOOT_NOT_INITIALIZED;
	}
	copy_editable(config, &context->stage4.config);
	config->state = context->stage4.config.state;
	config->source = context->stage4.config.source;
	return JP_BOOT_SUCCESS;
}
