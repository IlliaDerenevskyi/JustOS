#include "wait.h"

JPTestWaitResult jp_test_wait_poll(const JPTestWaitSource *source, uint32_t seconds)
{
	uint64_t start;
	uint64_t limit;

	if (source->ticks_per_second == 0) {
		return JP_TEST_WAIT_TIMEOUT;
	}
	start = source->ticks(source->user);
	/* seconds fits in 32 bits and the frequency in 32 bits in practice. */
	limit = (uint64_t)seconds * source->ticks_per_second;
	for (;;) {
		if (source->escape_pressed(source->user)) {
			return JP_TEST_WAIT_ESCAPE;
		}
		if (source->ticks(source->user) - start >= limit) {
			return JP_TEST_WAIT_TIMEOUT;
		}
	}
}
