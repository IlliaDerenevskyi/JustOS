#include "report.h"

enum {
	JP_REPORT_HEX_DIGITS = 16,
	JP_REPORT_DEC_DIGITS = 20,
	JP_REPORT_NIBBLE_BITS = 4,
	JP_REPORT_NIBBLE_MASK = 0xF,
	JP_REPORT_DECIMAL_BASE = 10,
	JP_REPORT_ELF_FLAG_X = 1,
	JP_REPORT_ELF_FLAG_W = 2,
	JP_REPORT_ELF_FLAG_R = 4
};

static void put_string(const JPTestSink *sink, const char *text)
{
	while (*text != '\0') {
		sink->put_char(sink->user, *text);
		text++;
	}
}

static void put_decimal(const JPTestSink *sink, uint64_t value)
{
	char digits[JP_REPORT_DEC_DIGITS];
	size_t count = 0;

	do {
		digits[count] = (char)('0' + (value % JP_REPORT_DECIMAL_BASE));
		value /= JP_REPORT_DECIMAL_BASE;
		count++;
	} while (value != 0);
	while (count > 0) {
		count--;
		sink->put_char(sink->user, digits[count]);
	}
}

/* "0x" followed by lower-case hexadecimal digits without leading zeros. */
static void put_hex(const JPTestSink *sink, uint64_t value)
{
	char digits[JP_REPORT_HEX_DIGITS];
	size_t count = 0;

	do {
		digits[count] = "0123456789abcdef"[value & JP_REPORT_NIBBLE_MASK];
		value >>= JP_REPORT_NIBBLE_BITS;
		count++;
	} while (value != 0);
	put_string(sink, "0x");
	while (count > 0) {
		count--;
		sink->put_char(sink->user, digits[count]);
	}
}

static void put_key(const JPTestSink *sink, const char *key)
{
	put_string(sink, "JP|");
	put_string(sink, key);
	sink->put_char(sink->user, '=');
}

static void end_line(const JPTestSink *sink)
{
	sink->put_char(sink->user, '\n');
}

static void line_text(const JPTestSink *sink, const char *key, const char *value)
{
	put_key(sink, key);
	put_string(sink, value);
	end_line(sink);
}

static void line_decimal(const JPTestSink *sink, const char *key, uint64_t value)
{
	put_key(sink, key);
	put_decimal(sink, value);
	end_line(sink);
}

static void line_hex(const JPTestSink *sink, const char *key, uint64_t value)
{
	put_key(sink, key);
	put_hex(sink, value);
	end_line(sink);
}

/* Key with a number: "memory.region.<i>". */
static void put_indexed_key(const JPTestSink *sink, const char *prefix, size_t index)
{
	put_string(sink, "JP|");
	put_string(sink, prefix);
	put_decimal(sink, index);
	sink->put_char(sink->user, '=');
}

static const char *yes_no(bool value)
{
	return value ? "yes" : "no";
}

static const char *name_architecture(JPArchitectureType value)
{
	switch (value) {
	case JP_ARCH_X86_64: return "x86_64";
	case JP_ARCH_AARCH64: return "aarch64";
	case JP_ARCH_UNKNOWN: break;
	}
	return "UNKNOWN";
}

static const char *name_environment(JPBootEnvironmentType value)
{
	switch (value) {
	case JP_BOOT_ENVIRONMENT_UEFI: return "UEFI";
	case JP_BOOT_ENVIRONMENT_BIOS: return "BIOS";
	case JP_BOOT_ENVIRONMENT_UNKNOWN: break;
	}
	return "UNKNOWN";
}

static const char *name_stage(JPBootStage value)
{
	switch (value) {
	case JP_BOOT_STAGE_UNINITIALIZED: return "UNINITIALIZED";
	case JP_BOOT_STAGE_HARDWARE_DISCOVERY: return "HARDWARE_DISCOVERY";
	case JP_BOOT_STAGE_MEMORY_DISCOVERY: return "MEMORY_DISCOVERY";
	case JP_BOOT_STAGE_FRAMEBUFFER_INIT: return "FRAMEBUFFER_INIT";
	case JP_BOOT_STAGE_KERNEL_IMAGE_ACQUISITION: return "KERNEL_IMAGE_ACQUISITION";
	case JP_BOOT_STAGE_KERNEL_VALIDATION: return "KERNEL_VALIDATION";
	case JP_BOOT_STAGE_KERNEL_LOADING: return "KERNEL_LOADING";
	case JP_BOOT_STAGE_HANDOFF_PREPARATION: return "HANDOFF_PREPARATION";
	case JP_BOOT_STAGE_READY: return "READY";
	}
	return "UNKNOWN";
}

static const char *name_result(JPBootResult value)
{
	switch (value) {
	case JP_BOOT_SUCCESS: return "SUCCESS";
	case JP_BOOT_NOT_INITIALIZED: return "NOT_INITIALIZED";
	case JP_BOOT_NOT_DETECTED: return "NOT_DETECTED";
	case JP_BOOT_UNKNOWN: return "UNKNOWN";
	case JP_BOOT_FAILED: return "FAILED";
	case JP_BOOT_ERROR_INVALID_PARAM: return "ERROR_INVALID_PARAM";
	case JP_BOOT_ERROR_CORRUPTED_DATA: return "ERROR_CORRUPTED_DATA";
	}
	return "UNKNOWN";
}

static const char *name_detection(JPDetectionState value)
{
	switch (value) {
	case JP_DETECTION_STATE_UNKNOWN: return "UNKNOWN";
	case JP_DETECTION_STATE_NOT_DETECTED: return "NOT_DETECTED";
	case JP_DETECTION_STATE_NOT_INITIALIZED: return "NOT_INITIALIZED";
	case JP_DETECTION_STATE_DETECTED: return "DETECTED";
	case JP_DETECTION_STATE_FAILED: return "FAILED";
	}
	return "UNKNOWN";
}

static const char *name_region_type(JPMemoryRegionType value)
{
	switch (value) {
	case JP_MEMORY_REGION_UNKNOWN: return "UNKNOWN";
	case JP_MEMORY_REGION_USABLE: return "USABLE";
	case JP_MEMORY_REGION_RESERVED: return "RESERVED";
	case JP_MEMORY_REGION_ACPI: return "ACPI";
	case JP_MEMORY_REGION_MMIO: return "MMIO";
	case JP_MEMORY_REGION_FIRMWARE: return "FIRMWARE";
	case JP_MEMORY_REGION_UNUSABLE: return "UNUSABLE";
	}
	return "UNKNOWN";
}

static const char *name_format(JPFramebufferFormat value)
{
	switch (value) {
	case JP_FRAMEBUFFER_FORMAT_RGB_888: return "RGB_888";
	case JP_FRAMEBUFFER_FORMAT_BGR_888: return "BGR_888";
	case JP_FRAMEBUFFER_FORMAT_UNKNOWN: break;
	}
	return "UNKNOWN";
}

static const char *name_validation(JPKernelValidationState value)
{
	switch (value) {
	case JP_KERNEL_VALIDATION_UNKNOWN: return "UNKNOWN";
	case JP_KERNEL_VALIDATION_NOT_INITIALIZED: return "NOT_INITIALIZED";
	case JP_KERNEL_VALIDATION_VALID: return "VALID";
	case JP_KERNEL_VALIDATION_INVALID: return "INVALID";
	case JP_KERNEL_VALIDATION_FAILED: return "FAILED";
	}
	return "UNKNOWN";
}

static const char *name_load(JPKernelLoadState value)
{
	switch (value) {
	case JP_KERNEL_LOAD_UNKNOWN: return "UNKNOWN";
	case JP_KERNEL_LOAD_NOT_INITIALIZED: return "NOT_INITIALIZED";
	case JP_KERNEL_LOAD_LOADED: return "LOADED";
	case JP_KERNEL_LOAD_FAILED: return "FAILED";
	}
	return "UNKNOWN";
}

static const char *name_handoff(JPHandoffState value)
{
	switch (value) {
	case JP_HANDOFF_STATE_UNINITIALIZED: return "UNINITIALIZED";
	case JP_HANDOFF_STATE_PREREQUISITES_VALID: return "PREREQUISITES_VALID";
	case JP_HANDOFF_STATE_PLATFORM_PREPARED: return "PLATFORM_PREPARED";
	case JP_HANDOFF_STATE_READY: return "READY";
	case JP_HANDOFF_STATE_FAILED: return "FAILED";
	}
	return "UNKNOWN";
}

static const char *name_interaction(JPBootInteractionState value)
{
	switch (value) {
	case JP_BOOT_INTERACTION_NOT_REGISTERED: return "NOT_REGISTERED";
	case JP_BOOT_INTERACTION_REGISTERED: return "REGISTERED";
	case JP_BOOT_INTERACTION_COMPLETED: return "COMPLETED";
	case JP_BOOT_INTERACTION_FAILED: return "FAILED";
	}
	return "UNKNOWN";
}

static void emit_flags(const JPTestSink *sink, uint32_t flags)
{
	sink->put_char(sink->user, (flags & JP_REPORT_ELF_FLAG_R) != 0 ? 'R' : '-');
	sink->put_char(sink->user, (flags & JP_REPORT_ELF_FLAG_W) != 0 ? 'W' : '-');
	sink->put_char(sink->user, (flags & JP_REPORT_ELF_FLAG_X) != 0 ? 'X' : '-');
}

static void emit_memory(const JPBootContext *context, const JPTestSink *sink)
{
	size_t count = context->memory_map.region_count;
	size_t i;

	line_text(sink, "memory.state", name_detection(context->memory_map.state));
	line_decimal(sink, "memory.regions", count);
	if (count > JP_MAX_MEMORY_REGIONS) {
		count = JP_MAX_MEMORY_REGIONS;
	}
	for (i = 0; i < count; i++) {
		const JPMemoryRegion *region = &context->memory_map.regions[i];

		put_indexed_key(sink, "memory.region.", i);
		put_hex(sink, region->base_address);
		sink->put_char(sink->user, ',');
		put_hex(sink, region->length);
		sink->put_char(sink->user, ',');
		put_string(sink, name_region_type(region->type));
		end_line(sink);
	}
}

static void emit_framebuffer(const JPBootContext *context, const JPTestSink *sink)
{
	const JPFramebufferInfo *fb = &context->framebuffer;

	line_text(sink, "framebuffer.state", name_detection(fb->state));
	if (fb->state == JP_DETECTION_STATE_DETECTED) {
		put_key(sink, "framebuffer.mode");
		put_decimal(sink, fb->width);
		sink->put_char(sink->user, 'x');
		put_decimal(sink, fb->height);
		put_string(sink, ",pitch=");
		put_decimal(sink, fb->pitch);
		sink->put_char(sink->user, ',');
		put_string(sink, name_format(fb->format));
		put_string(sink, ",base=");
		put_hex(sink, fb->base_address);
		end_line(sink);
	}
}

static void emit_kernel(const JPBootContext *context, const JPTestSink *sink)
{
	size_t count = context->kernel.segment_count;
	size_t i;

	line_text(sink, "kernel.validation", name_validation(context->kernel.validation_state));
	line_text(sink, "kernel.load", name_load(context->kernel.load_state));
	line_decimal(sink, "kernel.segments", count);
	if (count > JP_MAX_KERNEL_SEGMENTS) {
		count = JP_MAX_KERNEL_SEGMENTS;
	}
	for (i = 0; i < count; i++) {
		const JPKernelSegment *segment = &context->kernel.segments[i];

		put_indexed_key(sink, "kernel.segment.", i);
		put_hex(sink, segment->virtual_address);
		sink->put_char(sink->user, ',');
		put_hex(sink, segment->physical_address);
		sink->put_char(sink->user, ',');
		put_hex(sink, segment->file_size);
		sink->put_char(sink->user, ',');
		put_hex(sink, segment->memory_size);
		sink->put_char(sink->user, ',');
		emit_flags(sink, segment->flags);
		end_line(sink);
	}
	line_hex(sink, "kernel.entry", context->kernel.entry_point);
}

/* The first key (in table order) whose PASS condition is not met, or NULL. */
static const char *first_failed_condition(const JPBootContext *context)
{
	if (context->current_stage != JP_BOOT_STAGE_READY) {
		return "current_stage";
	}
	if (context->stage2_result != JP_BOOT_SUCCESS) {
		return "stage2_result";
	}
	if (context->hardware.overall_state != JP_DETECTION_STATE_DETECTED) {
		return "hardware.state";
	}
	if (context->memory_map.state != JP_DETECTION_STATE_DETECTED) {
		return "memory.state";
	}
	if (context->kernel.load_state != JP_KERNEL_LOAD_LOADED) {
		return "kernel.load";
	}
	if (context->handoff.state != JP_HANDOFF_STATE_READY) {
		return "handoff.state";
	}
	if (!context->handoff.paging_enabled) {
		return "handoff.paging_enabled";
	}
	if (!context->handoff.interrupts_disabled) {
		return "handoff.interrupts_disabled";
	}
	if (!context->handoff.ownership_transferred) {
		return "handoff.ownership_transferred";
	}
	return NULL;
}

bool jp_test_report_emit(const JPBootContext *context, const JPTestSink *sink)
{
	const char *failed;

	line_text(sink, "machine", name_architecture(context->handoff.architecture));
	line_text(sink, "environment", name_environment(context->environment.type));
	line_text(sink, "current_stage", name_stage(context->current_stage));
	line_text(sink, "stage2_result", name_result(context->stage2_result));
	line_text(sink, "cpu.state", name_detection(context->hardware.cpu.state));
	line_text(sink, "firmware.state", name_detection(context->hardware.firmware.state));
	line_text(sink, "boot_device.state", name_detection(context->hardware.boot_device.state));
	line_text(sink, "hardware.state", name_detection(context->hardware.overall_state));
	line_decimal(sink, "cpu.logical_cores", context->hardware.cpu.logical_cores);
	emit_memory(context, sink);
	emit_framebuffer(context, sink);
	emit_kernel(context, sink);
	line_text(sink, "handoff.state", name_handoff(context->handoff.state));
	line_hex(sink, "handoff.page_table_root", context->handoff.page_table_root);
	line_hex(sink, "handoff.stack_pointer", context->handoff.stack_pointer);
	line_text(sink, "handoff.paging_enabled", yes_no(context->handoff.paging_enabled));
	line_text(sink, "handoff.interrupts_disabled", yes_no(context->handoff.interrupts_disabled));
	line_text(sink, "handoff.ownership_transferred", yes_no(context->handoff.ownership_transferred));
	line_text(sink, "interaction.state", name_interaction(context->interaction.state));

	failed = first_failed_condition(context);
	if (failed == NULL) {
		line_text(sink, "RESULT", "PASS");
		return true;
	}
	put_key(sink, "RESULT");
	put_string(sink, "FAIL|");
	put_string(sink, failed);
	end_line(sink);
	return false;
}
