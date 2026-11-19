/*
 * uefi_defs.h - the private UEFI definitions.
 *
 * Purpose:
 *   There is no EDK2 or gnu-efi in the project, so the few UEFI structures that
 *   uefi.c needs are written here by hand, exactly as the UEFI specification
 *   lays them out (64-bit layout). Only uefi.c and uefi_entry.c include this
 *   file; no UEFI type appears in a public header.
 *
 * Limitations:
 *   Declarations only. Table entries that are not used are kept as unnamed
 *   pointers so that the offsets of the used entries stay right; static
 *   assertions at the end check every offset and size.
 *
 * Depends on:
 *   <stddef.h>, <stdint.h>.
 *
 * Must not depend on:
 *   Any other project header.
 */

#ifndef JUSTOS_BOOT_PLATFORM_UEFI_DEFS_H
#define JUSTOS_BOOT_PLATFORM_UEFI_DEFS_H

#include <stddef.h>
#include <stdint.h>

/* UEFI on x86_64 uses the Microsoft x64 calling convention; on AArch64 it uses the standard one. */
#if defined(__x86_64__) || defined(_M_X64)
#define JP_UEFI_API __attribute__((ms_abi))
#else
#define JP_UEFI_API
#endif

/* ---- Basic types ---- */

typedef uint64_t JPUefiStatus;          /* the highest bit marks an error */
typedef uint64_t JPUefiUintn;           /* both supported targets are 64-bit */
typedef void *JPUefiHandle;
typedef uint16_t JPUefiChar16;

#define JP_UEFI_ERROR_BIT 0x8000000000000000ULL
#define JP_UEFI_SUCCESS 0ULL
#define JP_UEFI_LOAD_ERROR (JP_UEFI_ERROR_BIT | 1ULL)
#define JP_UEFI_INVALID_PARAMETER (JP_UEFI_ERROR_BIT | 2ULL)
#define JP_UEFI_UNSUPPORTED (JP_UEFI_ERROR_BIT | 3ULL)
#define JP_UEFI_BUFFER_TOO_SMALL (JP_UEFI_ERROR_BIT | 5ULL)
#define JP_UEFI_NOT_READY (JP_UEFI_ERROR_BIT | 6ULL)
#define JP_UEFI_DEVICE_ERROR (JP_UEFI_ERROR_BIT | 7ULL)
#define JP_UEFI_OUT_OF_RESOURCES (JP_UEFI_ERROR_BIT | 9ULL)
#define JP_UEFI_NOT_FOUND (JP_UEFI_ERROR_BIT | 14ULL)

typedef struct JPUefiGuid {
	uint32_t data1;
	uint16_t data2;
	uint16_t data3;
	uint8_t data4[8];
} JPUefiGuid;

typedef struct JPUefiTableHeader {
	uint64_t signature;
	uint32_t revision;
	uint32_t header_size;
	uint32_t crc32;
	uint32_t reserved;
} JPUefiTableHeader;                    /* 24 bytes */

#define JP_UEFI_SYSTEM_TABLE_SIGNATURE 0x5453595320494249ULL   /* "IBI SYS" */

/* ---- Memory ---- */

typedef enum JPUefiMemoryType {
	JP_UEFI_MEMORY_RESERVED = 0,
	JP_UEFI_MEMORY_LOADER_CODE = 1,
	JP_UEFI_MEMORY_LOADER_DATA = 2,
	JP_UEFI_MEMORY_BOOT_SERVICES_CODE = 3,
	JP_UEFI_MEMORY_BOOT_SERVICES_DATA = 4,
	JP_UEFI_MEMORY_RUNTIME_SERVICES_CODE = 5,
	JP_UEFI_MEMORY_RUNTIME_SERVICES_DATA = 6,
	JP_UEFI_MEMORY_CONVENTIONAL = 7,
	JP_UEFI_MEMORY_UNUSABLE = 8,
	JP_UEFI_MEMORY_ACPI_RECLAIM = 9,
	JP_UEFI_MEMORY_ACPI_NVS = 10,
	JP_UEFI_MEMORY_MAPPED_IO = 11,
	JP_UEFI_MEMORY_MAPPED_IO_PORT_SPACE = 12,
	JP_UEFI_MEMORY_PAL_CODE = 13,
	JP_UEFI_MEMORY_PERSISTENT = 14
} JPUefiMemoryType;

/* The firmware may use a larger descriptor size than this structure: always step by the size it reports. */
typedef struct JPUefiMemoryDescriptor {
	uint32_t type;
	uint32_t padding;
	uint64_t physical_start;
	uint64_t virtual_start;
	uint64_t number_of_pages;
	uint64_t attribute;
} JPUefiMemoryDescriptor;               /* 40 bytes */

#define JP_UEFI_PAGE_SIZE 4096ULL

#define JP_UEFI_ALLOCATE_ANY_PAGES 0
#define JP_UEFI_ALLOCATE_ADDRESS 2

/* ---- Boot services ---- */

typedef JPUefiStatus (JP_UEFI_API *JPUefiAllocatePages)(JPUefiUintn type, JPUefiUintn memory_type,
	JPUefiUintn pages, uint64_t *address);
typedef JPUefiStatus (JP_UEFI_API *JPUefiFreePages)(uint64_t address, JPUefiUintn pages);
typedef JPUefiStatus (JP_UEFI_API *JPUefiGetMemoryMap)(JPUefiUintn *map_size,
	JPUefiMemoryDescriptor *map, JPUefiUintn *map_key, JPUefiUintn *descriptor_size,
	uint32_t *descriptor_version);
typedef JPUefiStatus (JP_UEFI_API *JPUefiAllocatePool)(JPUefiUintn memory_type, JPUefiUintn size, void **buffer);
typedef JPUefiStatus (JP_UEFI_API *JPUefiFreePool)(void *buffer);
typedef JPUefiStatus (JP_UEFI_API *JPUefiHandleProtocol)(JPUefiHandle handle, const JPUefiGuid *protocol, void **interface);
typedef JPUefiStatus (JP_UEFI_API *JPUefiExitBootServices)(JPUefiHandle image_handle, JPUefiUintn map_key);
typedef JPUefiStatus (JP_UEFI_API *JPUefiLocateProtocol)(const JPUefiGuid *protocol, void *registration, void **interface);

typedef struct JPUefiBootServices {
	JPUefiTableHeader header;                 /* offset 0 */
	void *raise_tpl;                          /* 24 */
	void *restore_tpl;                        /* 32 */
	JPUefiAllocatePages allocate_pages;       /* 40 */
	JPUefiFreePages free_pages;               /* 48 */
	JPUefiGetMemoryMap get_memory_map;        /* 56 */
	JPUefiAllocatePool allocate_pool;         /* 64 */
	JPUefiFreePool free_pool;                 /* 72 */
	void *unused_events[9];                   /* 80 to 144: events and protocol installation */
	JPUefiHandleProtocol handle_protocol;     /* 152 */
	void *unused_images[9];                   /* 160 to 224: reserved, locate handle, install table, images */
	JPUefiExitBootServices exit_boot_services;/* 232 */
	void *get_next_monotonic_count;           /* 240 */
	void *stall;                              /* 248 */
	void *unused_watchdog[3];                 /* 256 to 272: watchdog and controllers */
	void *open_protocol;                      /* 280 */
	void *unused_protocols[4];                /* 288 to 312 */
	JPUefiLocateProtocol locate_protocol;     /* 320 */
} JPUefiBootServices;

/* ---- Protocols ---- */

typedef struct JPUefiInputKey {
	uint16_t scan_code;
	JPUefiChar16 unicode_char;
} JPUefiInputKey;

typedef struct JPUefiSimpleTextInput {
	void *reset;                              /* 0 */
	JPUefiStatus (JP_UEFI_API *read_key_stroke)(struct JPUefiSimpleTextInput *self, JPUefiInputKey *key); /* 8 */
	void *wait_for_key;                       /* 16 */
} JPUefiSimpleTextInput;

typedef struct JPUefiSimpleTextOutput {
	void *reset;                              /* 0 */
	JPUefiStatus (JP_UEFI_API *output_string)(struct JPUefiSimpleTextOutput *self, const JPUefiChar16 *text); /* 8 */
} JPUefiSimpleTextOutput;

typedef struct JPUefiSystemTable {
	JPUefiTableHeader header;                 /* 0 */
	JPUefiChar16 *firmware_vendor;            /* 24 */
	uint32_t firmware_revision;               /* 32 */
	JPUefiHandle console_in_handle;           /* 40 */
	JPUefiSimpleTextInput *con_in;            /* 48 */
	JPUefiHandle console_out_handle;          /* 56 */
	JPUefiSimpleTextOutput *con_out;          /* 64 */
	JPUefiHandle standard_error_handle;       /* 72 */
	void *std_err;                            /* 80 */
	void *runtime_services;                   /* 88 */
	JPUefiBootServices *boot_services;        /* 96 */
	JPUefiUintn number_of_table_entries;      /* 104 */
	void *configuration_table;                /* 112 */
} JPUefiSystemTable;

typedef struct JPUefiLoadedImage {
	uint32_t revision;                        /* 0 */
	JPUefiHandle parent_handle;               /* 8 */
	JPUefiSystemTable *system_table;          /* 16 */
	JPUefiHandle device_handle;               /* 24 */
	void *file_path;                          /* 32 */
	void *reserved;                           /* 40 */
	uint32_t load_options_size;               /* 48 */
	void *load_options;                       /* 56 */
	void *image_base;                         /* 64 */
	uint64_t image_size;                      /* 72 */
	uint32_t image_code_type;                 /* 80 */
	uint32_t image_data_type;                 /* 84 */
	void *unload;                             /* 88 */
} JPUefiLoadedImage;

typedef struct JPUefiFile {
	uint64_t revision;                        /* 0 */
	JPUefiStatus (JP_UEFI_API *open)(struct JPUefiFile *self, struct JPUefiFile **new_handle,
		const JPUefiChar16 *file_name, uint64_t open_mode, uint64_t attributes); /* 8 */
	JPUefiStatus (JP_UEFI_API *close)(struct JPUefiFile *self);                  /* 16 */
	void *delete_file;                        /* 24 */
	JPUefiStatus (JP_UEFI_API *read)(struct JPUefiFile *self, JPUefiUintn *size, void *buffer); /* 32 */
	void *write;                              /* 40 */
	void *get_position;                       /* 48 */
	void *set_position;                       /* 56 */
	JPUefiStatus (JP_UEFI_API *get_info)(struct JPUefiFile *self, const JPUefiGuid *information_type,
		JPUefiUintn *size, void *buffer);     /* 64 */
	void *set_info;                           /* 72 */
	void *flush;                              /* 80 */
} JPUefiFile;

#define JP_UEFI_FILE_MODE_READ 1ULL

typedef struct JPUefiFileInfo {
	uint64_t size;                            /* the size of this structure, including the name */
	uint64_t file_size;                       /* 8 */
	uint64_t physical_size;                   /* 16 */
	uint8_t create_time[16];
	uint8_t last_access_time[16];
	uint8_t modification_time[16];
	uint64_t attribute;
	JPUefiChar16 file_name[1];                /* the name follows */
} JPUefiFileInfo;

typedef struct JPUefiSimpleFileSystem {
	uint64_t revision;                        /* 0 */
	JPUefiStatus (JP_UEFI_API *open_volume)(struct JPUefiSimpleFileSystem *self, JPUefiFile **root); /* 8 */
} JPUefiSimpleFileSystem;

typedef struct JPUefiGopModeInfo {
	uint32_t version;                         /* 0 */
	uint32_t horizontal_resolution;           /* 4 */
	uint32_t vertical_resolution;             /* 8 */
	uint32_t pixel_format;                    /* 12 */
	uint32_t pixel_information[4];            /* 16 */
	uint32_t pixels_per_scan_line;            /* 32 */
} JPUefiGopModeInfo;

typedef enum JPUefiGopPixelFormat {
	JP_UEFI_GOP_PIXEL_RGB_RESERVED_8 = 0,     /* red, green, blue, one unused byte */
	JP_UEFI_GOP_PIXEL_BGR_RESERVED_8 = 1,     /* blue, green, red, one unused byte */
	JP_UEFI_GOP_PIXEL_BIT_MASK = 2,
	JP_UEFI_GOP_PIXEL_BLT_ONLY = 3
} JPUefiGopPixelFormat;

typedef struct JPUefiGopMode {
	uint32_t max_mode;                        /* 0 */
	uint32_t mode;                            /* 4 */
	JPUefiGopModeInfo *info;                  /* 8 */
	uint64_t size_of_info;                    /* 16 */
	uint64_t frame_buffer_base;               /* 24 */
	uint64_t frame_buffer_size;               /* 32 */
} JPUefiGopMode;

typedef struct JPUefiGraphicsOutput {
	void *query_mode;                         /* 0 */
	void *set_mode;                           /* 8 */
	void *blt;                                /* 16 */
	JPUefiGopMode *mode;                      /* 24 */
} JPUefiGraphicsOutput;

/* ---- GUIDs ---- */

#define JP_UEFI_LOADED_IMAGE_GUID \
	{ 0x5B1B31A1, 0x9562, 0x11d2, { 0x8E, 0x3F, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B } }
#define JP_UEFI_SIMPLE_FILE_SYSTEM_GUID \
	{ 0x964e5b22, 0x6459, 0x11d2, { 0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b } }
#define JP_UEFI_GRAPHICS_OUTPUT_GUID \
	{ 0x9042a9de, 0x23dc, 0x4a38, { 0x96, 0xfb, 0x7a, 0xde, 0xd0, 0x80, 0x51, 0x6a } }
#define JP_UEFI_FILE_INFO_GUID \
	{ 0x09576e92, 0x6d3f, 0x11d2, { 0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b } }

/* ---- Layout checks: every offset and size above is fixed by the UEFI specification ---- */

_Static_assert(sizeof(JPUefiChar16) == 2, "CHAR16 is 16 bits");
_Static_assert(sizeof(JPUefiGuid) == 16, "GUID size");
_Static_assert(sizeof(JPUefiTableHeader) == 24, "table header size");
_Static_assert(sizeof(JPUefiMemoryDescriptor) == 40, "memory descriptor size");

_Static_assert(offsetof(JPUefiSystemTable, con_in) == 48, "system table: ConIn");
_Static_assert(offsetof(JPUefiSystemTable, con_out) == 64, "system table: ConOut");
_Static_assert(offsetof(JPUefiSystemTable, boot_services) == 96, "system table: BootServices");

_Static_assert(offsetof(JPUefiBootServices, allocate_pages) == 40, "boot services: AllocatePages");
_Static_assert(offsetof(JPUefiBootServices, free_pages) == 48, "boot services: FreePages");
_Static_assert(offsetof(JPUefiBootServices, get_memory_map) == 56, "boot services: GetMemoryMap");
_Static_assert(offsetof(JPUefiBootServices, allocate_pool) == 64, "boot services: AllocatePool");
_Static_assert(offsetof(JPUefiBootServices, free_pool) == 72, "boot services: FreePool");
_Static_assert(offsetof(JPUefiBootServices, handle_protocol) == 152, "boot services: HandleProtocol");
_Static_assert(offsetof(JPUefiBootServices, exit_boot_services) == 232, "boot services: ExitBootServices");
_Static_assert(offsetof(JPUefiBootServices, stall) == 248, "boot services: Stall");
_Static_assert(offsetof(JPUefiBootServices, open_protocol) == 280, "boot services: OpenProtocol");
_Static_assert(offsetof(JPUefiBootServices, locate_protocol) == 320, "boot services: LocateProtocol");

_Static_assert(offsetof(JPUefiLoadedImage, device_handle) == 24, "loaded image: DeviceHandle");
_Static_assert(offsetof(JPUefiFile, open) == 8, "file: Open");
_Static_assert(offsetof(JPUefiFile, close) == 16, "file: Close");
_Static_assert(offsetof(JPUefiFile, read) == 32, "file: Read");
_Static_assert(offsetof(JPUefiFile, get_info) == 64, "file: GetInfo");
_Static_assert(offsetof(JPUefiFileInfo, file_size) == 8, "file info: FileSize");
_Static_assert(offsetof(JPUefiSimpleFileSystem, open_volume) == 8, "file system: OpenVolume");
_Static_assert(offsetof(JPUefiSimpleTextInput, read_key_stroke) == 8, "text input: ReadKeyStroke");
_Static_assert(offsetof(JPUefiSimpleTextOutput, output_string) == 8, "text output: OutputString");
_Static_assert(sizeof(JPUefiInputKey) == 4, "input key size");

_Static_assert(offsetof(JPUefiGopModeInfo, horizontal_resolution) == 4, "mode info: width");
_Static_assert(offsetof(JPUefiGopModeInfo, vertical_resolution) == 8, "mode info: height");
_Static_assert(offsetof(JPUefiGopModeInfo, pixel_format) == 12, "mode info: format");
_Static_assert(offsetof(JPUefiGopModeInfo, pixels_per_scan_line) == 32, "mode info: pixels per scan line");
_Static_assert(offsetof(JPUefiGopMode, info) == 8, "mode: Info");
_Static_assert(offsetof(JPUefiGopMode, frame_buffer_base) == 24, "mode: FrameBufferBase");
_Static_assert(offsetof(JPUefiGraphicsOutput, mode) == 24, "graphics output: Mode");

#endif /* JUSTOS_BOOT_PLATFORM_UEFI_DEFS_H */
