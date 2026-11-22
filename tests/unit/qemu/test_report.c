/* Unit test (level U) of the test-kernel data list (tests/qemu/test_kernel/report.c). */

#include <assert.h>
#include <string.h>

#include "report.h"

#define OUT_CAPACITY 8192

typedef struct Buffer {
	char text[OUT_CAPACITY];
	size_t length;
} Buffer;

static void put(void *user, char c)
{
	Buffer *buffer = (Buffer *)user;

	assert(buffer->length + 1 < OUT_CAPACITY);
	buffer->text[buffer->length] = c;
	buffer->length++;
	buffer->text[buffer->length] = '\0';
}

static bool has_line(const Buffer *buffer, const char *line)
{
	char wanted[256];
	const char *found;
	size_t n = strlen(line);

	assert(n + 3 < sizeof wanted);
	wanted[0] = '\n';
	memcpy(wanted + 1, line, n);
	wanted[n + 1] = '\n';
	wanted[n + 2] = '\0';
	/* Prepend a newline so the first line can be found too. */
	{
		static char padded[OUT_CAPACITY + 2];

		padded[0] = '\n';
		memcpy(padded + 1, buffer->text, buffer->length + 1);
		found = strstr(padded, wanted);
	}
	return found != NULL;
}

static void make_passing_context(JPBootContext *c)
{
	memset(c, 0, sizeof *c);
	c->current_stage = JP_BOOT_STAGE_READY;
	c->stage2_result = JP_BOOT_SUCCESS;
	c->environment.type = JP_BOOT_ENVIRONMENT_UEFI;
	c->environment.state = JP_DETECTION_STATE_DETECTED;
	c->hardware.cpu.state = JP_DETECTION_STATE_DETECTED;
	c->hardware.cpu.logical_cores = 4;
	c->hardware.firmware.state = JP_DETECTION_STATE_DETECTED;
	c->hardware.boot_device.state = JP_DETECTION_STATE_DETECTED;
	c->hardware.overall_state = JP_DETECTION_STATE_DETECTED;
	c->memory_map.state = JP_DETECTION_STATE_DETECTED;
	c->memory_map.region_count = 2;
	c->memory_map.regions[0].base_address = 0;
	c->memory_map.regions[0].length = 0xA0000;
	c->memory_map.regions[0].type = JP_MEMORY_REGION_RESERVED;
	c->memory_map.regions[1].base_address = 0x100000;
	c->memory_map.regions[1].length = 0x7FF00000;
	c->memory_map.regions[1].type = JP_MEMORY_REGION_USABLE;
	c->framebuffer.state = JP_DETECTION_STATE_DETECTED;
	c->framebuffer.base_address = 0x80000000ULL;
	c->framebuffer.width = 1024;
	c->framebuffer.height = 768;
	c->framebuffer.pitch = 4096;
	c->framebuffer.format = JP_FRAMEBUFFER_FORMAT_BGR_888;
	c->kernel.validation_state = JP_KERNEL_VALIDATION_VALID;
	c->kernel.load_state = JP_KERNEL_LOAD_LOADED;
	c->kernel.segment_count = 1;
	c->kernel.segments[0].virtual_address = 0xFFFFFFFF80000000ULL;
	c->kernel.segments[0].physical_address = 0x100000;
	c->kernel.segments[0].file_size = 0x10;
	c->kernel.segments[0].memory_size = 0x1000;
	c->kernel.segments[0].flags = 5;
	c->kernel.entry_point = 0xFFFFFFFF80000000ULL;
	c->handoff.state = JP_HANDOFF_STATE_READY;
	c->handoff.architecture = JP_ARCH_X86_64;
	c->handoff.page_table_root = 0x7000;
	c->handoff.stack_pointer = 0x104000;
	c->handoff.paging_enabled = true;
	c->handoff.interrupts_disabled = true;
	c->handoff.ownership_transferred = true;
	c->interaction.state = JP_BOOT_INTERACTION_COMPLETED;
}

static bool emit(const JPBootContext *context, Buffer *buffer)
{
	JPTestSink sink = { put, buffer };

	memset(buffer, 0, sizeof *buffer);
	return jp_test_report_emit(context, &sink);
}

static void test_passing_context(void)
{
	JPBootContext c;
	Buffer b;

	make_passing_context(&c);
	assert(emit(&c, &b));
	assert(has_line(&b, "JP|machine=x86_64"));
	assert(has_line(&b, "JP|environment=UEFI"));
	assert(has_line(&b, "JP|current_stage=READY"));
	assert(has_line(&b, "JP|stage2_result=SUCCESS"));
	assert(has_line(&b, "JP|cpu.logical_cores=4"));
	assert(has_line(&b, "JP|hardware.state=DETECTED"));
	assert(has_line(&b, "JP|memory.regions=2"));
	assert(has_line(&b, "JP|memory.region.0=0x0,0xa0000,RESERVED"));
	assert(has_line(&b, "JP|memory.region.1=0x100000,0x7ff00000,USABLE"));
	assert(has_line(&b, "JP|framebuffer.mode=1024x768,pitch=4096,BGR_888,base=0x80000000"));
	assert(has_line(&b, "JP|kernel.segments=1"));
	assert(has_line(&b, "JP|kernel.segment.0=0xffffffff80000000,0x100000,0x10,0x1000,R-X"));
	assert(has_line(&b, "JP|kernel.entry=0xffffffff80000000"));
	assert(has_line(&b, "JP|handoff.state=READY"));
	assert(has_line(&b, "JP|handoff.page_table_root=0x7000"));
	assert(has_line(&b, "JP|handoff.paging_enabled=yes"));
	assert(has_line(&b, "JP|interaction.state=COMPLETED"));
	/* RESULT is the last line. */
	assert(strcmp(b.text + b.length - strlen("JP|RESULT=PASS\n"), "JP|RESULT=PASS\n") == 0);
}

static void expect_fail(JPBootContext *c, const char *key)
{
	Buffer b;
	char expected[128];
	size_t n;

	assert(!emit(c, &b));
	n = strlen("JP|RESULT=FAIL|") + strlen(key) + 1;
	assert(n < sizeof expected);
	strcpy(expected, "JP|RESULT=FAIL|");
	strcat(expected, key);
	strcat(expected, "\n");
	assert(strcmp(b.text + b.length - n, expected) == 0);
}

static void test_each_missing_condition_fails_with_its_key(void)
{
	JPBootContext c;

	make_passing_context(&c);
	c.current_stage = JP_BOOT_STAGE_HANDOFF_PREPARATION;
	expect_fail(&c, "current_stage");

	make_passing_context(&c);
	c.stage2_result = JP_BOOT_FAILED;
	expect_fail(&c, "stage2_result");

	make_passing_context(&c);
	c.hardware.overall_state = JP_DETECTION_STATE_NOT_DETECTED;
	expect_fail(&c, "hardware.state");

	make_passing_context(&c);
	c.memory_map.state = JP_DETECTION_STATE_FAILED;
	expect_fail(&c, "memory.state");

	make_passing_context(&c);
	c.kernel.load_state = JP_KERNEL_LOAD_FAILED;
	expect_fail(&c, "kernel.load");

	make_passing_context(&c);
	c.handoff.state = JP_HANDOFF_STATE_PLATFORM_PREPARED;
	expect_fail(&c, "handoff.state");

	make_passing_context(&c);
	c.handoff.paging_enabled = false;
	expect_fail(&c, "handoff.paging_enabled");

	make_passing_context(&c);
	c.handoff.interrupts_disabled = false;
	expect_fail(&c, "handoff.interrupts_disabled");

	make_passing_context(&c);
	c.handoff.ownership_transferred = false;
	expect_fail(&c, "handoff.ownership_transferred");
}

static void test_first_failing_key_in_table_order_wins(void)
{
	JPBootContext c;

	make_passing_context(&c);
	c.handoff.ownership_transferred = false;
	c.memory_map.state = JP_DETECTION_STATE_UNKNOWN;
	expect_fail(&c, "memory.state");
}

static void test_unknown_values_are_printed_as_reported(void)
{
	JPBootContext c;
	Buffer b;

	memset(&c, 0, sizeof c);
	assert(!emit(&c, &b));
	assert(has_line(&b, "JP|machine=UNKNOWN"));
	assert(has_line(&b, "JP|environment=UNKNOWN"));
	assert(has_line(&b, "JP|hardware.state=UNKNOWN"));
	assert(has_line(&b, "JP|memory.regions=0"));
	assert(has_line(&b, "JP|kernel.entry=0x0"));
	assert(has_line(&b, "JP|handoff.state=UNINITIALIZED"));
	assert(has_line(&b, "JP|handoff.paging_enabled=no"));
	assert(has_line(&b, "JP|interaction.state=NOT_REGISTERED"));
}

static void test_framebuffer_mode_only_when_detected(void)
{
	JPBootContext c;
	Buffer b;

	make_passing_context(&c);
	c.framebuffer.state = JP_DETECTION_STATE_NOT_DETECTED;
	assert(emit(&c, &b));
	assert(has_line(&b, "JP|framebuffer.state=NOT_DETECTED"));
	assert(strstr(b.text, "framebuffer.mode") == NULL);
}

static void test_counts_beyond_capacity_are_not_read_past_the_array(void)
{
	JPBootContext c;
	Buffer b;

	make_passing_context(&c);
	c.memory_map.region_count = JP_MAX_MEMORY_REGIONS + 50;
	c.kernel.segment_count = JP_MAX_KERNEL_SEGMENTS + 5;
	assert(emit(&c, &b));
	assert(has_line(&b, "JP|memory.regions=178"));
	assert(strstr(b.text, "memory.region.128=") == NULL);
	assert(strstr(b.text, "kernel.segment.16=") == NULL);
}

int main(void)
{
	test_passing_context();
	test_each_missing_condition_fails_with_its_key();
	test_first_failing_key_in_table_order_wins();
	test_unknown_values_are_printed_as_reported();
	test_framebuffer_mode_only_when_detected();
	test_counts_beyond_capacity_are_not_read_past_the_array();
	return 0;
}
