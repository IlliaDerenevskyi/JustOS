/*
 * boot_types.h - central normalized Stage 1 type and constant contract.
 *
 * Purpose:
 *   Defines the normalized structures, state enumerations and contract
 *   constants shared by all generic Stage 1 modules.
 *
 * Responsibilities:
 *   - normalized Stage 1 data structures and state enumerations;
 *   - public Stage 1 constants (ELF, capacities);
 *   - normalized boot input and pre-transition interaction types.
 *
 * Limitations:
 *   Declarations only. No runtime logic and no provider-native (UEFI, BIOS,
 *   VBE, GOP) structures. JPBootInfo is not declared here.
 *
 * Public API:
 *   None. The file exposes shared types and constants only.
 *
 * Depends on:
 *   <stdint.h>, <stddef.h>, <stdbool.h>.
 *
 * Must not depend on:
 *   Any firmware header, any provider header.
 */

#ifndef JUSTOS_BOOT_BOOT_TYPES_H
#define JUSTOS_BOOT_BOOT_TYPES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* ---- Capacity constants ---- */

#define JP_MAX_MEMORY_REGIONS 128
#define JP_MAX_KERNEL_SEGMENTS 16

/* Memory layout constants of Stage 1. */
#define JP_BOOT_PAGE_SIZE 4096
#define JP_BOOT_PROTECTED_LOW_MEMORY 0x100000
#define JP_BOOT_KERNEL_STACK_SIZE 16384

/* ---- Generic ELF constants ---- */

#define JP_ELF_MAGIC 0x464C457F
#define JP_ELF_CLASS_64 2
#define JP_ELF_TYPE_EXEC 2
#define JP_ELF_TYPE_DYN 3
#define JP_ELF_PT_LOAD 1

/* ELF segment flags (p_flags). */
#define JP_ELF_PF_X 1
#define JP_ELF_PF_W 2
#define JP_ELF_PF_R 4

#define JP_ELF_MACHINE_X86_64 62
#define JP_ELF_MACHINE_AARCH64 183

/* ---- Enumerations ---- */

typedef enum JPBootResult {
	JP_BOOT_SUCCESS = 0,
	JP_BOOT_NOT_INITIALIZED,
	JP_BOOT_NOT_DETECTED,
	JP_BOOT_UNKNOWN,
	JP_BOOT_FAILED,
	JP_BOOT_ERROR_INVALID_PARAM,
	JP_BOOT_ERROR_CORRUPTED_DATA
} JPBootResult;

typedef enum JPBootStage {
	JP_BOOT_STAGE_UNINITIALIZED = 0,
	JP_BOOT_STAGE_HARDWARE_DISCOVERY,
	JP_BOOT_STAGE_MEMORY_DISCOVERY,
	JP_BOOT_STAGE_FRAMEBUFFER_INIT,
	JP_BOOT_STAGE_KERNEL_IMAGE_ACQUISITION,
	JP_BOOT_STAGE_KERNEL_VALIDATION,
	JP_BOOT_STAGE_KERNEL_LOADING,
	JP_BOOT_STAGE_HANDOFF_PREPARATION,
	JP_BOOT_STAGE_READY
} JPBootStage;

typedef enum JPDetectionState {
	JP_DETECTION_STATE_UNKNOWN = 0,
	JP_DETECTION_STATE_NOT_DETECTED,
	JP_DETECTION_STATE_NOT_INITIALIZED,
	JP_DETECTION_STATE_DETECTED,
	JP_DETECTION_STATE_FAILED
} JPDetectionState;

typedef enum JPBootEnvironmentType {
	JP_BOOT_ENVIRONMENT_UNKNOWN = 0,
	JP_BOOT_ENVIRONMENT_UEFI,
	JP_BOOT_ENVIRONMENT_BIOS
} JPBootEnvironmentType;

/* How far the boot-environment provider has got (see boot_environment.h). */
typedef enum JPBootEnvironmentLifecycle {
	JP_BOOT_ENVIRONMENT_LIFECYCLE_UNINITIALIZED = 0,
	JP_BOOT_ENVIRONMENT_LIFECYCLE_INITIALIZED,
	JP_BOOT_ENVIRONMENT_LIFECYCLE_ACTIVE,
	JP_BOOT_ENVIRONMENT_LIFECYCLE_HANDOFF_PREPARED,
	JP_BOOT_ENVIRONMENT_LIFECYCLE_FAILED
} JPBootEnvironmentLifecycle;

typedef enum JPArchitectureType {
	JP_ARCH_UNKNOWN = 0,
	JP_ARCH_X86_64,
	JP_ARCH_AARCH64
} JPArchitectureType;

typedef enum JPMemoryRegionType {
	JP_MEMORY_REGION_UNKNOWN = 0,
	JP_MEMORY_REGION_USABLE,
	JP_MEMORY_REGION_RESERVED,
	JP_MEMORY_REGION_ACPI,
	JP_MEMORY_REGION_MMIO,
	JP_MEMORY_REGION_FIRMWARE,
	JP_MEMORY_REGION_UNUSABLE
} JPMemoryRegionType;

typedef enum JPFramebufferFormat {
	JP_FRAMEBUFFER_FORMAT_UNKNOWN = 0,
	JP_FRAMEBUFFER_FORMAT_RGB_888,
	JP_FRAMEBUFFER_FORMAT_BGR_888
} JPFramebufferFormat;

typedef enum JPKernelValidationState {
	JP_KERNEL_VALIDATION_UNKNOWN = 0,
	JP_KERNEL_VALIDATION_NOT_INITIALIZED,
	JP_KERNEL_VALIDATION_VALID,
	JP_KERNEL_VALIDATION_INVALID,
	JP_KERNEL_VALIDATION_FAILED
} JPKernelValidationState;

typedef enum JPKernelLoadState {
	JP_KERNEL_LOAD_UNKNOWN = 0,
	JP_KERNEL_LOAD_NOT_INITIALIZED,
	JP_KERNEL_LOAD_LOADED,
	JP_KERNEL_LOAD_FAILED
} JPKernelLoadState;

/* Lifecycle of one kernel segment (see kernel_loader.c). FAILED is distinct from every success state. */
typedef enum JPKernelSegmentState {
	JP_KERNEL_SEGMENT_UNINITIALIZED = 0,
	JP_KERNEL_SEGMENT_VALIDATED,
	JP_KERNEL_SEGMENT_ALLOCATED,
	JP_KERNEL_SEGMENT_LOADED,
	JP_KERNEL_SEGMENT_MAPPED,
	JP_KERNEL_SEGMENT_PERMISSIONED,
	JP_KERNEL_SEGMENT_FAILED
} JPKernelSegmentState;

typedef enum JPHandoffState {
	JP_HANDOFF_STATE_UNINITIALIZED = 0,
	JP_HANDOFF_STATE_PREREQUISITES_VALID,
	JP_HANDOFF_STATE_PLATFORM_PREPARED,
	JP_HANDOFF_STATE_READY,
	JP_HANDOFF_STATE_FAILED
} JPHandoffState;

typedef enum JPBootInputKind {
	JP_BOOT_INPUT_NONE = 0,
	JP_BOOT_INPUT_KEY,
	JP_BOOT_INPUT_CHARACTER
} JPBootInputKind;

typedef enum JPBootInputKey {
	JP_BOOT_KEY_UP,
	JP_BOOT_KEY_DOWN,
	JP_BOOT_KEY_LEFT,
	JP_BOOT_KEY_RIGHT,
	JP_BOOT_KEY_ENTER,
	JP_BOOT_KEY_ESCAPE,
	JP_BOOT_KEY_F10,
	JP_BOOT_KEY_BACKSPACE
} JPBootInputKey;

typedef enum JPBootInteractionState {
	JP_BOOT_INTERACTION_NOT_REGISTERED = 0,
	JP_BOOT_INTERACTION_REGISTERED,
	JP_BOOT_INTERACTION_COMPLETED,
	JP_BOOT_INTERACTION_FAILED
} JPBootInteractionState;

/* ---- Normalized hardware information ---- */

typedef struct JPCpuInfo {
	JPDetectionState state;
	uint32_t logical_cores;
} JPCpuInfo;

typedef struct JPFirmwareInfo {
	JPDetectionState state;
	JPBootEnvironmentType type;
} JPFirmwareInfo;

typedef struct JPBootDeviceInfo {
	JPDetectionState state;
} JPBootDeviceInfo;

typedef struct JPHardwareInfo {
	JPCpuInfo cpu;
	JPFirmwareInfo firmware;
	JPBootDeviceInfo boot_device;
	JPDetectionState overall_state;
} JPHardwareInfo;

/* platform_context is opaque and owned by the provider. */
typedef struct JPBootEnvironment {
	JPBootEnvironmentType type;
	JPDetectionState state;
	void *platform_context;
	JPBootEnvironmentLifecycle lifecycle;   /* written only by boot_environment.c */
} JPBootEnvironment;

/* ---- Memory map ---- */

typedef struct JPMemoryRegion {
	uint64_t base_address;
	uint64_t length;
	JPMemoryRegionType type;
} JPMemoryRegion;

typedef struct JPMemoryMap {
	JPDetectionState state;
	size_t region_count;
	JPMemoryRegion regions[JP_MAX_MEMORY_REGIONS];
} JPMemoryMap;

/* ---- Framebuffer ---- */

/* pitch is measured in bytes per scanline. */
typedef struct JPFramebufferInfo {
	JPDetectionState state;
	uint64_t base_address;
	uint32_t width;
	uint32_t height;
	uint32_t pitch;
	JPFramebufferFormat format;
} JPFramebufferInfo;

/* ---- Kernel image ---- */

typedef struct JPKernelImage {
	JPDetectionState state;
	const void *buffer;
	size_t size;
} JPKernelImage;

typedef struct JPKernelHeader {
	uint32_t magic;
	uint16_t elf_class;
	uint16_t machine;
	uint16_t type;

	uint64_t entry_point;
	uint64_t program_header_offset;

	uint16_t program_header_entry_size;
	uint16_t program_header_count;
} JPKernelHeader;

typedef struct JPKernelSegment {
	uint64_t virtual_address;
	uint64_t physical_address;

	uint64_t file_offset;
	uint64_t file_size;
	uint64_t memory_size;

	uint32_t flags;

	JPKernelSegmentState state;
} JPKernelSegment;

typedef struct JPKernelInfo {
	JPKernelValidationState validation_state;
	JPKernelLoadState load_state;

	JPKernelImage image;
	JPKernelHeader header;

	JPKernelSegment segments[JP_MAX_KERNEL_SEGMENTS];
	size_t segment_count;

	uint64_t entry_point;
} JPKernelInfo;

/* ---- Handoff state ---- */

/*
 * Normalized handoff state. Architecture-specific register state (CR3 and
 * long mode on x86_64; TTBR, exception level and PSTATE on AArch64) is kept
 * behind the architecture modules; generic code reads only these fields.
 */
typedef struct JPHandoffInfo {
	JPHandoffState state;
	JPArchitectureType architecture;

	uint64_t kernel_entry;
	uint64_t stack_pointer;
	uint64_t page_table_root;

	/*
	 * The memory that holds the boot page tables and the descriptor table.
	 * Stage 2 adopts base and size as ownership facts; used counts the pages
	 * that the architecture module has already handed out of it.
	 */
	uint64_t page_table_memory_base;
	uint64_t page_table_memory_size;
	uint64_t page_table_memory_used;

	bool paging_enabled;
	bool interrupts_disabled;
	bool ownership_transferred;
} JPHandoffInfo;

/* ---- Pre-transition interaction ---- */

typedef struct JPBootContext JPBootContext;

typedef struct JPBootInputEvent {
	JPBootInputKind kind;
	JPBootInputKey key;
	uint32_t character;
} JPBootInputEvent;

typedef JPBootResult (*JPBootInteractionHook)(
	JPBootContext *context,
	void *user_data
);

typedef struct JPBootInteraction {
	JPBootInteractionState state;
	JPBootInteractionHook hook;
	void *user_data;
} JPBootInteraction;

/* ---- Stage 2 area ---- */

/*
 * The table "address range -> owner" of the Stage 2 memory subsystem. Only
 * src/kernel/memory.c reads and writes it. The entries are sorted by
 * base_address and never overlap. owner holds a JPKernelMemoryOwner value
 * (a plain integer here, so that this header needs no Stage 2 header).
 */
#define JP_KERNEL_MEMORY_MAX_ENTRIES 256

typedef struct JPBootStage2MemoryEntry {
	uint64_t base_address;
	uint64_t length;
	uint32_t owner;
} JPBootStage2MemoryEntry;

typedef struct JPBootStage2Area {
	size_t entry_count;
	JPBootStage2MemoryEntry entries[JP_KERNEL_MEMORY_MAX_ENTRIES];
} JPBootStage2Area;

/* ---- Areas reserved for later stages ---- */

/*
 * Stage 3 and Stage 4 own the contents of these areas; Stage 1 never reads,
 * writes or interprets them. The areas are reserved inside the context so
 * that their lifetime equals the lifetime of the context.
 *
 * TEMPORARY: the real types are defined by Stage 3 and Stage 4. Until those
 * stages exist, a byte block of a generous size is reserved. When a stage
 * defines its type, it replaces the corresponding block here, and the size
 * constant is removed.
 */
#define JP_BOOT_STAGE3_AREA_RESERVED_BYTES 65536
#define JP_BOOT_STAGE4_AREA_RESERVED_BYTES 8192

typedef struct JPBootStage3Area {
	_Alignas(16) uint8_t reserved[JP_BOOT_STAGE3_AREA_RESERVED_BYTES];
} JPBootStage3Area;

typedef struct JPBootStage4Area {
	_Alignas(16) uint8_t reserved[JP_BOOT_STAGE4_AREA_RESERVED_BYTES];
} JPBootStage4Area;

/* ---- Boot context ---- */

/*
 * The central runtime context. current_stage is owned and changed only by the
 * Stage 1 orchestrator. stage2_result is owned by Stage 2 and reads as
 * JP_BOOT_SUCCESS until a failure is recorded; the creator of the context
 * zero-initializes it. JPBootInfo is not part of this contract.
 */
struct JPBootContext {
	JPBootStage current_stage;

	JPBootEnvironment environment;
	JPHardwareInfo hardware;
	JPMemoryMap memory_map;
	JPFramebufferInfo framebuffer;
	JPKernelInfo kernel;
	JPHandoffInfo handoff;

	JPBootInteraction interaction;

	JPBootResult stage2_result;

	JPBootStage2Area stage2;

	JPBootStage3Area stage3;
	JPBootStage4Area stage4;
};

#endif /* JUSTOS_BOOT_BOOT_TYPES_H */
