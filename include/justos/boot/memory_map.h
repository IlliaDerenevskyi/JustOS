/*
 * memory_map.h - normalized memory-map contract.
 *
 * Purpose:
 *   Public normalized memory-map interface.
 *
 * Responsibilities:
 *   - acquire the memory map;
 *   - validate normalized regions.
 *
 * Limitations:
 *   Declarations only. Provider-native memory descriptors do not escape.
 *
 * Public API:
 *   jp_boot_discover_memory, jp_boot_validate_memory_map.
 *
 * Depends on:
 *   justos/boot/boot_types.h.
 *
 * Must not depend on:
 *   Provider-native types.
 */

#ifndef JUSTOS_BOOT_MEMORY_MAP_H
#define JUSTOS_BOOT_MEMORY_MAP_H

#include "justos/boot/boot_types.h"

JPBootResult jp_boot_discover_memory(
	JPBootContext *context
);

JPBootResult jp_boot_validate_memory_map(
	JPBootContext *context
);

#endif /* JUSTOS_BOOT_MEMORY_MAP_H */
