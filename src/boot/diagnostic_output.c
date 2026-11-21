/*
 * diagnostic_output.c - where the diagnostic text goes.
 *
 * Purpose:
 *   Sends each message to the screen (when the framebuffer is valid) and to
 *   the serial port (when the port is there). The rules are in milestone0.md,
 *   "3 Decided rules": "Where the text goes" and "The serial port in detail".
 *
 * Responsibilities:
 *   - jp_boot_diagnostic_output: validate the message, try both sinks, count
 *     failures, succeed if at least one sink took the message;
 *   - the screen: 8x16 text lines in white on black, one line per message;
 *   - the serial port: probe once, then send the characters and a line break.
 *
 * Limitations:
 *   Writes only the output state of the Stage 3 area, the framebuffer pixels,
 *   and the serial port. Never starts a panic. No allocation.
 *
 * Depends on:
 *   justos/boot/diagnostic_output.h, diagnostic_font.h, diagnostic_hw.h,
 *   arch_dispatch.h.
 *
 * Must not depend on:
 *   firmware, heap, scheduler, Stage 4, panic, recovery.
 */

#include "justos/boot/diagnostic_output.h"

#include "arch_dispatch.h"
#include "diagnostic_font.h"
#include "diagnostic_hw.h"

/* ---- Private constants ---- */

#define JP_DIAGNOSTIC_COLUMNS_MAX 80U
#define JP_DIAGNOSTIC_SERIAL_MAX_CHARS 255U
#define JP_DIAGNOSTIC_SERIAL_SPIN_LIMIT 100000U
#define JP_DIAGNOSTIC_UNPRINTABLE '?'
#define JP_DIAGNOSTIC_PIXEL_BYTES 4U
#define JP_DIAGNOSTIC_WHITE 0x00FFFFFFU
#define JP_DIAGNOSTIC_BLACK 0x00000000U

/* x86_64 UART registers (offsets from COM1). */
#define JP_UART_DATA 0U
#define JP_UART_IER 1U
#define JP_UART_FCR 2U
#define JP_UART_LCR 3U
#define JP_UART_MCR 4U
#define JP_UART_LSR 5U
#define JP_UART_SCR 7U
#define JP_UART_LCR_DLAB 0x80U
#define JP_UART_LCR_8N1 0x03U
#define JP_UART_FCR_ENABLE_CLEAR 0xC7U
#define JP_UART_MCR_DTR_RTS_OUT2 0x0BU
#define JP_UART_LSR_TX_EMPTY 0x20U
#define JP_UART_DIVISOR_115200 1U
#define JP_UART_PROBE_FIRST 0x5AU
#define JP_UART_PROBE_SECOND 0xA5U
#define JP_UART_BYTE_MASK 0xFFU

/* PL011 registers (offsets). */
#define JP_PL011_DR 0x00U
#define JP_PL011_FR 0x18U
#define JP_PL011_FR_TX_FULL 0x20U
#define JP_PL011_PERIPH_ID0 0xFE0U
#define JP_PL011_CELL_ID0 0xFF0U
#define JP_PL011_ID_STEP 4U
#define JP_PL011_ID_COUNT 4U
#define JP_PL011_ID_MASK 0xFFU

#if !defined(JP_BOOT_ONLY_X86_64)
static const uint8_t pl011_periph_id[JP_PL011_ID_COUNT] = { 0x11, 0x10, 0x04, 0x00 };
static const uint8_t pl011_periph_mask[JP_PL011_ID_COUNT] = { 0xFF, 0xFF, 0x0F, 0xFF };
static const uint8_t pl011_cell_id[JP_PL011_ID_COUNT] = { 0x0D, 0xF0, 0x05, 0xB1 };
#endif /* !JP_BOOT_ONLY_X86_64 */

/* ---- Private helpers: the message ---- */

static bool architecture_is_supported(JPArchitectureType architecture)
{
	return JP_ARCH_IS_SUPPORTED(architecture);
}

static char printable(char c)
{
	unsigned char code = (unsigned char)c;

	return code >= JP_DIAGNOSTIC_FONT_FIRST_CHAR && code <= JP_DIAGNOSTIC_FONT_LAST_CHAR ? c : JP_DIAGNOSTIC_UNPRINTABLE;
}

/* ---- Private helpers: the serial port ---- */

static uint32_t reg(uint32_t offset)
{
	return jp_diagnostic_serial_read(offset) & JP_UART_BYTE_MASK;
}

#if !defined(JP_BOOT_ONLY_AARCH64)
/* x86_64: program the port, then check that the scratch register keeps two different values. */
static bool x86_64_probe(void)
{
	jp_diagnostic_serial_write(JP_UART_IER, 0);
	jp_diagnostic_serial_write(JP_UART_LCR, JP_UART_LCR_DLAB);
	jp_diagnostic_serial_write(JP_UART_DATA, JP_UART_DIVISOR_115200);
	jp_diagnostic_serial_write(JP_UART_IER, 0);               /* with the divisor latch open this is the high byte */
	jp_diagnostic_serial_write(JP_UART_LCR, JP_UART_LCR_8N1);
	jp_diagnostic_serial_write(JP_UART_FCR, JP_UART_FCR_ENABLE_CLEAR);
	jp_diagnostic_serial_write(JP_UART_MCR, JP_UART_MCR_DTR_RTS_OUT2);

	jp_diagnostic_serial_write(JP_UART_SCR, JP_UART_PROBE_FIRST);
	if (reg(JP_UART_SCR) != JP_UART_PROBE_FIRST) {
		return false;
	}
	jp_diagnostic_serial_write(JP_UART_SCR, JP_UART_PROBE_SECOND);
	return reg(JP_UART_SCR) == JP_UART_PROBE_SECOND;
}

#endif /* !JP_BOOT_ONLY_AARCH64 */

#if !defined(JP_BOOT_ONLY_X86_64)
/* AArch64: the identification registers say "PL011". The port is used as the firmware left it. */
static bool aarch64_probe(void)
{
	unsigned int i;

	for (i = 0; i < JP_PL011_ID_COUNT; i++) {
		uint32_t periph = reg(JP_PL011_PERIPH_ID0 + i * JP_PL011_ID_STEP);
		uint32_t cell = reg(JP_PL011_CELL_ID0 + i * JP_PL011_ID_STEP);

		if ((periph & pl011_periph_mask[i]) != pl011_periph_id[i] || cell != pl011_cell_id[i]) {
			return false;
		}
	}
	return true;
}

#endif /* !JP_BOOT_ONLY_X86_64 */

#if !defined(JP_BOOT_ONLY_AARCH64)
static bool x86_64_send(uint8_t byte)
{
	unsigned int spin;

	for (spin = 0; spin < JP_DIAGNOSTIC_SERIAL_SPIN_LIMIT; spin++) {
		if ((reg(JP_UART_LSR) & JP_UART_LSR_TX_EMPTY) != 0) {
			jp_diagnostic_serial_write(JP_UART_DATA, byte);
			return true;
		}
	}
	return false;
}

#endif /* !JP_BOOT_ONLY_AARCH64 */

#if !defined(JP_BOOT_ONLY_X86_64)
static bool aarch64_send(uint8_t byte)
{
	unsigned int spin;

	for (spin = 0; spin < JP_DIAGNOSTIC_SERIAL_SPIN_LIMIT; spin++) {
		if ((reg(JP_PL011_FR) & JP_PL011_FR_TX_FULL) == 0) {
			jp_diagnostic_serial_write(JP_PL011_DR, byte);
			return true;
		}
	}
	return false;
}

#endif /* !JP_BOOT_ONLY_X86_64 */

/* Sends one message and a line break; false when a byte could not be sent. */
static bool serial_send(JPArchitectureType architecture, const JPBootDiagnosticMessage *message)
{
	size_t length = message->length < JP_DIAGNOSTIC_SERIAL_MAX_CHARS ? message->length : JP_DIAGNOSTIC_SERIAL_MAX_CHARS;
	size_t i;

	(void)architecture;     /* unused when the build is for one architecture only */
	for (i = 0; i < length; i++) {
		uint8_t byte = (uint8_t)printable(message->text[i]);

		if (!JP_ARCH_DISPATCH(architecture, x86_64_send(byte), aarch64_send(byte))) {
			return false;
		}
	}
	return JP_ARCH_DISPATCH(architecture, x86_64_send('\r'), aarch64_send('\r')) &&
	       JP_ARCH_DISPATCH(architecture, x86_64_send('\n'), aarch64_send('\n'));
}

static bool serial_sink(JPBootContext *context, const JPBootDiagnosticMessage *message)
{
	JPBootOutputState *output = &context->stage3.output;
	JPArchitectureType architecture = context->handoff.architecture;

	if (!output->serial_probed) {
		output->serial_available = JP_ARCH_DISPATCH(architecture, x86_64_probe(), aarch64_probe());
		output->serial_probed = true;
	}
	return output->serial_available && serial_send(architecture, message);
}

/* ---- Private helpers: the screen ---- */

static bool framebuffer_is_valid(const JPFramebufferInfo *fb)
{
	return fb->state == JP_DETECTION_STATE_DETECTED &&
	       fb->base_address != 0 &&
	       fb->width != 0 && fb->height != 0 && fb->pitch != 0 &&
	       (fb->format == JP_FRAMEBUFFER_FORMAT_RGB_888 || fb->format == JP_FRAMEBUFFER_FORMAT_BGR_888) &&
	       (uint64_t)fb->pitch >= (uint64_t)fb->width * JP_DIAGNOSTIC_PIXEL_BYTES &&
	       fb->pitch % JP_DIAGNOSTIC_PIXEL_BYTES == 0;
}

static void put_pixel(const JPFramebufferInfo *fb, uint32_t x, uint32_t y, uint32_t color)
{
	uint8_t *base = (uint8_t *)(uintptr_t)fb->base_address;
	uint32_t *pixel = (uint32_t *)(void *)(base + (uint64_t)y * fb->pitch + (uint64_t)x * JP_DIAGNOSTIC_PIXEL_BYTES);

	*pixel = color;
}

static bool screen_sink(JPBootContext *context, const JPBootDiagnosticMessage *message)
{
	const JPFramebufferInfo *fb = &context->framebuffer;
	JPBootOutputState *output = &context->stage3.output;
	uint32_t columns;
	uint32_t top;
	uint32_t x;
	uint32_t y;
	size_t i;

	if (!framebuffer_is_valid(fb)) {
		return false;
	}
	columns = fb->width / JP_DIAGNOSTIC_FONT_WIDTH;
	if (columns > JP_DIAGNOSTIC_COLUMNS_MAX) {
		columns = JP_DIAGNOSTIC_COLUMNS_MAX;
	}
	if (columns == 0 || (uint64_t)(output->next_line + 1U) * JP_DIAGNOSTIC_FONT_HEIGHT > fb->height) {
		return false;     /* no line left: the screen refuses this message */
	}
	top = output->next_line * JP_DIAGNOSTIC_FONT_HEIGHT;

	for (y = 0; y < JP_DIAGNOSTIC_FONT_HEIGHT; y++) {
		for (x = 0; x < columns * JP_DIAGNOSTIC_FONT_WIDTH; x++) {
			put_pixel(fb, x, top + y, JP_DIAGNOSTIC_BLACK);
		}
	}
	for (i = 0; i < message->length && i < columns; i++) {
		const uint8_t *glyph = jp_diagnostic_font8x16[(unsigned char)printable(message->text[i]) - JP_DIAGNOSTIC_FONT_FIRST_CHAR];

		for (y = 0; y < JP_DIAGNOSTIC_FONT_HEIGHT; y++) {
			for (x = 0; x < JP_DIAGNOSTIC_FONT_WIDTH; x++) {
				if ((glyph[y] & (0x80U >> x)) != 0) {
					put_pixel(fb, (uint32_t)i * JP_DIAGNOSTIC_FONT_WIDTH + x, top + y, JP_DIAGNOSTIC_WHITE);
				}
			}
		}
	}
	output->next_line++;
	return true;
}

/* ---- Public API ---- */

JPBootResult jp_boot_diagnostic_output(JPBootContext *context, const JPBootDiagnosticMessage *message)
{
	bool screen;
	bool serial;

	if (context == NULL || message == NULL || message->text == NULL ||
	    message->stage > JP_DIAGNOSTIC_STAGE_STAGE4 || message->source_component > JP_BOOT_SOURCE_BOOT_MANAGER) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	if (!context->stage3.log_initialized) {
		return JP_BOOT_NOT_INITIALIZED;
	}
	if (!architecture_is_supported(context->handoff.architecture)) {
		return JP_BOOT_FAILED;
	}

	screen = screen_sink(context, message);
	serial = serial_sink(context, message);
	if (!screen && !serial) {
		context->stage3.output.failed_messages++;
		return JP_BOOT_FAILED;
	}
	return JP_BOOT_SUCCESS;
}
