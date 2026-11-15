#include <stdio.h>
#include <assert.h>
#include "justos/boot/boot_manager.h"

int main(void) {
    JPBootContext boot_ctx;
    
    JPBootResult res = jp_boot_initialize(&boot_ctx);
    assert(res == JP_BOOT_SUCCESS);

    res = jp_boot_discover_hardware(&boot_ctx);
    assert(res == JP_BOOT_SUCCESS);

    jp_boot_render_bios_ui(&boot_ctx, "General");

    printf("\n\n");

    jp_boot_print_report(&boot_ctx);

    return 0;
}