/*
 * fake_architecture.h - fake of the architecture operations used by generic
 * Stage 1 code on the host.
 *
 * Test support only. The fake keeps one global configuration (allowed here:
 * the "no global mutable state" rule is for production code). It implements
 * jp_x86_64_* and jp_aarch64_* operations that the generic modules call:
 *   - validate_elf_machine really compares the machine number, as the
 *     contract says (62 for x86_64, 183 for AArch64);
 *   - discover_cpu returns exactly the configured result and data.
 */

#ifndef JUSTOS_TESTS_SUPPORT_FAKE_ARCHITECTURE_H
#define JUSTOS_TESTS_SUPPORT_FAKE_ARCHITECTURE_H

#include "justos/boot/architecture/aarch64.h"
#include "justos/boot/architecture/x86_64.h"
#include <stddef.h>
#include <stdint.h>

#include "justos/boot/boot_types.h"

#define JP_FAKE_ARCH_CALL_LOG_CAPACITY 16
#define JP_FAKE_PAGE_TABLE_ROOT 0x7000ULL

typedef enum JPFakeArchCall {
	JP_FAKE_CALL_SETUP_PAGING = 1,
	JP_FAKE_CALL_PREPARE_ENTRY,
	JP_FAKE_CALL_VALIDATE_ENTRY_STATE
} JPFakeArchCall;

typedef struct JPFakeArchitecture {
	JPBootResult discover_cpu_result;
	JPCpuInfo cpu;

	/* Loading. map_fail_index < 0 means "no failing segment". */
	JPBootResult map_result;
	int map_fail_index;
	unsigned int map_calls;
	size_t mapped_indexes[JP_MAX_KERNEL_SEGMENTS];
	/* The state of segment i at the moment map_segment(i) was called. */
	JPKernelSegmentState state_at_map[JP_MAX_KERNEL_SEGMENTS];
	/* The states of all segments at the moment of the latest phys_access call. */
	JPKernelSegmentState states_at_phys_access[JP_MAX_KERNEL_SEGMENTS];
	/* Test memory standing for physical memory [phys_base, phys_base + phys_size). */
	uint8_t *phys_memory;
	uint64_t phys_base;
	uint64_t phys_size;

	/* Handoff operations. setup_paging and prepare_entry write the handoff
	 * fields the contract names (page_table_root and paging_enabled; kernel
	 * entry, architecture, interrupts) only when they succeed. */
	JPBootResult setup_paging_result;
	JPBootResult prepare_entry_result;
	JPBootResult validate_entry_state_result;
	JPFakeArchCall call_log[JP_FAKE_ARCH_CALL_LOG_CAPACITY];
	size_t call_count;

	unsigned int discover_cpu_calls;
	unsigned int validate_machine_calls;
	unsigned int x86_64_calls;
	unsigned int aarch64_calls;
} JPFakeArchitecture;

extern JPFakeArchitecture jp_fake_arch;

/* Clears the configuration: CPU discovery succeeds with 4 DETECTED cores. */
void jp_fake_arch_reset(void);

#endif /* JUSTOS_TESTS_SUPPORT_FAKE_ARCHITECTURE_H */
