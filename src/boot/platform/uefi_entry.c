/*
 * uefi_entry.c - the entry point of the UEFI boot application.
 *
 * Purpose:
 *   The function that the firmware starts: it creates the contexts, chooses
 *   the UEFI provider, runs the Stage 1 pipeline, and enters the kernel.
 *
 * Responsibilities:
 *   - allocate the JPBootContext and the provider context from the firmware
 *     (loader-data pages, so that they live on after this program ends and the
 *     memory map keeps them reserved);
 *   - fill in environment.type, environment.platform_context, and
 *     handoff.architecture (the architecture this application is built for);
 *   - run jp_boot_run_pipeline(); on failure print the name of the failing
 *     result while the firmware services still work, otherwise halt;
 *   - on success enter the kernel (never returns).
 *
 * Limitations:
 *   Built only into the UEFI boot application, for x86_64 and AArch64. The
 *   Stage 4 registration step is absent until Stage 4 exists.
 *
 * Depends on:
 *   justos/boot/boot_pipeline.h, justos/boot/platform/uefi.h, the architecture
 *   header of the target, and the private uefi_defs.h.
 *
 * Must not depend on:
 *   Anything that is not part of Stage 1.
 */

#include "justos/boot/boot_pipeline.h"
#include "justos/boot/platform/uefi.h"

#if defined(__x86_64__) || defined(_M_X64)
#include "justos/boot/architecture/x86_64.h"
#define JP_ENTRY_ARCHITECTURE JP_ARCH_X86_64
#define jp_enter_kernel jp_x86_64_enter_kernel
#elif defined(__aarch64__) || defined(_M_ARM64)
#include "justos/boot/architecture/aarch64.h"
#define JP_ENTRY_ARCHITECTURE JP_ARCH_AARCH64
#define jp_enter_kernel jp_aarch64_enter_kernel
#else
#error "the UEFI boot application is built for x86_64 or AArch64 only"
#endif

#include "uefi_defs.h"

/* ---- Private constants ---- */

enum {
	JP_ENTRY_MESSAGE_CAPACITY = 120
};

/* ---- Private helpers ---- */

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

/* Allocates whole zeroed loader-data pages; returns null on failure. */
static void *allocate_zeroed(JPUefiBootServices *services, size_t bytes)
{
	uint64_t address = 0;
	JPUefiUintn pages = (bytes + JP_UEFI_PAGE_SIZE - 1) / JP_UEFI_PAGE_SIZE;
	uint8_t *memory;
	size_t i;

	if (services->allocate_pages(JP_UEFI_ALLOCATE_ANY_PAGES, JP_UEFI_MEMORY_LOADER_DATA, pages, &address) != JP_UEFI_SUCCESS) {
		return NULL;
	}
	memory = (uint8_t *)(uintptr_t)address;
	for (i = 0; i < pages * JP_UEFI_PAGE_SIZE; i++) {
		memory[i] = 0;
	}
	return memory;
}

/* Appends a text to the 16-bit message; the text is cut if it does not fit. */
static size_t append_text(JPUefiChar16 *message, size_t length, const char *text)
{
	size_t i;

	for (i = 0; text[i] != '\0' && length < JP_ENTRY_MESSAGE_CAPACITY - 4; i++) {
		message[length++] = (JPUefiChar16)text[i];
	}
	return length;
}

/* "JustOS boot failed: <result> at <stage>" with a carriage return and a line feed. */
static void print_failure(JPUefiSystemTable *system_table, JPBootResult result, JPBootStage stage)
{
	JPUefiChar16 text[JP_ENTRY_MESSAGE_CAPACITY];
	size_t length = 0;

	if (system_table->con_out == NULL || system_table->con_out->output_string == NULL) {
		return;
	}
	length = append_text(text, length, "JustOS boot failed: ");
	length = append_text(text, length, result_name(result));
	length = append_text(text, length, " at ");
	length = append_text(text, length, stage_name(stage));
	text[length++] = '\r';
	text[length++] = '\n';
	text[length] = 0;
	(void)system_table->con_out->output_string(system_table->con_out, text);
}

/* ---- The entry point ---- */

JPUefiStatus JP_UEFI_API efi_main(JPUefiHandle image_handle, JPUefiSystemTable *system_table);

JPUefiStatus JP_UEFI_API efi_main(JPUefiHandle image_handle, JPUefiSystemTable *system_table)
{
	JPUefiBootServices *services;
	JPBootContext *context;
	JPUefiContext *provider;
	JPBootResult result;

	if (system_table == NULL || system_table->boot_services == NULL) {
		return JP_UEFI_LOAD_ERROR;
	}
	services = system_table->boot_services;

	context = (JPBootContext *)allocate_zeroed(services, sizeof(JPBootContext));
	provider = (JPUefiContext *)allocate_zeroed(services, jp_uefi_context_size());
	if (context == NULL || provider == NULL) {
		return JP_UEFI_OUT_OF_RESOURCES;
	}
	if (jp_uefi_context_setup(provider, image_handle, system_table) == NULL) {
		return JP_UEFI_LOAD_ERROR;
	}

	context->environment.type = JP_BOOT_ENVIRONMENT_UEFI;
	context->environment.platform_context = provider;
	context->handoff.architecture = JP_ENTRY_ARCHITECTURE;

	result = jp_boot_run_pipeline(context);
	if (result != JP_BOOT_SUCCESS) {
		if (jp_uefi_boot_services_active(provider)) {
			print_failure(system_table, result, context->current_stage);
			return JP_UEFI_LOAD_ERROR;
		}
		/* The services are gone: nothing can be shown. enter_kernel halts a context that is not READY. */
	}
	jp_enter_kernel(context);
}
