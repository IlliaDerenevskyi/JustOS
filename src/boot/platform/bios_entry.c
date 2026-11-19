/*
 * bios_entry.c - the 64-bit entry point of the BIOS path.
 *
 * Purpose:
 *   jp_bios_main() is started by the loader in long mode, with the address of
 *   the boot data block in RDI. It creates the contexts, chooses the BIOS
 *   provider, runs the Stage 1 pipeline, and enters the kernel.
 *
 * Responsibilities:
 *   - check the block (magic text, version, entry count) before anything else;
 *   - place the JPBootContext at 0x00400000 (1 MiB) and the JPBiosContext at
 *     0x00500000 (64 KiB), both zero-filled, inside the first 8 MiB that the
 *     provider keeps RESERVED;
 *   - fill in environment.type, environment.platform_context, and
 *     handoff.architecture;
 *   - run jp_boot_run_pipeline(); on failure write the failing result to the
 *     serial port (COM1) and halt; on success enter the kernel;
 *   - provide jp_bios_port_read8() (the processor instruction "inb").
 *
 * Limitations:
 *   The only output of this path is the serial port. The Stage 4 registration
 *   step is absent until Stage 4 exists. Built only into the BIOS loader.
 *
 * Depends on:
 *   justos/boot/boot_pipeline.h, justos/boot/platform/bios.h and bios_block.h,
 *   justos/boot/architecture/x86_64.h, and the private bios_io.h.
 *
 * Must not depend on:
 *   Anything that is not part of Stage 1.
 */

#include "justos/boot/architecture/x86_64.h"
#include "justos/boot/boot_pipeline.h"
#include "justos/boot/platform/bios.h"
#include "justos/boot/platform/bios_block.h"

#include "bios_io.h"

/* ---- Private constants ---- */

#define JP_BIOS_CONTEXT_ADDRESS 0x00400000ULL
#define JP_BIOS_CONTEXT_SIZE 0x00100000ULL        /* 1 MiB for the JPBootContext */
#define JP_BIOS_PROVIDER_ADDRESS 0x00500000ULL
#define JP_BIOS_PROVIDER_SIZE 0x00010000ULL       /* 64 KiB for the JPBiosContext */

enum {
	JP_BIOS_COM1 = 0x3F8,
	JP_BIOS_COM1_LINE_STATUS = 0x3FD,
	JP_BIOS_LINE_STATUS_TRANSMIT_EMPTY = 0x20,
	JP_BIOS_SERIAL_WAIT_LIMIT = 0x100000
};

_Static_assert(sizeof(JPBootContext) <= JP_BIOS_CONTEXT_SIZE, "JPBootContext must fit its place");

/* ---- Private helpers: ports and serial ---- */

static uint8_t port_in8(uint16_t port)
{
	uint8_t value;

	__asm__ volatile ("inb %1, %0" : "=a" (value) : "Nd" (port));
	return value;
}

static void port_out8(uint16_t port, uint8_t value)
{
	__asm__ volatile ("outb %0, %1" : : "a" (value), "Nd" (port));
}

static void serial_putc(char character)
{
	unsigned int wait;

	for (wait = 0; wait < JP_BIOS_SERIAL_WAIT_LIMIT; wait++) {
		if ((port_in8(JP_BIOS_COM1_LINE_STATUS) & JP_BIOS_LINE_STATUS_TRANSMIT_EMPTY) != 0) {
			break;
		}
	}
	port_out8(JP_BIOS_COM1, (uint8_t)character);
}

static void serial_puts(const char *text)
{
	while (*text != '\0') {
		serial_putc(*text);
		text++;
	}
}

static _Noreturn void halt(void)
{
	for (;;) {
		__asm__ volatile ("cli; hlt");
	}
}

static const char *result_name(JPBootResult result)
{
	switch (result) {
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

static const char *stage_name(JPBootStage stage)
{
	switch (stage) {
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

/* Writes zero bytes through a volatile pointer: the compiler must not turn the loop into a call of memset (there is no libc). */
static void zero_fill(uint64_t address, uint64_t size)
{
	volatile uint64_t *words = (volatile uint64_t *)(uintptr_t)address;
	uint64_t i;

	for (i = 0; i < size / sizeof(uint64_t); i++) {
		words[i] = 0;
	}
}

static bool block_is_valid(const JPBiosBlock *block)
{
	size_t i;

	for (i = 0; i < JP_BIOS_BLOCK_MAGIC_SIZE; i++) {
		if (block->magic[i] != JP_BIOS_BLOCK_MAGIC[i]) {
			return false;
		}
	}
	return block->version == JP_BIOS_BLOCK_VERSION && block->e820_count <= JP_BIOS_BLOCK_MAX_E820;
}

/* ---- The port read that bios.c needs ---- */

uint8_t jp_bios_port_read8(uint16_t port)
{
	return port_in8(port);
}

/* ---- The entry point ---- */

void jp_bios_main(const JPBiosBlock *block);

void jp_bios_main(const JPBiosBlock *block)
{
	JPBootContext *context;
	JPBiosContext *provider;
	JPBootResult result;

	if (block == NULL || !block_is_valid(block) || jp_bios_context_size() > JP_BIOS_PROVIDER_SIZE) {
		serial_putc('!');
		halt();
	}
	zero_fill(JP_BIOS_CONTEXT_ADDRESS, JP_BIOS_CONTEXT_SIZE);
	zero_fill(JP_BIOS_PROVIDER_ADDRESS, JP_BIOS_PROVIDER_SIZE);
	context = (JPBootContext *)(uintptr_t)JP_BIOS_CONTEXT_ADDRESS;
	provider = jp_bios_context_setup((void *)(uintptr_t)JP_BIOS_PROVIDER_ADDRESS, block);
	if (provider == NULL) {
		serial_putc('!');
		halt();
	}

	context->environment.type = JP_BOOT_ENVIRONMENT_BIOS;
	context->environment.platform_context = provider;
	context->handoff.architecture = JP_ARCH_X86_64;

	result = jp_boot_run_pipeline(context);
	if (result != JP_BOOT_SUCCESS) {
		serial_puts("JustOS boot failed: ");
		serial_puts(result_name(result));
		serial_puts(" at ");
		serial_puts(stage_name(context->current_stage));
		serial_puts("\r\n");
		halt();
	}
	jp_x86_64_enter_kernel(context);
}
