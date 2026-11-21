/*
 * Unit test (level U) of src/boot/boot_config.c: the default values, every rule of validity, apply (all or
 * nothing), freeze, get, the state moves, and that a failed call changes nothing.
 */

#include <assert.h>
#include <string.h>

#include "justos/boot/boot_config.h"

static JPBootContext context;

static void fresh(void)
{
	memset(&context, 0, sizeof context);
}

static void ready(void)
{
	fresh();
	assert(jp_boot_config_initialize(&context) == JP_BOOT_SUCCESS);
}

static JPBootConfig get(void)
{
	JPBootConfig config;

	assert(jp_boot_config_get(&context, &config) == JP_BOOT_SUCCESS);
	return config;
}

/* Field by field: the padding bytes of a structure do not count. */
static bool same(const JPBootConfig *a, const JPBootConfig *b)
{
	if (a->state != b->state || a->source != b->source || memcmp(a->kernel_parameters, b->kernel_parameters, sizeof a->kernel_parameters) != 0 ||
	    a->boot_logging != b->boot_logging || a->verbose_output != b->verbose_output || a->safe_mode != b->safe_mode ||
	    a->recovery_requested != b->recovery_requested || a->entry_count != b->entry_count) {
		return false;
	}
	for (size_t i = 0; i < JP_BOOT_CONFIG_ENTRY_CAPACITY; i++) {
		if (memcmp(a->entries[i].id, b->entries[i].id, sizeof a->entries[i].id) != 0 ||
		    memcmp(a->entries[i].parameters, b->entries[i].parameters, sizeof a->entries[i].parameters) != 0 ||
		    a->entries[i].enabled != b->entries[i].enabled) {
			return false;
		}
	}
	return true;
}

/* A valid candidate that differs from the default in every editable field. */
static JPBootConfig candidate(void)
{
	JPBootConfig c;

	memset(&c, 0, sizeof c);
	strcpy(c.kernel_parameters, "quiet debug=1");
	c.boot_logging = false;
	c.verbose_output = true;
	c.safe_mode = true;
	c.recovery_requested = true;
	c.entry_count = 2;
	strcpy(c.entries[0].id, "alpha");
	strcpy(c.entries[0].parameters, "a b");
	c.entries[0].enabled = true;
	strcpy(c.entries[1].id, "beta");
	return c;
}

static void same_as_default(const JPBootConfig *c)
{
	assert(c->state == JP_BOOT_CONFIG_STATE_VALIDATED);
	assert(c->source == JP_BOOT_CONFIG_SOURCE_DEFAULT);
	assert(c->kernel_parameters[0] == '\0');
	assert(c->boot_logging && !c->verbose_output && !c->safe_mode && !c->recovery_requested);
	assert(c->entry_count == 0);
}

static void test_initialize(void)
{
	JPBootConfig c;

	fresh();
	assert(context.stage4.config.state == JP_BOOT_CONFIG_STATE_UNSET);
	assert(jp_boot_config_initialize(NULL) == JP_BOOT_ERROR_INVALID_PARAM);
	assert(jp_boot_config_initialize(&context) == JP_BOOT_SUCCESS);
	c = get();
	same_as_default(&c);
	/* All entries are zero. */
	for (size_t i = 0; i < JP_BOOT_CONFIG_ENTRY_CAPACITY; i++) {
		assert(c.entries[i].id[0] == '\0' && c.entries[i].parameters[0] == '\0' && !c.entries[i].enabled);
	}
	/* Allowed only in UNSET: a second call fails and changes nothing. */
	assert(jp_boot_config_apply(&context, &(JPBootConfig){ .boot_logging = false }) == JP_BOOT_SUCCESS);
	assert(jp_boot_config_initialize(&context) == JP_BOOT_FAILED);
	assert(!get().boot_logging);
	assert(jp_boot_config_freeze(&context) == JP_BOOT_SUCCESS);
	assert(jp_boot_config_initialize(&context) == JP_BOOT_FAILED);
	assert(get().state == JP_BOOT_CONFIG_STATE_FROZEN);
}

static void test_initialize_clears_stale_entries(void)
{
	fresh();
	strcpy(context.stage4.config.entries[3].id, "stale");
	context.stage4.config.entries[3].enabled = true;
	strcpy(context.stage4.config.kernel_parameters, "stale");
	assert(jp_boot_config_initialize(&context) == JP_BOOT_SUCCESS);
	assert(context.stage4.config.entries[3].id[0] == '\0' && !context.stage4.config.entries[3].enabled);
	assert(context.stage4.config.kernel_parameters[0] == '\0');
}

static void test_get(void)
{
	JPBootConfig c;

	fresh();
	assert(jp_boot_config_get(&context, &c) == JP_BOOT_NOT_INITIALIZED);
	assert(jp_boot_config_get(NULL, &c) == JP_BOOT_ERROR_INVALID_PARAM);
	assert(jp_boot_config_get(&context, NULL) == JP_BOOT_ERROR_INVALID_PARAM);
	ready();
	assert(jp_boot_config_get(&context, &c) == JP_BOOT_SUCCESS);
	assert(jp_boot_config_freeze(&context) == JP_BOOT_SUCCESS);
	assert(jp_boot_config_get(&context, &c) == JP_BOOT_SUCCESS);
	assert(c.state == JP_BOOT_CONFIG_STATE_FROZEN);
}

static void test_apply_valid(void)
{
	JPBootConfig c = candidate();
	JPBootConfig got;

	ready();
	c.state = JP_BOOT_CONFIG_STATE_FROZEN;          /* the state and the source of the candidate are ignored */
	c.source = JP_BOOT_CONFIG_SOURCE_UNKNOWN;
	assert(jp_boot_config_apply(&context, &c) == JP_BOOT_SUCCESS);
	got = get();
	assert(got.state == JP_BOOT_CONFIG_STATE_VALIDATED);
	assert(got.source == JP_BOOT_CONFIG_SOURCE_INTERACTIVE);
	assert(strcmp(got.kernel_parameters, "quiet debug=1") == 0);
	assert(!got.boot_logging && got.verbose_output && got.safe_mode && got.recovery_requested);
	assert(got.entry_count == 2);
	assert(strcmp(got.entries[0].id, "alpha") == 0 && strcmp(got.entries[0].parameters, "a b") == 0 && got.entries[0].enabled);
	assert(strcmp(got.entries[1].id, "beta") == 0 && !got.entries[1].enabled);
	/* The candidate belongs to the caller and is not changed. */
	assert(c.state == JP_BOOT_CONFIG_STATE_FROZEN && c.entry_count == 2);
	/* A second valid apply replaces the first. */
	c = candidate();
	c.entry_count = 0;
	memset(c.entries, 0, sizeof c.entries);
	assert(jp_boot_config_apply(&context, &c) == JP_BOOT_SUCCESS);
	assert(get().entry_count == 0 && get().entries[0].id[0] == '\0');
}

static void test_apply_full_capacity(void)
{
	JPBootConfig c;

	ready();
	memset(&c, 0, sizeof c);
	memset(c.kernel_parameters, 'k', JP_BOOT_CONFIG_KERNEL_PARAMETERS_CAPACITY - 1);   /* the longest text */
	c.entry_count = JP_BOOT_CONFIG_ENTRY_CAPACITY;
	for (size_t i = 0; i < JP_BOOT_CONFIG_ENTRY_CAPACITY; i++) {
		c.entries[i].id[0] = 'e';
		c.entries[i].id[1] = (char)('a' + i);
		memset(c.entries[i].parameters, 'p', JP_BOOT_CONFIG_ENTRY_PARAMETERS_CAPACITY - 1);
	}
	memset(c.entries[0].id + 2, 'x', JP_BOOT_CONFIG_ENTRY_ID_CAPACITY - 3);       /* the longest id */
	assert(jp_boot_config_apply(&context, &c) == JP_BOOT_SUCCESS);
	assert(get().entry_count == JP_BOOT_CONFIG_ENTRY_CAPACITY);
}

/* A rejected candidate: INVALID_PARAM, and the active configuration is exactly as it was. */
static void rejected(const JPBootConfig *c)
{
	JPBootConfig before;
	JPBootConfig after;

	before = get();
	assert(jp_boot_config_apply(&context, c) == JP_BOOT_ERROR_INVALID_PARAM);
	after = get();
	assert(same(&before, &after));
}

static void test_apply_rejects(void)
{
	JPBootConfig c = candidate();

	ready();
	assert(jp_boot_config_apply(&context, &(JPBootConfig){ .verbose_output = true }) == JP_BOOT_SUCCESS);   /* not the default */

	assert(jp_boot_config_apply(NULL, &c) == JP_BOOT_ERROR_INVALID_PARAM);
	assert(jp_boot_config_apply(&context, NULL) == JP_BOOT_ERROR_INVALID_PARAM);

	/* The kernel parameters: no end marker, a non-printable character, a control character, a high byte. */
	c = candidate();
	memset(c.kernel_parameters, 'a', sizeof c.kernel_parameters);
	rejected(&c);
	c = candidate();
	c.kernel_parameters[2] = '\n';
	rejected(&c);
	c = candidate();
	c.kernel_parameters[0] = 0x1F;
	rejected(&c);
	c = candidate();
	c.kernel_parameters[1] = 0x7F;
	rejected(&c);
	c = candidate();
	c.kernel_parameters[3] = (char)0xC3;
	rejected(&c);
	/* A space and the first and the last printable characters are allowed. */
	c = candidate();
	strcpy(c.kernel_parameters, " ~!");
	assert(jp_boot_config_apply(&context, &c) == JP_BOOT_SUCCESS);

	/* The entry count. */
	c = candidate();
	c.entry_count = JP_BOOT_CONFIG_ENTRY_CAPACITY + 1;
	rejected(&c);

	/* A used entry: empty id, an id with a space, a non-printable id, no end marker, a duplicate id. */
	c = candidate();
	c.entries[1].id[0] = '\0';
	rejected(&c);
	c = candidate();
	strcpy(c.entries[1].id, "b c");
	rejected(&c);
	c = candidate();
	strcpy(c.entries[1].id, "b\tc");
	rejected(&c);
	c = candidate();
	memset(c.entries[1].id, 'i', sizeof c.entries[1].id);
	rejected(&c);
	c = candidate();
	strcpy(c.entries[1].id, "alpha");
	rejected(&c);
	/* Two used entries that differ only in the last character are different. */
	c = candidate();
	strcpy(c.entries[1].id, "alph");
	assert(jp_boot_config_apply(&context, &c) == JP_BOOT_SUCCESS);

	/* The text of a used entry: no end marker, a non-printable character. */
	c = candidate();
	memset(c.entries[0].parameters, 'q', sizeof c.entries[0].parameters);
	rejected(&c);
	c = candidate();
	c.entries[0].parameters[1] = '\r';
	rejected(&c);

	/* An unused entry must be all zero. */
	c = candidate();
	c.entries[2].id[0] = 'z';
	rejected(&c);
	c = candidate();
	c.entries[5].parameters[10] = 'z';
	rejected(&c);
	c = candidate();
	c.entries[JP_BOOT_CONFIG_ENTRY_CAPACITY - 1].enabled = true;
	rejected(&c);
}

static void test_apply_states(void)
{
	JPBootConfig c = candidate();

	fresh();
	assert(jp_boot_config_apply(&context, &c) == JP_BOOT_FAILED);           /* UNSET */
	assert(context.stage4.config.state == JP_BOOT_CONFIG_STATE_UNSET);
	ready();
	assert(jp_boot_config_freeze(&context) == JP_BOOT_SUCCESS);
	{
		JPBootConfig before = get();

		assert(jp_boot_config_apply(&context, &c) == JP_BOOT_FAILED);       /* FROZEN */
		{
			JPBootConfig now = get();

			assert(same(&before, &now));
		}
		/* A frozen configuration refuses even an invalid candidate with FAILED (the state check comes first). */
		c.entry_count = JP_BOOT_CONFIG_ENTRY_CAPACITY + 1;
		assert(jp_boot_config_apply(&context, &c) == JP_BOOT_FAILED);
	}
}

static void test_freeze(void)
{
	fresh();
	assert(jp_boot_config_freeze(NULL) == JP_BOOT_ERROR_INVALID_PARAM);
	assert(jp_boot_config_freeze(&context) == JP_BOOT_FAILED);               /* UNSET */
	assert(context.stage4.config.state == JP_BOOT_CONFIG_STATE_UNSET);
	ready();
	assert(jp_boot_config_freeze(&context) == JP_BOOT_SUCCESS);
	assert(get().state == JP_BOOT_CONFIG_STATE_FROZEN);
	{
		JPBootConfig before = get();

		assert(jp_boot_config_freeze(&context) == JP_BOOT_SUCCESS);          /* twice: nothing changes */
		{
			JPBootConfig now = get();

			assert(same(&before, &now));
		}
	}
}

int main(void)
{
	test_initialize();
	test_initialize_clears_stale_entries();
	test_get();
	test_apply_valid();
	test_apply_full_capacity();
	test_apply_rejects();
	test_apply_states();
	test_freeze();
	return 0;
}
