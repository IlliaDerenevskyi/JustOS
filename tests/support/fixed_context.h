/*
 * fixed_context.h - puts the boot context of a host test at a fixed, known address.
 *
 * Tests only. Some tests (test_memory, test_paging) use the address of the boot context as a "physical" address
 * and list it as one region of a fake memory map. The code under test keeps physical addresses below 2^47 and
 * in ascending order. The address of an ordinary global depends on the host: about 2^46 on x86_64 Linux, but
 * about 2^47.6 on AArch64 Linux, so the same test passed on one and failed on the other. This helper maps the
 * context at JP_TEST_CONTEXT_ADDRESS (1 TiB: above every fake region of those tests, below 2^47), on every host.
 *
 * The caller defines _GNU_SOURCE before any include (MAP_ANONYMOUS and MAP_FIXED_NOREPLACE need it with -std=c17).
 */

#ifndef JUSTOS_TESTS_SUPPORT_FIXED_CONTEXT_H
#define JUSTOS_TESTS_SUPPORT_FIXED_CONTEXT_H

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/mman.h>

#define JP_TEST_CONTEXT_ADDRESS ((uintptr_t)0x10000000000ull)

/* Maps `size` zeroed bytes at JP_TEST_CONTEXT_ADDRESS and returns the pointer. The test stops if that is not possible. */
static inline void *jp_test_map_context(size_t size)
{
	void *memory = mmap((void *)JP_TEST_CONTEXT_ADDRESS, size, PROT_READ | PROT_WRITE,
			    MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);

	assert(memory != MAP_FAILED);
	assert((uintptr_t)memory == JP_TEST_CONTEXT_ADDRESS);
	return memory;
}

#endif /* JUSTOS_TESTS_SUPPORT_FIXED_CONTEXT_H */
