/*
 * bios_block.h - the boot data block that the BIOS loader leaves for the
 * 64-bit code.
 *
 * Purpose:
 *   The 16-bit loader and bios.c (64-bit) must agree on one fixed layout; this
 *   header is that agreement and the only place where the layout is written.
 *
 * Limitations:
 *   Little-endian, no padding (checked with static assertions). Declarations
 *   only.
 *
 * Depends on:
 *   <stdint.h>.
 *
 * Must not depend on:
 *   Any firmware header.
 */

#ifndef JUSTOS_BOOT_PLATFORM_BIOS_BLOCK_H
#define JUSTOS_BOOT_PLATFORM_BIOS_BLOCK_H

#include <stddef.h>
#include <stdint.h>

#define JP_BIOS_BLOCK_ADDRESS 0x00010000
#define JP_BIOS_BLOCK_MAGIC "JUSTBLK1"      /* 8 bytes, no end marker is stored */
#define JP_BIOS_BLOCK_MAGIC_SIZE 8
#define JP_BIOS_BLOCK_VERSION 1
#define JP_BIOS_BLOCK_MAX_E820 128
#define JP_BIOS_BLOCK_MAX_SIZE 16384

#define JP_BIOS_BLOCK_FLAG_A20 0x1           /* the A20 line is enabled */
#define JP_BIOS_BLOCK_FLAG_VIDEO 0x2         /* the video fields are valid */
#define JP_BIOS_BLOCK_FLAG_KERNEL 0x4        /* the kernel fields are valid */

#define JP_BIOS_VIDEO_FORMAT_RGB 1
#define JP_BIOS_VIDEO_FORMAT_BGR 2

typedef struct JPBiosE820Entry {
	uint64_t base;
	uint64_t length;
	uint32_t type;       /* 1 usable, 2 reserved, 3 ACPI reclaimable, 4 ACPI NVS, 5 bad memory */
	uint32_t attributes;
} JPBiosE820Entry;

typedef struct JPBiosBlock {
	char magic[JP_BIOS_BLOCK_MAGIC_SIZE];
	uint32_t version;
	uint32_t flags;
	uint32_t boot_drive;
	uint32_t e820_count;
	JPBiosE820Entry e820[JP_BIOS_BLOCK_MAX_E820];
	uint64_t framebuffer_base;
	uint32_t width;
	uint32_t height;
	uint32_t pitch;
	uint32_t video_format;
	uint64_t kernel_address;   /* where the loader left kernel.elf */
	uint64_t kernel_size;
} JPBiosBlock;

_Static_assert(sizeof(JPBiosE820Entry) == 24, "E820 entry is 24 bytes");
_Static_assert(offsetof(JPBiosBlock, version) == 8, "version offset");
_Static_assert(offsetof(JPBiosBlock, flags) == 12, "flags offset");
_Static_assert(offsetof(JPBiosBlock, boot_drive) == 16, "boot_drive offset");
_Static_assert(offsetof(JPBiosBlock, e820_count) == 20, "e820_count offset");
_Static_assert(offsetof(JPBiosBlock, e820) == 24, "e820 offset");
_Static_assert(offsetof(JPBiosBlock, framebuffer_base) == 3096, "framebuffer_base offset");
_Static_assert(offsetof(JPBiosBlock, width) == 3104, "width offset");
_Static_assert(offsetof(JPBiosBlock, height) == 3108, "height offset");
_Static_assert(offsetof(JPBiosBlock, pitch) == 3112, "pitch offset");
_Static_assert(offsetof(JPBiosBlock, video_format) == 3116, "video_format offset");
_Static_assert(offsetof(JPBiosBlock, kernel_address) == 3120, "kernel_address offset");
_Static_assert(offsetof(JPBiosBlock, kernel_size) == 3128, "kernel_size offset");
_Static_assert(sizeof(JPBiosBlock) == 3136, "block size");
_Static_assert(sizeof(JPBiosBlock) <= JP_BIOS_BLOCK_MAX_SIZE, "block fits in 16 KiB");

#endif /* JUSTOS_BOOT_PLATFORM_BIOS_BLOCK_H */
