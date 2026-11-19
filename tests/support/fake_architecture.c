#include "fake_architecture.h"

#include <string.h>

enum { JP_FAKE_DEFAULT_CORES = 4 };

JPFakeArchitecture jp_fake_arch;

void jp_fake_arch_reset(void)
{
	memset(&jp_fake_arch, 0, sizeof jp_fake_arch);
	jp_fake_arch.discover_cpu_result = JP_BOOT_SUCCESS;
	jp_fake_arch.cpu.state = JP_DETECTION_STATE_DETECTED;
	jp_fake_arch.cpu.logical_cores = JP_FAKE_DEFAULT_CORES;
	jp_fake_arch.map_fail_index = -1;
}

static void log_call(JPFakeArchCall call)
{
	if (jp_fake_arch.call_count < JP_FAKE_ARCH_CALL_LOG_CAPACITY) {
		jp_fake_arch.call_log[jp_fake_arch.call_count] = call;
	}
	jp_fake_arch.call_count++;
}

static JPBootResult map_segment(JPBootContext *context, size_t index)
{
	jp_fake_arch.map_calls++;
	if (index < JP_MAX_KERNEL_SEGMENTS) {
		jp_fake_arch.state_at_map[index] = context->kernel.segments[index].state;
	}
	if (jp_fake_arch.map_fail_index >= 0 &&
	    (size_t)jp_fake_arch.map_fail_index == index) {
		return jp_fake_arch.map_result;
	}
	if (jp_fake_arch.map_calls <= JP_MAX_KERNEL_SEGMENTS) {
		jp_fake_arch.mapped_indexes[jp_fake_arch.map_calls - 1] = index;
	}
	return JP_BOOT_SUCCESS;
}

static void *phys_access(const JPBootContext *context, uint64_t physical, uint64_t length)
{
	size_t i;

	for (i = 0; i < JP_MAX_KERNEL_SEGMENTS; i++) {
		jp_fake_arch.states_at_phys_access[i] = context->kernel.segments[i].state;
	}
	if (jp_fake_arch.phys_memory == NULL ||
	    physical < jp_fake_arch.phys_base ||
	    length > jp_fake_arch.phys_size ||
	    physical - jp_fake_arch.phys_base > jp_fake_arch.phys_size - length) {
		return NULL;
	}
	return jp_fake_arch.phys_memory + (physical - jp_fake_arch.phys_base);
}

static JPBootResult setup_paging(JPBootContext *context)
{
	log_call(JP_FAKE_CALL_SETUP_PAGING);
	if (jp_fake_arch.setup_paging_result != JP_BOOT_SUCCESS) {
		return jp_fake_arch.setup_paging_result;
	}
	context->handoff.page_table_root = JP_FAKE_PAGE_TABLE_ROOT;
	context->handoff.paging_enabled = true;
	return JP_BOOT_SUCCESS;
}

static JPBootResult prepare_entry(JPBootContext *context, JPArchitectureType arch)
{
	log_call(JP_FAKE_CALL_PREPARE_ENTRY);
	if (jp_fake_arch.prepare_entry_result != JP_BOOT_SUCCESS) {
		return jp_fake_arch.prepare_entry_result;
	}
	if (context->handoff.stack_pointer == 0) {
		return JP_BOOT_FAILED;
	}
	context->handoff.kernel_entry = context->kernel.entry_point;
	context->handoff.architecture = arch;
	context->handoff.interrupts_disabled = true;
	return JP_BOOT_SUCCESS;
}

static JPBootResult validate_entry_state(void)
{
	log_call(JP_FAKE_CALL_VALIDATE_ENTRY_STATE);
	return jp_fake_arch.validate_entry_state_result;
}

static JPBootResult discover(JPCpuInfo *cpu)
{
	jp_fake_arch.discover_cpu_calls++;
	if (cpu == NULL) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	if (jp_fake_arch.discover_cpu_result != JP_BOOT_SUCCESS) {
		return jp_fake_arch.discover_cpu_result;
	}
	*cpu = jp_fake_arch.cpu;
	return JP_BOOT_SUCCESS;
}

static JPBootResult validate_machine(const JPKernelHeader *header, uint16_t machine)
{
	jp_fake_arch.validate_machine_calls++;
	if (header == NULL) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	return header->machine == machine ? JP_BOOT_SUCCESS : JP_BOOT_ERROR_CORRUPTED_DATA;
}

JPBootResult jp_x86_64_discover_cpu(JPCpuInfo *cpu)
{
	jp_fake_arch.x86_64_calls++;
	return discover(cpu);
}

JPBootResult jp_aarch64_discover_cpu(JPCpuInfo *cpu)
{
	jp_fake_arch.aarch64_calls++;
	return discover(cpu);
}

JPBootResult jp_x86_64_validate_elf_machine(const JPKernelHeader *header)
{
	jp_fake_arch.x86_64_calls++;
	return validate_machine(header, JP_ELF_MACHINE_X86_64);
}

JPBootResult jp_aarch64_validate_elf_machine(const JPKernelHeader *header)
{
	jp_fake_arch.aarch64_calls++;
	return validate_machine(header, JP_ELF_MACHINE_AARCH64);
}

JPBootResult jp_x86_64_map_segment(JPBootContext *context, size_t index)
{
	return map_segment(context, index);
}

JPBootResult jp_aarch64_map_segment(JPBootContext *context, size_t index)
{
	return map_segment(context, index);
}

void *jp_x86_64_phys_access(JPBootContext *context, uint64_t physical, uint64_t length)
{
	return phys_access(context, physical, length);
}

void *jp_aarch64_phys_access(JPBootContext *context, uint64_t physical, uint64_t length)
{
	return phys_access(context, physical, length);
}

JPBootResult jp_x86_64_setup_paging(JPBootContext *context)
{
	return setup_paging(context);
}

JPBootResult jp_aarch64_setup_paging(JPBootContext *context)
{
	return setup_paging(context);
}

JPBootResult jp_x86_64_prepare_entry(JPBootContext *context)
{
	return prepare_entry(context, JP_ARCH_X86_64);
}

JPBootResult jp_aarch64_prepare_entry(JPBootContext *context)
{
	return prepare_entry(context, JP_ARCH_AARCH64);
}

JPBootResult jp_x86_64_validate_entry_state(const JPBootContext *context)
{
	(void)context;
	return validate_entry_state();
}

JPBootResult jp_aarch64_validate_entry_state(const JPBootContext *context)
{
	(void)context;
	return validate_entry_state();
}
