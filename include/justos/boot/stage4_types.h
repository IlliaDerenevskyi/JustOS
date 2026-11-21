/*
 * stage4_types.h - the types of Stage 4 (boot manager and boot UI).
 *
 * Purpose:
 *   The boot configuration, the menu outcome, the manager state, the
 *   decision, and the Stage 4 area, in one place, so that boot_types.h can
 *   hold the Stage 4 area inside JPBootContext (the context must be one
 *   complete object). The Stage 4 headers (boot_config.h, boot_info.h,
 *   boot_ui.h, boot_manager.h) declare the functions and include
 *   boot_types.h, which includes this file. Every name is the one that
 *   milestone0.md gives ("4 Boot Configuration Contract", "4 Selection
 *   Outcome and Manager State", "4 Stage 4 Area of JPBootContext").
 *
 * Limitations:
 *   Declarations only. Do not include this file on its own; include
 *   justos/boot/boot_types.h.
 *
 * Depends on:
 *   the first part of justos/boot/boot_types.h (<stdint.h>, <stddef.h>,
 *   <stdbool.h>).
 */

#ifndef JUSTOS_BOOT_STAGE4_TYPES_H
#define JUSTOS_BOOT_STAGE4_TYPES_H

#ifndef JUSTOS_BOOT_BOOT_TYPES_H
#error "include justos/boot/boot_types.h, not stage4_types.h"
#endif

/* ---- Boot configuration ---- */

#define JP_BOOT_CONFIG_KERNEL_PARAMETERS_CAPACITY 256   /* bytes, including the end marker */
#define JP_BOOT_CONFIG_ENTRY_CAPACITY 16                /* most entries a configuration can hold */
#define JP_BOOT_CONFIG_ENTRY_ID_CAPACITY 32             /* bytes for the name of an entry */
#define JP_BOOT_CONFIG_ENTRY_PARAMETERS_CAPACITY 64     /* bytes for the text of an entry */

typedef enum JPBootConfigSource {        /* where the configuration came from */
	JP_BOOT_CONFIG_SOURCE_UNKNOWN = 0,
	JP_BOOT_CONFIG_SOURCE_DEFAULT,       /* built-in default values */
	JP_BOOT_CONFIG_SOURCE_INTERACTIVE    /* chosen in the menu */
} JPBootConfigSource;

typedef enum JPBootConfigState {         /* how far the configuration has got */
	JP_BOOT_CONFIG_STATE_UNSET = 0,      /* nothing has been set up yet */
	JP_BOOT_CONFIG_STATE_VALIDATED,      /* checked and still changeable */
	JP_BOOT_CONFIG_STATE_FROZEN          /* checked and locked */
} JPBootConfigState;

typedef struct JPBootConfigEntry {       /* one launch record */
	char id[JP_BOOT_CONFIG_ENTRY_ID_CAPACITY];
	char parameters[JP_BOOT_CONFIG_ENTRY_PARAMETERS_CAPACITY];
	bool enabled;
} JPBootConfigEntry;

typedef struct JPBootConfig {
	JPBootConfigState state;
	JPBootConfigSource source;
	char kernel_parameters[JP_BOOT_CONFIG_KERNEL_PARAMETERS_CAPACITY];
	bool boot_logging;                   /* show a summary of the boot */
	bool verbose_output;                 /* also show every recorded event */
	bool safe_mode;                      /* a flag handed to the kernel */
	bool recovery_requested;             /* ask for the recovery screen */
	size_t entry_count;                  /* how many entries are used */
	JPBootConfigEntry entries[JP_BOOT_CONFIG_ENTRY_CAPACITY];
} JPBootConfig;

/* ---- Selection outcome, manager state, decision ---- */

typedef enum JPBootSelectionOutcome {    /* what happened to the menu */
	JP_BOOT_SELECTION_NOT_RUN = 0,       /* the menu function has not run yet */
	JP_BOOT_SELECTION_SKIPPED_POLICY,    /* the build setting turned the menu off */
	JP_BOOT_SELECTION_SKIPPED_NO_FRAMEBUFFER, /* there is no usable screen */
	JP_BOOT_SELECTION_SKIPPED_NO_INPUT,  /* there is no keyboard */
	JP_BOOT_SELECTION_APPLIED,           /* F10 was pressed and the choice was accepted */
	JP_BOOT_SELECTION_DISCARDED          /* Esc was pressed */
} JPBootSelectionOutcome;

typedef enum JPBootManagerState {        /* how far the Boot Manager has got */
	JP_BOOT_MANAGER_UNINITIALIZED = 0,
	JP_BOOT_MANAGER_PREPARED,
	JP_BOOT_MANAGER_SELECTED,
	JP_BOOT_MANAGER_COMPLETING,
	JP_BOOT_MANAGER_RECOVERY_REQUESTED,
	JP_BOOT_MANAGER_DECIDED
} JPBootManagerState;

typedef enum JPBootDecision {            /* what the Boot Manager decided */
	JP_BOOT_DECISION_NONE = 0,
	JP_BOOT_DECISION_CONTINUE_TO_KERNEL,
	JP_BOOT_DECISION_RETRY_BOOT,
	JP_BOOT_DECISION_ENTER_RECOVERY,
	JP_BOOT_DECISION_TERMINAL_HALT
} JPBootDecision;

/* ---- The Stage 4 area of JPBootContext ---- */

typedef struct JPBootStage4Area {
	JPBootConfig config;                      /* the boot configuration */
	JPBootManagerState state;                 /* how far the Boot Manager has got */
	JPBootSelectionOutcome selection_outcome; /* what happened to the menu */
	JPBootDecision decision;                  /* what the Boot Manager decided */
} JPBootStage4Area;

#endif /* JUSTOS_BOOT_STAGE4_TYPES_H */
