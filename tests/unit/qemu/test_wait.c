/* Unit test (level U) of the shared waiting loop (wait_common.c) with a fake clock. */

#include <assert.h>

#include "wait.h"

typedef struct Fake {
	uint64_t now;
	uint64_t step;               /* ticks added at every clock read */
	unsigned int escape_after;   /* ESC is reported at this escape check (1-based); 0 = never */
	unsigned int escape_checks;
	unsigned int clock_reads;
} Fake;

static uint64_t fake_ticks(void *user)
{
	Fake *fake = (Fake *)user;

	fake->clock_reads++;
	fake->now += fake->step;
	return fake->now;
}

static bool fake_escape(void *user)
{
	Fake *fake = (Fake *)user;

	fake->escape_checks++;
	return fake->escape_after != 0 && fake->escape_checks >= fake->escape_after;
}

static JPTestWaitSource source_of(Fake *fake, uint64_t hertz)
{
	JPTestWaitSource source = { fake_ticks, hertz, fake_escape, fake };

	return source;
}

static void test_timeout_after_the_given_seconds(void)
{
	Fake fake = { 1000, 10, 0, 0, 0 };
	JPTestWaitSource source = source_of(&fake, 100);   /* 100 ticks per second */

	/* 40 s = 4000 ticks, 10 ticks per read: about 400 reads. */
	assert(jp_test_wait_poll(&source, 40) == JP_TEST_WAIT_TIMEOUT);
	assert(fake.now - 1000 >= 4000);
	assert(fake.now - 1000 < 4000 + 3 * fake.step);
}

static void test_escape_ends_the_wait_early(void)
{
	Fake fake = { 0, 10, 5, 0, 0 };
	JPTestWaitSource source = source_of(&fake, 100);

	assert(jp_test_wait_poll(&source, 40) == JP_TEST_WAIT_ESCAPE);
	assert(fake.escape_checks == 5);
	assert(fake.now < 4000);
}

static void test_escape_already_pressed_wins_even_for_zero_seconds(void)
{
	Fake fake = { 0, 10, 1, 0, 0 };
	JPTestWaitSource source = source_of(&fake, 100);

	assert(jp_test_wait_poll(&source, 0) == JP_TEST_WAIT_ESCAPE);
}

static void test_zero_seconds_without_escape_ends_at_once(void)
{
	Fake fake = { 0, 10, 0, 0, 0 };
	JPTestWaitSource source = source_of(&fake, 100);

	assert(jp_test_wait_poll(&source, 0) == JP_TEST_WAIT_TIMEOUT);
	assert(fake.escape_checks == 1);
}

static void test_broken_clock_never_waits_forever(void)
{
	Fake fake = { 0, 0, 0, 0, 0 };
	JPTestWaitSource source = source_of(&fake, 0);

	assert(jp_test_wait_poll(&source, 40) == JP_TEST_WAIT_TIMEOUT);
	assert(fake.clock_reads == 0);
}

static void test_large_frequency_and_seconds_do_not_overflow(void)
{
	Fake fake = { 0, 0x100000000ULL, 0, 0, 0 };
	JPTestWaitSource source = source_of(&fake, 0xFFFFFFFFULL);

	/* limit = 40 * (2^32 - 1), computed in 64 bits; each read adds 2^32. */
	assert(jp_test_wait_poll(&source, 40) == JP_TEST_WAIT_TIMEOUT);
	assert(fake.now >= 40ULL * 0xFFFFFFFFULL);
}

int main(void)
{
	test_timeout_after_the_given_seconds();
	test_escape_ends_the_wait_early();
	test_escape_already_pressed_wins_even_for_zero_seconds();
	test_zero_seconds_without_escape_ends_at_once();
	test_broken_clock_never_waits_forever();
	test_large_frequency_and_seconds_do_not_overflow();
	return 0;
}
