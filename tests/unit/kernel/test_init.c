/*
 * Unit test (level U) of src/kernel/init.c. Every Stage 2 subsystem is a fake
 * that records the order of the calls and returns a scripted result. The
 * non-returning functions (the completion continuation and the halt) are
 * fakes that jump back into the test.
 */

#include <assert.h>
#include <setjmp.h>
#include <string.h>

#include "halt_hw.h"
#include "justos/kernel/cpu.h"
#include "justos/kernel/elf.h"
#include "justos/kernel/heap.h"
#include "justos/kernel/init.h"
#include "justos/kernel/memory.h"
#include "justos/kernel/paging.h"
#include "justos/kernel/stack.h"

/* ---- The fakes ---- */

enum { CPU, MEMORY, PAGING, STACK, HEAP, ELF, VALIDATE_MEMORY, VALIDATE_PAGING, STEPS };

static JPBootResult scripted[STEPS];
static int calls[16];
static size_t call_count;
static const JPBootContext *seen_context[16];
static unsigned int complete_calls;
static unsigned int halt_calls;
static const JPBootContext *complete_context;
static jmp_buf escape;

static JPBootResult step(int which, const JPBootContext *context)
{
	assert(call_count < 16);
	calls[call_count] = which;
	seen_context[call_count] = context;
	call_count++;
	return scripted[which];
}

JPBootResult jp_kernel_cpu_initialize(JPBootContext *context)
{
	return step(CPU, context);
}

JPBootResult jp_kernel_memory_initialize(JPBootContext *context)
{
	return step(MEMORY, context);
}

JPBootResult jp_kernel_paging_initialize(JPBootContext *context)
{
	return step(PAGING, context);
}

JPBootResult jp_kernel_stack_initialize(JPBootContext *context)
{
	return step(STACK, context);
}

JPBootResult jp_kernel_heap_initialize(JPBootContext *context)
{
	return step(HEAP, context);
}

JPBootResult jp_kernel_elf_initialize(JPBootContext *context)
{
	return step(ELF, context);
}

JPBootResult jp_kernel_memory_validate_ownership(const JPBootContext *context)
{
	return step(VALIDATE_MEMORY, context);
}

JPBootResult jp_kernel_paging_validate(JPBootContext *context)
{
	return step(VALIDATE_PAGING, context);
}

_Noreturn void jp_kernel_stage2_complete(JPBootContext *context)
{
	complete_calls++;
	complete_context = context;
	longjmp(escape, 1);
}

_Noreturn void jp_kernel_halt(void)
{
	halt_calls++;
	longjmp(escape, 2);
}

/* ---- Setup ---- */

static JPBootContext context;

static void setup(void)
{
	size_t i;

	memset(&context, 0, sizeof context);
	context.current_stage = JP_BOOT_STAGE_READY;
	context.handoff.state = JP_HANDOFF_STATE_READY;
	context.handoff.ownership_transferred = true;
	context.handoff.interrupts_disabled = true;
	context.handoff.paging_enabled = true;
	context.handoff.architecture = JP_ARCH_X86_64;
	for (i = 0; i < STEPS; i++) {
		scripted[i] = JP_BOOT_SUCCESS;
	}
	call_count = 0;
	complete_calls = 0;
	halt_calls = 0;
	complete_context = NULL;
}

/* ---- jp_kernel_init ---- */

static void test_success(void)
{
	static const int expected[] = { CPU, MEMORY, PAGING, STACK, HEAP, ELF, VALIDATE_MEMORY, VALIDATE_PAGING };
	static JPBootContext copy;
	size_t i;
	int arch;

	for (arch = 0; arch < 2; arch++) {
		setup();
		context.handoff.architecture = arch == 0 ? JP_ARCH_X86_64 : JP_ARCH_AARCH64;
		memcpy(&copy, &context, sizeof copy);
		assert(jp_kernel_init(&context) == JP_BOOT_SUCCESS);
		assert(call_count == sizeof expected / sizeof expected[0]);
		for (i = 0; i < call_count; i++) {
			assert(calls[i] == expected[i]);
			assert(seen_context[i] == &context);                 /* the same pointer everywhere */
		}
		assert(memcmp(&copy, &context, sizeof copy) == 0);       /* init writes nothing itself */
		assert(complete_calls == 0 && halt_calls == 0);           /* init never makes the transfer */
	}
}

static void test_each_failure_stops(void)
{
	static const JPBootResult codes[] = { JP_BOOT_FAILED, JP_BOOT_ERROR_INVALID_PARAM, JP_BOOT_ERROR_CORRUPTED_DATA,
		JP_BOOT_NOT_INITIALIZED, JP_BOOT_NOT_DETECTED, JP_BOOT_UNKNOWN };
	int which;
	size_t c;

	for (which = 0; which < STEPS; which++) {
		for (c = 0; c < sizeof codes / sizeof codes[0]; c++) {
			setup();
			scripted[which] = codes[c];
			assert(jp_kernel_init(&context) == codes[c]);         /* the original result, unchanged */
			assert(call_count == (size_t)which + 1);              /* and nothing after the failed step */
			assert(calls[call_count - 1] == which);
		}
	}
}

static void test_only_the_first_failure_counts(void)
{
	setup();
	scripted[PAGING] = JP_BOOT_ERROR_CORRUPTED_DATA;
	scripted[STACK] = JP_BOOT_FAILED;
	scripted[ELF] = JP_BOOT_NOT_DETECTED;
	assert(jp_kernel_init(&context) == JP_BOOT_ERROR_CORRUPTED_DATA);
	assert(call_count == 3);
}

static void test_entry_checks(void)
{
	assert(jp_kernel_init(NULL) == JP_BOOT_ERROR_INVALID_PARAM);

#define BROKEN(statement) \
	do { \
		setup(); \
		statement; \
		assert(jp_kernel_init(&context) == JP_BOOT_FAILED); \
		assert(call_count == 0); \
	} while (0)

	BROKEN(context.current_stage = JP_BOOT_STAGE_FRAMEBUFFER_INIT);
	BROKEN(context.current_stage = JP_BOOT_STAGE_UNINITIALIZED);
	BROKEN(context.handoff.state = JP_HANDOFF_STATE_PLATFORM_PREPARED);
	BROKEN(context.handoff.state = JP_HANDOFF_STATE_FAILED);
	BROKEN(context.handoff.ownership_transferred = false);
	BROKEN(context.handoff.interrupts_disabled = false);
	BROKEN(context.handoff.paging_enabled = false);
	BROKEN(context.handoff.architecture = JP_ARCH_UNKNOWN);
	BROKEN(context.handoff.architecture = (JPArchitectureType)9);
#undef BROKEN
}

/* ---- jp_kernel_stage2_failure ---- */

static int run_failure(JPBootContext *target, JPBootResult result)
{
	int outcome = setjmp(escape);

	if (outcome == 0) {
		jp_kernel_stage2_failure(target, result);
	}
	return outcome;
}

static void test_failure_sink(void)
{
	static const JPBootResult codes[] = { JP_BOOT_FAILED, JP_BOOT_ERROR_INVALID_PARAM, JP_BOOT_ERROR_CORRUPTED_DATA,
		JP_BOOT_NOT_INITIALIZED, JP_BOOT_NOT_DETECTED, JP_BOOT_UNKNOWN };
	static JPBootContext copy;
	size_t c;

	for (c = 0; c < sizeof codes / sizeof codes[0]; c++) {
		setup();
		memcpy(&copy, &context, sizeof copy);
		assert(run_failure(&context, codes[c]) == 1);              /* it went on to the completion continuation */
		assert(complete_calls == 1 && complete_context == &context);
		assert(halt_calls == 0);
		assert(context.stage2_result == codes[c]);
		/* nothing else in the context changed */
		copy.stage2_result = codes[c];
		assert(memcmp(&copy, &context, sizeof copy) == 0);
		assert(call_count == 0);                                  /* no subsystem was called */
	}
}

static void test_failure_sink_never_records_success(void)
{
	setup();
	assert(run_failure(&context, JP_BOOT_SUCCESS) == 1);
	assert(context.stage2_result == JP_BOOT_FAILED);
	assert(complete_calls == 1);
}

static void test_failure_sink_without_context(void)
{
	setup();
	assert(run_failure(NULL, JP_BOOT_FAILED) == 2);               /* the processor is stopped */
	assert(halt_calls == 1);
	assert(complete_calls == 0);
}

static void test_failure_after_init_failure(void)
{
	/* the way the stub uses them: init fails, its result goes to the sink */
	JPBootResult result;

	setup();
	scripted[HEAP] = JP_BOOT_FAILED;
	result = jp_kernel_init(&context);
	assert(result == JP_BOOT_FAILED);
	assert(run_failure(&context, result) == 1);
	assert(context.stage2_result == JP_BOOT_FAILED && complete_calls == 1);
}

int main(void)
{
	test_success();
	test_each_failure_stops();
	test_only_the_first_failure_counts();
	test_entry_checks();
	test_failure_sink();
	test_failure_sink_never_records_success();
	test_failure_sink_without_context();
	test_failure_after_init_failure();
	return 0;
}
