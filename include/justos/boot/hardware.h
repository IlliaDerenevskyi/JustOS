/*
 * hardware.h - hardware discovery contract.
 *
 * Purpose:
 *   Public hardware discovery interface.
 *
 * Responsibilities:
 *   - CPU, firmware and boot-device discovery;
 *   - normalization into JPHardwareInfo (context->hardware).
 *
 * Limitations:
 *   Declarations only. No provider-native hardware types are exposed.
 *
 * Public API:
 *   jp_boot_discover_hardware.
 *
 * Depends on:
 *   justos/boot/boot_types.h.
 *
 * Must not depend on:
 *   Provider-native types.
 */

#ifndef JUSTOS_BOOT_HARDWARE_H
#define JUSTOS_BOOT_HARDWARE_H

#include "justos/boot/boot_types.h"

JPBootResult jp_boot_discover_hardware(
	JPBootContext *context
);

#endif /* JUSTOS_BOOT_HARDWARE_H */
