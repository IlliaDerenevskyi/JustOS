/*
 * Unit test (level U) of phase A of src/boot_manager.c: jp_boot_manager_prepare and jp_boot_manager_select, with
 * both selection policies (the policy is chosen when the program is built, so CMake builds this test twice).
 * The real boot_config.c is used. The Stage 1 functions (registering the hook, the keyboard) and the menu are
 * fakes that record their calls and can fail.
 */

#include <assert.h>
#include <string.h>

#include "justos/boot/boot_config.h"
#include "justos/boot/boot_environment.h"
#include "justos/boot/boot_manager.h"
#include "justos/boot/boot_ui.h"
#include "justos/boot/handoff.h"

static JPBootContext context;

/* ---- The fakes ---- */

static unsigned int register_calls;
static JPBootInteractionHook registered_hook;
static void *registered_user_data;
static JPBootResult register_result;

static unsigned int read_calls;
static JPBootResult read_result;
static JPBootInputKind read_kind;

static unsigned int ui_calls;
static JPBootResult ui_result;
static JPBootSelectionOutcome ui_outcome;
static JPBootManagerState state_at_ui;

JPBootResult jp_boot_register_interaction(JPBootContext *ctx, JPBootInteractionHook hook, void *user_data)
{
	assert(ctx == &context);
	register_calls++;
	if (register_result == JP_BOOT_SUCCESS) {
		registered_hook = hook;
		registered_user_data = user_data;
	}
	return register_result;
}

JPBootResult jp_boot_environment_read_input(JPBootContext *ctx, JPBootInputEvent *event)
{
	assert(ctx == &context);
	read_calls++;
	if (read_result == JP_BOOT_SUCCESS) {
		event->kind = read_kind;
	}
	return read_result;
}

JPBootResult jp_boot_ui_select(JPBootContext *ctx, JPBootSelectionOutcome *outcome)
{
	assert(ctx == &context && outcome != NULL);
	ui_calls++;
	state_at_ui = ctx->stage4.state;
	if (ui_result == JP_BOOT_SUCCESS) {
		*outcome = ui_outcome;
	}
	return ui_result;
}

static void set_up(void)
{
	memset(&context, 0, sizeof context);
	context.framebuffer.state = JP_DETECTION_STATE_DETECTED;
	context.framebuffer.base_address = 0x1000;
	context.framebuffer.width = 640;
	context.framebuffer.height = 400;
	context.framebuffer.pitch = 640 * 4;
	context.framebuffer.format = JP_FRAMEBUFFER_FORMAT_RGB_888;
	register_calls = read_calls = ui_calls = 0;
	registered_hook = NULL;
	registered_user_data = (void *)1;
	register_result = JP_BOOT_SUCCESS;
	read_result = JP_BOOT_SUCCESS;
	read_kind = JP_BOOT_INPUT_NONE;
	ui_result = JP_BOOT_SUCCESS;
	ui_outcome = JP_BOOT_SELECTION_APPLIED;
}

static void prepared(void)
{
	set_up();
	assert(jp_boot_manager_prepare(&context) == JP_BOOT_SUCCESS);
}

/* ---- prepare ---- */

static void test_prepare(void)
{
	JPBootConfig c;

	set_up();
	assert(jp_boot_manager_prepare(NULL) == JP_BOOT_ERROR_INVALID_PARAM);
	assert(register_calls == 0);
	context.stage4.selection_outcome = JP_BOOT_SELECTION_DISCARDED;
	context.stage4.decision = JP_BOOT_DECISION_TERMINAL_HALT;       /* stale values are reset */
	assert(jp_boot_manager_prepare(&context) == JP_BOOT_SUCCESS);
	assert(context.stage4.state == JP_BOOT_MANAGER_PREPARED);
	assert(context.stage4.selection_outcome == JP_BOOT_SELECTION_NOT_RUN);
	assert(context.stage4.decision == JP_BOOT_DECISION_NONE);
	assert(jp_boot_config_get(&context, &c) == JP_BOOT_SUCCESS);
	assert(c.state == JP_BOOT_CONFIG_STATE_VALIDATED && c.source == JP_BOOT_CONFIG_SOURCE_DEFAULT);
	assert(register_calls == 1 && registered_hook == jp_boot_manager_select && registered_user_data == NULL);
}

static void test_prepare_twice_and_wrong_state(void)
{
	for (JPBootManagerState s = JP_BOOT_MANAGER_PREPARED; s <= JP_BOOT_MANAGER_DECIDED; s++) {
		set_up();
		context.stage4.state = s;
		assert(jp_boot_manager_prepare(&context) == JP_BOOT_FAILED);
		assert(context.stage4.state == s && register_calls == 0);
		assert(context.stage4.config.state == JP_BOOT_CONFIG_STATE_UNSET);      /* nothing was set up */
	}
	prepared();
	assert(jp_boot_manager_prepare(&context) == JP_BOOT_FAILED);
	assert(register_calls == 1 && context.stage4.state == JP_BOOT_MANAGER_PREPARED);
}

static void test_prepare_failures(void)
{
	/* The configuration is not UNSET: its failing result is returned, nothing is registered. */
	set_up();
	context.stage4.config.state = JP_BOOT_CONFIG_STATE_VALIDATED;
	assert(jp_boot_manager_prepare(&context) == JP_BOOT_FAILED);
	assert(context.stage4.state == JP_BOOT_MANAGER_UNINITIALIZED && register_calls == 0);

	/* Stage 1 refuses the registration: its result is returned unchanged, and the state stays UNINITIALIZED. */
	for (JPBootResult r = JP_BOOT_NOT_INITIALIZED; r <= JP_BOOT_ERROR_CORRUPTED_DATA; r++) {
		set_up();
		register_result = r;
		assert(jp_boot_manager_prepare(&context) == r);
		assert(context.stage4.state == JP_BOOT_MANAGER_UNINITIALIZED && registered_hook == NULL);
	}
}

/* ---- select ---- */

static void test_select_wrong_state(void)
{
	for (JPBootManagerState s = JP_BOOT_MANAGER_UNINITIALIZED; s <= JP_BOOT_MANAGER_DECIDED; s++) {
		if (s == JP_BOOT_MANAGER_PREPARED) {
			continue;
		}
		set_up();
		context.stage4.state = s;
		assert(jp_boot_manager_select(&context, NULL) == JP_BOOT_FAILED);
		assert(context.stage4.state == s && read_calls == 0 && ui_calls == 0);
		assert(context.stage4.selection_outcome == JP_BOOT_SELECTION_NOT_RUN);
	}
	assert(jp_boot_manager_select(NULL, NULL) == JP_BOOT_ERROR_INVALID_PARAM);
}

static void test_select_user_data_is_ignored(void)
{
	int value = 5;

	prepared();
	assert(jp_boot_manager_select(&context, &value) == JP_BOOT_SUCCESS);
	assert(value == 5);
}


static void test_select_policy_and_skips(void)
{
	if (JP_BOOT_SELECTION_POLICY == JP_BOOT_SELECTION_POLICY_DEFAULT_ONLY) {
		/* The policy comes first: the menu is skipped before anything else is looked at. */
		prepared();
		context.framebuffer.state = JP_DETECTION_STATE_NOT_DETECTED;       /* would be another outcome */
		read_result = JP_BOOT_NOT_DETECTED;
		assert(jp_boot_manager_select(&context, NULL) == JP_BOOT_SUCCESS);
		assert(context.stage4.selection_outcome == JP_BOOT_SELECTION_SKIPPED_POLICY);
		assert(context.stage4.state == JP_BOOT_MANAGER_SELECTED);
		assert(read_calls == 0 && ui_calls == 0);
		prepared();
		assert(jp_boot_manager_select(&context, NULL) == JP_BOOT_SUCCESS);
		assert(context.stage4.selection_outcome == JP_BOOT_SELECTION_SKIPPED_POLICY && ui_calls == 0 && read_calls == 0);
		/* The configuration stays at the default values. */
		assert(context.stage4.config.source == JP_BOOT_CONFIG_SOURCE_DEFAULT);
		return;
	}

	/* No usable framebuffer: every defect (the same rule as the menu's) skips, before the keyboard is asked. */
	for (int variant = 0; variant < 9; variant++) {
		prepared();
		read_result = JP_BOOT_NOT_DETECTED;                           /* the keyboard would also say no */
		switch (variant) {
		case 0: context.framebuffer.state = JP_DETECTION_STATE_NOT_DETECTED; break;
		case 1: context.framebuffer.state = JP_DETECTION_STATE_FAILED; break;
		case 2: context.framebuffer.base_address = 0; break;
		case 3: context.framebuffer.width = 0; break;
		case 4: context.framebuffer.height = 0; break;
		case 5: context.framebuffer.pitch = 0; break;
		case 6: context.framebuffer.format = JP_FRAMEBUFFER_FORMAT_UNKNOWN; break;
		case 7: context.framebuffer.pitch = 640 * 4 - 4; break;
		default: context.framebuffer.pitch = 640 * 4 + 2; break;
		}
		assert(jp_boot_manager_select(&context, NULL) == JP_BOOT_SUCCESS);
		assert(context.stage4.selection_outcome == JP_BOOT_SELECTION_SKIPPED_NO_FRAMEBUFFER);
		assert(context.stage4.state == JP_BOOT_MANAGER_SELECTED);
		assert(read_calls == 0 && ui_calls == 0);
	}
	/* The exact pitch and the BGR format are usable. */
	prepared();
	context.framebuffer.pitch = 640 * 4;
	context.framebuffer.format = JP_FRAMEBUFFER_FORMAT_BGR_888;
	assert(jp_boot_manager_select(&context, NULL) == JP_BOOT_SUCCESS);
	assert(ui_calls == 1);

	/* No keyboard. */
	prepared();
	read_result = JP_BOOT_NOT_DETECTED;
	assert(jp_boot_manager_select(&context, NULL) == JP_BOOT_SUCCESS);
	assert(context.stage4.selection_outcome == JP_BOOT_SELECTION_SKIPPED_NO_INPUT);
	assert(context.stage4.state == JP_BOOT_MANAGER_SELECTED && read_calls == 1 && ui_calls == 0);
}

static void test_select_keyboard_failure(void)
{
	if (JP_BOOT_SELECTION_POLICY == JP_BOOT_SELECTION_POLICY_DEFAULT_ONLY) {
		return;
	}
	/* The keyboard answers with a failure: it is returned unchanged, the state stays PREPARED. */
	for (JPBootResult r = JP_BOOT_NOT_INITIALIZED; r <= JP_BOOT_ERROR_CORRUPTED_DATA; r++) {
		if (r == JP_BOOT_NOT_DETECTED) {
			continue;
		}
		prepared();
		read_result = r;
		assert(jp_boot_manager_select(&context, NULL) == r);
		assert(context.stage4.state == JP_BOOT_MANAGER_PREPARED);
		assert(context.stage4.selection_outcome == JP_BOOT_SELECTION_NOT_RUN && ui_calls == 0);
	}
}

static void test_select_runs_the_menu(void)
{
	if (JP_BOOT_SELECTION_POLICY == JP_BOOT_SELECTION_POLICY_DEFAULT_ONLY) {
		return;
	}
	/* A keyboard that answers with "no key" or with a key exists: the menu runs once. */
	for (int kind = JP_BOOT_INPUT_NONE; kind <= JP_BOOT_INPUT_CHARACTER; kind++) {
		prepared();
		read_kind = (JPBootInputKind)kind;
		ui_outcome = JP_BOOT_SELECTION_APPLIED;
		assert(jp_boot_manager_select(&context, NULL) == JP_BOOT_SUCCESS);
		assert(read_calls == 1 && ui_calls == 1 && state_at_ui == JP_BOOT_MANAGER_PREPARED);
		assert(context.stage4.selection_outcome == JP_BOOT_SELECTION_APPLIED);
		assert(context.stage4.state == JP_BOOT_MANAGER_SELECTED);
	}
	prepared();
	ui_outcome = JP_BOOT_SELECTION_DISCARDED;
	assert(jp_boot_manager_select(&context, NULL) == JP_BOOT_SUCCESS);
	assert(context.stage4.selection_outcome == JP_BOOT_SELECTION_DISCARDED);
	/* At most once: a second call is refused and changes nothing. */
	assert(jp_boot_manager_select(&context, NULL) == JP_BOOT_FAILED);
	assert(ui_calls == 1 && context.stage4.selection_outcome == JP_BOOT_SELECTION_DISCARDED);
}

static void test_select_menu_failure(void)
{
	if (JP_BOOT_SELECTION_POLICY == JP_BOOT_SELECTION_POLICY_DEFAULT_ONLY) {
		return;
	}
	for (JPBootResult r = JP_BOOT_NOT_INITIALIZED; r <= JP_BOOT_ERROR_CORRUPTED_DATA; r++) {
		prepared();
		ui_result = r;
		ui_outcome = JP_BOOT_SELECTION_APPLIED;                       /* must not be taken over after a failure */
		assert(jp_boot_manager_select(&context, NULL) == r);
		assert(context.stage4.state == JP_BOOT_MANAGER_PREPARED);
		assert(context.stage4.selection_outcome == JP_BOOT_SELECTION_NOT_RUN);
	}
}

int main(void)
{
	test_prepare();
	test_prepare_twice_and_wrong_state();
	test_prepare_failures();
	test_select_wrong_state();
	test_select_user_data_is_ignored();
	test_select_policy_and_skips();
	test_select_keyboard_failure();
	test_select_runs_the_menu();
	test_select_menu_failure();
	return 0;
}
