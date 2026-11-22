/*
 * Unit test (level U) of src/boot/diagnostic_output.c: the choice of the
 * sinks, the screen drawing, the serial port protocol on both architectures,
 * and every refusal. The serial port registers are a fake of the hardware
 * functions of diagnostic_hw.h; the framebuffer is a buffer.
 */

#include <assert.h>
#include <string.h>

#include "diagnostic_font.h"
#include "diagnostic_hw.h"
#include "justos/boot/boot_log.h"
#include "justos/boot/diagnostic_output.h"

/* ---- The fake serial port ---- */

static JPArchitectureType fake_arch;
static bool port_present;
static bool tx_never_ready;
static bool scr_stuck;                     /* x86_64: the scratch register does not keep what is written */
static unsigned int port_reads;
static unsigned int port_writes;
static uint8_t sent[2048];
static size_t sent_count;

/* x86_64 UART */
static uint8_t u_ier, u_fcr, u_lcr, u_mcr, u_dll, u_dlm, u_scr;
static unsigned int u_config_writes;
/* PL011 */
static uint32_t pl_id[8];
static bool pl_other_write;

static void reset_port(void)
{
	static const uint32_t good[8] = { 0x11, 0x10, 0x14, 0x00, 0x0D, 0xF0, 0x05, 0xB1 };

	port_present = true;
	tx_never_ready = false;
	scr_stuck = false;
	port_reads = port_writes = 0;
	sent_count = 0;
	u_ier = u_fcr = u_lcr = u_mcr = u_dll = u_dlm = u_scr = 0;
	u_config_writes = 0;
	memcpy(pl_id, good, sizeof good);
	pl_other_write = false;
}

uint32_t jp_diagnostic_serial_read(uint32_t offset)
{
	port_reads++;
	if (fake_arch == JP_ARCH_X86_64) {
		if (!port_present) {
			return 0xFF;                       /* a floating bus */
		}
		switch (offset) {
		case 5: return tx_never_ready ? 0x00 : 0x60;      /* LSR: bit 5 = transmitter empty */
		case 7: return u_scr;
		case 3: return u_lcr;
		default: return 0;
		}
	}
	if (!port_present) {
		return 0;
	}
	switch (offset) {
	case 0x18: return tx_never_ready ? 0x20 : 0x90;       /* FR: bit 5 = transmit FIFO full */
	case 0xFE0: return pl_id[0];
	case 0xFE4: return pl_id[1];
	case 0xFE8: return pl_id[2];
	case 0xFEC: return pl_id[3];
	case 0xFF0: return pl_id[4];
	case 0xFF4: return pl_id[5];
	case 0xFF8: return pl_id[6];
	case 0xFFC: return pl_id[7];
	default: return 0;
	}
}

void jp_diagnostic_serial_write(uint32_t offset, uint32_t value)
{
	port_writes++;
	if (fake_arch == JP_ARCH_X86_64) {
		bool dlab = (u_lcr & 0x80) != 0;

		if (offset == 0 && !dlab) {
			assert(sent_count < sizeof sent);
			sent[sent_count++] = (uint8_t)value;
			return;
		}
		u_config_writes++;
		switch (offset) {
		case 0: u_dll = (uint8_t)value; break;        /* divisor low (DLAB) */
		case 1: if (dlab) { u_dlm = (uint8_t)value; } else { u_ier = (uint8_t)value; } break;
		case 2: u_fcr = (uint8_t)value; break;
		case 3: u_lcr = (uint8_t)value; break;
		case 4: u_mcr = (uint8_t)value; break;
		case 7: if (!scr_stuck) { u_scr = (uint8_t)value; } break;
		default: break;
		}
		return;
	}
	if (offset == 0) {
		assert(sent_count < sizeof sent);
		sent[sent_count++] = (uint8_t)value;
	} else {
		pl_other_write = true;
	}
}

_Noreturn void jp_diagnostic_halt(void)
{
	assert(0);
	for (;;) {
	}
}

/* ---- The context and the screen ---- */

#define FB_WIDTH 128
#define FB_HEIGHT 64
#define FB_PITCH (FB_WIDTH * 4 + 32)
#define GUARD 64

static JPBootContext context;
static uint8_t screen_memory[GUARD + FB_PITCH * (FB_HEIGHT + 4) + GUARD];

static uint8_t *pixels(void)
{
	return screen_memory + GUARD;
}

static void setup(JPArchitectureType architecture, bool with_screen)
{
	memset(&context, 0, sizeof context);
	memset(screen_memory, 0xAB, sizeof screen_memory);
	reset_port();
	fake_arch = architecture;
	context.current_stage = JP_BOOT_STAGE_READY;
	context.handoff.state = JP_HANDOFF_STATE_READY;
	context.handoff.architecture = architecture;
	assert(jp_boot_log_initialize(&context) == JP_BOOT_SUCCESS);
	if (with_screen) {
		context.framebuffer.state = JP_DETECTION_STATE_DETECTED;
		context.framebuffer.base_address = (uint64_t)(uintptr_t)pixels();
		context.framebuffer.width = FB_WIDTH;
		context.framebuffer.height = FB_HEIGHT;
		context.framebuffer.pitch = FB_PITCH;
		context.framebuffer.format = JP_FRAMEBUFFER_FORMAT_BGR_888;
	}
}

static JPBootDiagnosticMessage message(const char *text)
{
	JPBootDiagnosticMessage m;

	memset(&m, 0, sizeof m);
	m.stage = JP_DIAGNOSTIC_STAGE_STAGE3;
	m.source_component = JP_BOOT_SOURCE_PANIC;
	m.text = text;
	m.length = strlen(text);
	return m;
}

static JPBootResult out(const char *text)
{
	JPBootDiagnosticMessage m = message(text);

	return jp_boot_diagnostic_output(&context, &m);
}

static uint32_t pixel(unsigned int x, unsigned int y)
{
	uint32_t value;

	memcpy(&value, pixels() + (size_t)y * FB_PITCH + (size_t)x * 4, sizeof value);
	return value;
}

/* The glyph of `c` as the screen shows it at the cell (column, line). */
static void expect_glyph(unsigned int column, unsigned int line, char c)
{
	unsigned int y;
	unsigned int x;
	unsigned char code = (unsigned char)c;

	if (code < JP_DIAGNOSTIC_FONT_FIRST_CHAR || code > JP_DIAGNOSTIC_FONT_LAST_CHAR) {
		code = '?';
	}
	for (y = 0; y < JP_DIAGNOSTIC_FONT_HEIGHT; y++) {
		uint8_t row = jp_diagnostic_font8x16[code - JP_DIAGNOSTIC_FONT_FIRST_CHAR][y];

		for (x = 0; x < JP_DIAGNOSTIC_FONT_WIDTH; x++) {
			uint32_t want = (row & (0x80u >> x)) != 0 ? 0x00FFFFFFu : 0x00000000u;

			assert((pixel(column * 8 + x, line * 16 + y) & 0x00FFFFFFu) == want);
		}
	}
}

static void expect_screen_untouched_from_line(unsigned int line)
{
	size_t i;

	for (i = (size_t)line * 16 * FB_PITCH; i < (size_t)FB_PITCH * (FB_HEIGHT + 4); i++) {
		assert(pixels()[i] == 0xAB);
	}
	for (i = 0; i < GUARD; i++) {
		assert(screen_memory[i] == 0xAB);
		assert(screen_memory[GUARD + FB_PITCH * (FB_HEIGHT + 4) + i] == 0xAB);
	}
}

/* ---- Serial: x86_64 ---- */

static void test_x86_serial(void)
{
	setup(JP_ARCH_X86_64, false);
	assert(out("Hello") == JP_BOOT_SUCCESS);
	/* the port was programmed: 115200 baud (divisor 1), 8N1, FIFO on, interrupts off */
	assert(u_dll == 1 && u_dlm == 0 && u_lcr == 0x03 && u_fcr == 0xC7 && u_mcr == 0x0B && u_ier == 0);
	assert(context.stage3.output.serial_probed && context.stage3.output.serial_available);
	assert(sent_count == 7 && memcmp(sent, "Hello\r\n", 7) == 0);

	/* the second message does not program the port again */
	{
		unsigned int config_before = u_config_writes;

		assert(out("again") == JP_BOOT_SUCCESS);
		assert(u_config_writes == config_before);
		assert(sent_count == 14 && memcmp(sent + 7, "again\r\n", 7) == 0);
	}
}

static void test_x86_probe_failure(void)
{
	unsigned int reads;

	setup(JP_ARCH_X86_64, false);
	port_present = false;
	assert(out("lost") == JP_BOOT_FAILED);
	assert(context.stage3.output.serial_probed && !context.stage3.output.serial_available);
	assert(context.stage3.output.failed_messages == 1 && sent_count == 0);
	/* an unavailable port never receives a byte, and is not probed again */
	reads = port_reads;
	port_writes = 0;
	assert(out("still lost") == JP_BOOT_FAILED);
	assert(port_reads == reads && port_writes == 0 && context.stage3.output.failed_messages == 2);

	/* a port whose scratch register does not keep the value: not a UART */
	setup(JP_ARCH_X86_64, false);
	scr_stuck = true;
	assert(out("x") == JP_BOOT_FAILED);
	assert(!context.stage3.output.serial_available && sent_count == 0);
}

/* ---- Serial: AArch64 ---- */

static void test_arm_serial(void)
{
	setup(JP_ARCH_AARCH64, false);
	assert(out("Hello") == JP_BOOT_SUCCESS);
	assert(sent_count == 7 && memcmp(sent, "Hello\r\n", 7) == 0);
	assert(!pl_other_write);                  /* the port is used as the firmware left it */
	assert(port_writes == 7);
	assert(context.stage3.output.serial_probed && context.stage3.output.serial_available);
}

static void test_arm_probe_failure(void)
{
	size_t i;
	unsigned int reads;

	for (i = 0; i < 8; i++) {
		setup(JP_ARCH_AARCH64, false);
		pl_id[i] ^= 0x01;                      /* one bit of one identification byte is wrong */
		assert(out("x") == JP_BOOT_FAILED);
		assert(!context.stage3.output.serial_available && sent_count == 0 && port_writes == 0);
	}
	/* the bits that the rule ignores: the high four bits of PeriphID2 (the revision), the high bytes of every register */
	setup(JP_ARCH_AARCH64, false);
	pl_id[2] = 0x34;
	pl_id[0] |= 0xFFFFFF00u;
	pl_id[7] |= 0xAA00AA00u;
	assert(out("x") == JP_BOOT_SUCCESS);

	setup(JP_ARCH_AARCH64, false);
	port_present = false;
	assert(out("a") == JP_BOOT_FAILED);
	reads = port_reads;
	assert(out("b") == JP_BOOT_FAILED);
	assert(port_reads == reads);
}

/* ---- Serial text ---- */

static void test_serial_text(void)
{
	char text[400];
	int arch;

	for (arch = 0; arch < 2; arch++) {
		setup(arch == 0 ? JP_ARCH_X86_64 : JP_ARCH_AARCH64, false);
		/* printable ASCII goes through; everything else is sent as '?' */
		assert(out(" ~!") == JP_BOOT_SUCCESS);
		assert(memcmp(sent, " ~!\r\n", 5) == 0);
		sent_count = 0;
		{
			JPBootDiagnosticMessage m = message("a\x1b[2Jb\x7f\x80\xffz");

			assert(jp_boot_diagnostic_output(&context, &m) == JP_BOOT_SUCCESS);
			assert(sent_count == 12 && memcmp(sent, "a?[2Jb???z\r\n", 12) == 0);
		}
		/* a message of 255 characters is sent whole, longer ones are cut at 255 */
		memset(text, 'x', sizeof text);
		text[255] = '\0';
		sent_count = 0;
		assert(out(text) == JP_BOOT_SUCCESS);
		assert(sent_count == 257);
		text[256] = '\0';
		text[255] = 'y';
		sent_count = 0;
		assert(out(text) == JP_BOOT_SUCCESS);
		assert(sent_count == 257 && sent[254] == 'x');             /* the 256th character is not sent */
		sent_count = 0;
		/* an empty message is a line break */
		assert(out("") == JP_BOOT_SUCCESS);
		assert(sent_count == 2 && sent[0] == '\r' && sent[1] == '\n');
	}
}

static void test_serial_transmitter_never_ready(void)
{
	int arch;

	for (arch = 0; arch < 2; arch++) {
		setup(arch == 0 ? JP_ARCH_X86_64 : JP_ARCH_AARCH64, false);
		tx_never_ready = true;
		assert(out("stuck") == JP_BOOT_FAILED);
		assert(sent_count == 0 && context.stage3.output.failed_messages == 1);
		/* the port stays "available": it was probed fine; the next message tries again */
		tx_never_ready = false;
		assert(out("ok") == JP_BOOT_SUCCESS);
		assert(sent_count == 4);
	}
}

/* ---- The screen ---- */

static void test_screen_drawing(void)
{
	int arch;

	for (arch = 0; arch < 2; arch++) {
		setup(arch == 0 ? JP_ARCH_X86_64 : JP_ARCH_AARCH64, true);
		assert(out("AB~") == JP_BOOT_SUCCESS);
		expect_glyph(0, 0, 'A');
		expect_glyph(1, 0, 'B');
		expect_glyph(2, 0, '~');
		/* the line is 16 rows of black as wide as the screen's columns (16 columns of 8 pixels), nothing more */
		{
			unsigned int x;
			unsigned int y;

			for (y = 0; y < 16; y++) {
				for (x = 3 * 8; x < FB_WIDTH; x++) {
					assert((pixel(x, y) & 0x00FFFFFFu) == 0);
				}
			}
		}
		assert(context.stage3.output.next_line == 1);
		assert(sent_count == 5);                 /* and the serial port got it too */
		expect_screen_untouched_from_line(1);    /* a glyph never reaches into the next line */
		/* the second message takes the next line; old content is not drawn over */
		assert(out("z") == JP_BOOT_SUCCESS);
		expect_glyph(0, 1, 'z');
		expect_glyph(0, 0, 'A');
		assert(context.stage3.output.next_line == 2);
		/* the padding between the pixels and the pitch is not written, nor anything after the last used line */
		{
			unsigned int y;
			unsigned int x;

			for (y = 0; y < 32; y++) {
				for (x = FB_WIDTH * 4; x < FB_PITCH; x++) {
					assert(pixels()[(size_t)y * FB_PITCH + x] == 0xAB);
				}
			}
		}
		expect_screen_untouched_from_line(2);
	}
}

static void test_screen_non_printable_and_columns(void)
{
	/* width 128 gives 16 columns: a longer message is cut on the screen, not on the serial port */
	setup(JP_ARCH_X86_64, true);
	assert(out("0123456789ABCDEFGHIJ") == JP_BOOT_SUCCESS);
	expect_glyph(15, 0, 'F');
	assert(sent_count == 22);                            /* the serial port got all 20 characters */
	{
		/* nothing is drawn past the last column: the padding of the row is untouched */
		unsigned int y;
		unsigned int k;

		for (y = 0; y < 16; y++) {
			for (k = FB_WIDTH * 4; k < FB_PITCH; k++) {
				assert(pixels()[(size_t)y * FB_PITCH + k] == 0xAB);
			}
		}
	}
	/* control characters are drawn as '?' */
	assert(out("a\tb\x01") == JP_BOOT_SUCCESS);
	expect_glyph(0, 1, 'a');
	expect_glyph(1, 1, '\t');
	expect_glyph(2, 1, 'b');
	expect_glyph(3, 1, '\x01');
}

static void test_screen_lines_run_out(void)
{
	unsigned int i;
	JPBootResult result;

	/* 64 pixels of height are 4 lines */
	setup(JP_ARCH_X86_64, true);
	for (i = 0; i < 4; i++) {
		assert(out("line") == JP_BOOT_SUCCESS);
	}
	assert(context.stage3.output.next_line == 4);
	/* the fifth message: no line left, the screen refuses it, the serial port takes it */
	sent_count = 0;
	assert(out("fifth") == JP_BOOT_SUCCESS);
	assert(sent_count == 7 && context.stage3.output.next_line == 4);
	assert(context.stage3.output.failed_messages == 0);
	expect_screen_untouched_from_line(4);

	/* when the serial port is also unavailable, the message has no sink at all */
	setup(JP_ARCH_X86_64, true);
	port_present = false;
	for (i = 0; i < 4; i++) {
		assert(out("line") == JP_BOOT_SUCCESS);       /* the screen takes it */
	}
	result = out("fifth");
	assert(result == JP_BOOT_FAILED && context.stage3.output.failed_messages == 1);

	/* a screen that is not a multiple of 16 high: the partial line at the bottom is not used */
	setup(JP_ARCH_X86_64, true);
	context.framebuffer.height = 40;                   /* two lines (32 pixels) */
	assert(out("1") == JP_BOOT_SUCCESS);
	assert(out("2") == JP_BOOT_SUCCESS);
	sent_count = 0;
	assert(out("3") == JP_BOOT_SUCCESS);
	assert(context.stage3.output.next_line == 2);
	expect_screen_untouched_from_line(2);
}

static void test_screen_validity(void)
{
	int reason;

	for (reason = 0; reason < 9; reason++) {
		setup(JP_ARCH_X86_64, true);
		switch (reason) {
		case 0: context.framebuffer.state = JP_DETECTION_STATE_NOT_DETECTED; break;
		case 1: context.framebuffer.state = JP_DETECTION_STATE_UNKNOWN; break;
		case 2: context.framebuffer.base_address = 0; break;
		case 3: context.framebuffer.width = 0; break;
		case 4: context.framebuffer.height = 0; break;
		case 5: context.framebuffer.pitch = 0; break;
		case 6: context.framebuffer.format = JP_FRAMEBUFFER_FORMAT_UNKNOWN; break;
		case 7: context.framebuffer.pitch = FB_WIDTH * 4 - 4; break;      /* shorter than a row of pixels */
		default: context.framebuffer.pitch = FB_PITCH + 2; break;         /* not a multiple of 4 */
		}
		assert(out("hi") == JP_BOOT_SUCCESS);             /* the serial port still works */
		assert(context.stage3.output.next_line == 0);
		expect_screen_untouched_from_line(0);
		assert(sent_count == 4);
	}
	/* the valid edge: pitch exactly width * 4, RGB format */
	setup(JP_ARCH_AARCH64, true);
	context.framebuffer.pitch = FB_WIDTH * 4;
	context.framebuffer.format = JP_FRAMEBUFFER_FORMAT_RGB_888;
	assert(out("hi") == JP_BOOT_SUCCESS);
	assert(context.stage3.output.next_line == 1);
}

static void test_wide_screen_has_80_columns(void)
{
	/* a screen of 1000 pixels would have 125 columns; only 80 are used */
	static uint8_t big[1000 * 4 * 16 + 64];

	setup(JP_ARCH_X86_64, false);
	memset(big, 0xAB, sizeof big);
	context.framebuffer.state = JP_DETECTION_STATE_DETECTED;
	context.framebuffer.base_address = (uint64_t)(uintptr_t)big;
	context.framebuffer.width = 1000;
	context.framebuffer.height = 16;
	context.framebuffer.pitch = 1000 * 4;
	context.framebuffer.format = JP_FRAMEBUFFER_FORMAT_BGR_888;
	{
		char text[100];

		memset(text, 'W', sizeof text - 1);
		text[sizeof text - 1] = '\0';
		assert(out(text) == JP_BOOT_SUCCESS);
	}
	/* the pixel at column 80 (x = 640) is untouched, the one at x = 639 is black or white (drawn) */
	{
		uint32_t before;
		uint32_t after;

		memcpy(&before, big + 639 * 4, 4);
		memcpy(&after, big + 640 * 4, 4);
		assert(before == 0x00000000u || before == 0x00FFFFFFu);
		assert(after == 0xABABABABu);
	}
}

/* ---- Messages and state ---- */

static void test_invalid_messages(void)
{
	JPBootDiagnosticMessage m;
	static JPBootContext copy;

	setup(JP_ARCH_X86_64, true);
	m = message("x");
	assert(jp_boot_diagnostic_output(NULL, &m) == JP_BOOT_ERROR_INVALID_PARAM);
	assert(jp_boot_diagnostic_output(&context, NULL) == JP_BOOT_ERROR_INVALID_PARAM);
	memcpy(&copy, &context, sizeof copy);
#define BAD(statement) \
	do { m = message("x"); statement; \
		assert(jp_boot_diagnostic_output(&context, &m) == JP_BOOT_ERROR_INVALID_PARAM); \
		assert(memcmp(&copy, &context, sizeof copy) == 0 && sent_count == 0); } while (0)
	BAD(m.text = NULL);
	BAD(m.stage = (JPDiagnosticStage)5);
	BAD(m.source_component = (JPBootSourceComponent)14);
#undef BAD
	assert(port_writes == 0 && port_reads == 0);

	/* before the log: nothing is written anywhere */
	setup(JP_ARCH_X86_64, true);
	context.stage3.log_initialized = false;
	assert(out("x") == JP_BOOT_NOT_INITIALIZED);
	assert(port_writes == 0);
	setup(JP_ARCH_X86_64, true);
	context.handoff.architecture = JP_ARCH_UNKNOWN;
	assert(out("x") == JP_BOOT_FAILED);
	assert(port_writes == 0);
}

static void test_extreme_valid_tags(void)
{
	JPBootDiagnosticMessage m;

	setup(JP_ARCH_X86_64, false);
	m = message("x");
	m.stage = JP_DIAGNOSTIC_STAGE_STAGE4;
	m.source_component = JP_BOOT_SOURCE_BOOT_MANAGER;
	assert(jp_boot_diagnostic_output(&context, &m) == JP_BOOT_SUCCESS);
	m.stage = JP_DIAGNOSTIC_STAGE_UNKNOWN;
	m.source_component = JP_BOOT_SOURCE_UNKNOWN;
	assert(jp_boot_diagnostic_output(&context, &m) == JP_BOOT_SUCCESS);
}

static void test_only_the_output_state_changes(void)
{
	static JPBootContext before;
	JPBootStage3Area keep;

	setup(JP_ARCH_AARCH64, true);
	memcpy(&before, &context, sizeof before);
	assert(out("hello") == JP_BOOT_SUCCESS);
	keep = context.stage3;
	/* everything outside the output state of the Stage 3 area is as it was */
	context.stage3.output = before.stage3.output;
	assert(memcmp(&before, &context, sizeof before) == 0);
	(void)keep;
	assert(before.stage3.output.next_line == 0);
}

int main(void)
{
	test_x86_serial();
	test_x86_probe_failure();
	test_arm_serial();
	test_arm_probe_failure();
	test_serial_text();
	test_serial_transmitter_never_ready();
	test_screen_drawing();
	test_screen_non_printable_and_columns();
	test_screen_lines_run_out();
	test_screen_validity();
	test_wide_screen_has_80_columns();
	test_invalid_messages();
	test_extreme_valid_tags();
	test_only_the_output_state_changes();
	return 0;
}
