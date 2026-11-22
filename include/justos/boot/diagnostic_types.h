/*
 * diagnostic_types.h - the types of Stage 3 (diagnostics and recovery).
 *
 * Purpose:
 *   Every enumeration and structure of Stage 3, in one place, so that
 *   boot_types.h can hold the Stage 3 area inside JPBootContext (the
 *   context must be one complete object). The Stage 3 headers (error.h,
 *   boot_log.h, boot_status.h, panic.h, recovery.h, diagnostic_output.h,
 *   boot_report.h) declare the functions and include boot_types.h, which
 *   includes this file. Every name is the one that milestone0.md gives.
 *
 * Limitations:
 *   Declarations only. Do not include this file on its own; include
 *   justos/boot/boot_types.h.
 *
 * Depends on:
 *   the first part of justos/boot/boot_types.h (JPBootResult,
 *   JPArchitectureType, <stdint.h>, <stddef.h>, <stdbool.h>).
 */

#ifndef JUSTOS_BOOT_DIAGNOSTIC_TYPES_H
#define JUSTOS_BOOT_DIAGNOSTIC_TYPES_H

#ifndef JUSTOS_BOOT_BOOT_TYPES_H
#error "include justos/boot/boot_types.h, not diagnostic_types.h"
#endif

/* ---- Stage and source vocabulary ---- */

typedef enum JPDiagnosticStage {
	JP_DIAGNOSTIC_STAGE_UNKNOWN = 0,
	JP_DIAGNOSTIC_STAGE_STAGE1,
	JP_DIAGNOSTIC_STAGE_STAGE2,
	JP_DIAGNOSTIC_STAGE_STAGE3,
	JP_DIAGNOSTIC_STAGE_STAGE4
} JPDiagnosticStage;

/* A closed vocabulary: a later component gets a new value, never an old one. */
typedef enum JPBootSourceComponent {
	JP_BOOT_SOURCE_UNKNOWN = 0,
	JP_BOOT_SOURCE_STAGE1,
	JP_BOOT_SOURCE_STAGE2,
	JP_BOOT_SOURCE_BOOT_LOG,
	JP_BOOT_SOURCE_BOOT_STATUS,
	JP_BOOT_SOURCE_ERROR,
	JP_BOOT_SOURCE_DIAGNOSTIC_OUTPUT,
	JP_BOOT_SOURCE_BOOT_REPORT,
	JP_BOOT_SOURCE_PANIC,
	JP_BOOT_SOURCE_RECOVERY,
	JP_BOOT_SOURCE_BOOT_CONFIG,
	JP_BOOT_SOURCE_BOOT_INFO,
	JP_BOOT_SOURCE_BOOT_UI,
	JP_BOOT_SOURCE_BOOT_MANAGER
} JPBootSourceComponent;

/* ---- Errors (error.h) ---- */

typedef enum JPBootSeverity {
	JP_BOOT_SEVERITY_INFO = 0,
	JP_BOOT_SEVERITY_WARNING,
	JP_BOOT_SEVERITY_ERROR,
	JP_BOOT_SEVERITY_FATAL
} JPBootSeverity;

typedef enum JPBootRecoverability {
	JP_BOOT_RECOVERABILITY_RECOVERABLE = 0,
	JP_BOOT_RECOVERABILITY_NON_RECOVERABLE,
	JP_BOOT_RECOVERABILITY_UNKNOWN
} JPBootRecoverability;

typedef enum JPBootSeverityOverride {
	JP_BOOT_SEVERITY_OVERRIDE_UNSPECIFIED = 0,
	JP_BOOT_SEVERITY_OVERRIDE_INFO,
	JP_BOOT_SEVERITY_OVERRIDE_WARNING,
	JP_BOOT_SEVERITY_OVERRIDE_ERROR,
	JP_BOOT_SEVERITY_OVERRIDE_FATAL
} JPBootSeverityOverride;

typedef enum JPBootRecoverabilityOverride {
	JP_BOOT_RECOVERABILITY_OVERRIDE_UNSPECIFIED = 0,
	JP_BOOT_RECOVERABILITY_OVERRIDE_RECOVERABLE,
	JP_BOOT_RECOVERABILITY_OVERRIDE_NON_RECOVERABLE,
	JP_BOOT_RECOVERABILITY_OVERRIDE_UNKNOWN
} JPBootRecoverabilityOverride;

typedef struct JPBootErrorInput {
	JPBootResult original_result;
	JPBootSourceComponent source_component;
	JPDiagnosticStage stage;
	JPBootSeverityOverride severity_override;
	JPBootRecoverabilityOverride recoverability_override;
	uint32_t diagnostic_context;
} JPBootErrorInput;

typedef struct JPBootError {
	uint64_t boot_attempt_id;
	JPBootResult original_result;
	JPDiagnosticStage stage;
	JPBootSeverity severity;
	JPBootRecoverability recoverability;
	JPArchitectureType architecture;
	JPBootSourceComponent source_component;
	uint32_t diagnostic_context;
} JPBootError;

/* ---- Boot log (boot_log.h) ---- */

typedef struct JPBootEvent {
	uint64_t boot_attempt_id;
	uint64_t event_sequence;
	JPDiagnosticStage stage;
	JPBootSeverity severity;
	JPArchitectureType architecture;
	JPBootSourceComponent source_component;
	JPBootResult result;
	uint32_t diagnostic_context;
} JPBootEvent;

typedef struct JPBootLogStatus {
	size_t event_count;
	size_t discarded_event_count;
	bool truncated;
	bool finalized;
} JPBootLogStatus;

#define JP_BOOT_LOG_CAPACITY 256

typedef struct JPBootLogStorage {
	JPBootEvent events[JP_BOOT_LOG_CAPACITY];
	JPBootLogStatus status;
	uint64_t next_event_sequence;
} JPBootLogStorage;

/* ---- Boot status (boot_status.h) ---- */

typedef enum JPBootStatus {
	JP_BOOT_STATUS_BOOTING = 0,
	JP_BOOT_STATUS_INITIALIZING,
	JP_BOOT_STATUS_RUNNING,
	JP_BOOT_STATUS_FAILED,
	JP_BOOT_STATUS_RECOVERY,
	JP_BOOT_STATUS_HALTED
} JPBootStatus;

/* The number of retries that Milestone 0 allows: none. */
#define JP_BOOT_MAXIMUM_RETRY_COUNT 0

typedef struct JPBootAttemptState {
	uint64_t boot_attempt_id;
	uint64_t attempt_sequence;
	uint32_t retry_count;
	uint32_t maximum_retry_count;
} JPBootAttemptState;

typedef struct JPBootStatusState {
	JPDiagnosticStage stage;
	JPBootStatus status;
	JPBootSeverity severity;
	JPBootRecoverability recoverability;
	JPBootResult original_result;
	JPArchitectureType architecture;
} JPBootStatusState;

/* ---- Panic (panic.h) ---- */

typedef enum JPBootPanicState {
	JP_BOOT_PANIC_NORMAL = 0,
	JP_BOOT_PANIC_REQUESTED,
	JP_BOOT_PANIC_ACTIVE,
	JP_BOOT_PANIC_DIAGNOSTIC_PRESENTED,
	JP_BOOT_PANIC_TERMINAL
} JPBootPanicState;

/* ---- Recovery (recovery.h) ---- */

typedef enum JPRecoveryDecision {
	JP_RECOVERY_ENTER = 0,
	JP_RECOVERY_RETRY,
	JP_RECOVERY_TERMINAL_HALT
} JPRecoveryDecision;

typedef struct JPBootRecoveryState {
	uint64_t current_attempt_id;
	uint64_t previous_attempt_id;
	uint32_t retry_count;
	uint32_t maximum_retry_count;
	bool exhausted;
	JPRecoveryDecision decision;
} JPBootRecoveryState;

/* ---- Diagnostic output (diagnostic_output.h) ---- */

typedef struct JPBootDiagnosticMessage {
	JPDiagnosticStage stage;
	JPBootSourceComponent source_component;
	uint32_t diagnostic_context;
	const char *text;
	size_t length;
} JPBootDiagnosticMessage;

/* What the output router remembers between messages (see "3 Decided rules"). */
typedef struct JPBootOutputState {
	uint32_t next_line;
	uint32_t failed_messages;
	bool serial_probed;
	bool serial_available;
} JPBootOutputState;

/* ---- Boot report (boot_report.h) ---- */

typedef struct JPBootReport {
	uint64_t boot_attempt_id;
	JPBootStatusState status;
	JPBootError error;
	JPBootRecoveryState recovery;
	JPBootLogStatus log_status;
	JPArchitectureType architecture;
	bool finalized;
} JPBootReport;

/* ---- The Stage 3 area of the context ---- */

/*
 * Everything that Stage 3 keeps. The flags say whether the part exists: a
 * part that was never set up is never read as if it had a value. Stage 1 and
 * Stage 2 never read, write, or interpret this area.
 */
typedef struct JPBootStage3Area {
	JPBootLogStorage log;
	bool log_initialized;
	JPBootAttemptState attempt;
	JPBootStatusState status;
	bool status_initialized;
	JPBootError error;
	bool error_recorded;
	JPBootPanicState panic;
	JPBootError first_panic;
	bool first_panic_recorded;
	JPBootRecoveryState recovery;
	JPBootOutputState output;
	JPBootReport report;
} JPBootStage3Area;

#endif /* JUSTOS_BOOT_DIAGNOSTIC_TYPES_H */
