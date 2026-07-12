/*
 * Soda Dungeon · PSVita Port — loader bootstrap.
 *
 * Loads the two native libraries that make up the game and prepares the
 * runtime environment they expect:
 *
 *   - liblime-legacy.so    (the lime framework host)
 *   - libApplicationMain.so (the compiled Haxe game code)
 *
 * Each is mapped at a distinct base address, relocated, has its Android/bionic
 * imports resolved against our Vita shim layer, is patched, and finally has its
 * ELF init_array executed. After this returns, main() can call into lime.
 *
 * Copyright (C) 2021      Andy Nguyen
 * Copyright (C) 2021-2022 Rinnegatamante
 * Copyright (C) 2022-2024 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#include "utils/init.h"

#include "utils/dialog.h"
#include "utils/glutil.h"
#include "utils/logger.h"
#include "utils/utils.h"
#include "utils/settings.h"

#include <string.h>

#include <psp2/appmgr.h>
#include <psp2/apputil.h>
#include <psp2/kernel/clib.h>
#include <psp2/power.h>

#include <falso_jni/FalsoJNI.h>
#include <so_util/so_util.h>
#include <fios/fios.h>

/*
 * Base addresses for the two Android .so images. They must be page-aligned
 * and far enough apart that the larger image (libApplicationMain.so, ~6.5 MB)
 * never overlaps the other. 0x98000000 + 0x01000000 (16 MB) gives comfortable
 * headroom for both, well inside the Vita's user address space.
 */
#define LOAD_ADDRESS_LIME 0x98000000ULL
#define LOAD_ADDRESS_GAME 0x99000000ULL

extern so_module so_mod;       // liblime-legacy.so
extern so_module so_mod_game;  // libApplicationMain.so

/*
 * The Android Java bootstrap passes ApplicationMain's hx_cffi resolver to
 * Lime before Lime requests any CFFI API. Our loader has no Java layer, so
 * register it directly while both .so images are mapped but before Lime's
 * init-array code runs.
 */
static int register_hxcpp_cffi_loader(void)
{
    typedef void (*HxSetLoader)(void *loader);

    HxSetLoader hx_set_loader =
        (HxSetLoader)so_symbol(&so_mod, "hx_set_loader");
    void *hx_cffi = (void *)so_symbol(&so_mod_game, "hx_cffi");

    if (!hx_set_loader || !hx_cffi) {
        l_fatal("Could not resolve the Haxe CFFI loader (%p, %p).",
                hx_set_loader, hx_cffi);
        return -1;
    }

    hx_set_loader(hx_cffi);
    l_success("Haxe CFFI loader registered.");
    return 0;
}

/*
 * Helper: load, relocate, resolve, patch, flush and initialise a single .so.
 * Returns 0 on success, -1 on failure (with a fatal dialog already shown).
 */
static int load_one_so(so_module *mod, const char *path, uintptr_t load_addr,
                       const char *display_name)
{
    if (!file_exists(path)) {
        fatal_error("%s was not found at %s. Please make sure you have copied "
                    "all of the game's data files into " DATA_PATH ".",
                    display_name, path);
        return -1;
    }

    if (so_file_load(mod, path, load_addr) < 0) {
        l_fatal("could not load %s (%s).", display_name, path);
        fatal_error("Error: could not load %s.", path);
        return -1;
    }

    so_relocate(mod);
    so_patch_kuser_helpers(mod);
    resolve_imports(mod);

    if (mod == &so_mod && register_hxcpp_cffi_loader() < 0) {
        fatal_error("Could not initialize Soda Dungeon's Haxe runtime.");
        return -1;
    }

    // so_patch() touches both modules; called once from the caller after both
    // are resolved so cross-module hooks can resolve either set of symbols.

    so_flush_caches(mod);
    so_initialize(mod);
    l_success("%s loaded and initialised.", display_name);
    return 0;
}

void soloader_init_all(void)
{
    // Launch `app0:configurator.bin` on `-config` init param
    sceAppUtilInit(&(SceAppUtilInitParam){}, &(SceAppUtilBootParam){});
    SceAppUtilAppEventParam eventParam;
    sceClibMemset(&eventParam, 0, sizeof(SceAppUtilAppEventParam));
    sceAppUtilReceiveAppEvent(&eventParam);
    if (eventParam.type == 0x05) {
        char buffer[2048];
        sceAppUtilAppEventParseLiveArea(&eventParam, buffer);
        if (strstr(buffer, "-config"))
            sceAppMgrLoadExec("app0:/configurator.bin", NULL, NULL);
    }

    // Overclock to the game's expected profile
    scePowerSetArmClockFrequency(444);
    scePowerSetBusClockFrequency(222);
    scePowerSetGpuClockFrequency(222);
    scePowerSetGpuXbarClockFrequency(166);

#ifdef USE_SCELIBC_IO
    if (fios_init(DATA_PATH) == 0)
        l_success("FIOS initialized.");
#endif

    if (!module_loaded("kubridge")) {
        l_fatal("kubridge is not loaded.");
        fatal_error("Error: kubridge.skprx is not installed.");
    }
    l_success("kubridge check passed.");

    settings_load();
    l_success("Settings loaded.");

    // --- Load both native libraries. ---
    // Order matters only for symbol resolution coherency; so_util resolves
    // each module independently against the shared dynlib table, so the game
    // .so's imports (which are plain libc/log) resolve the same way as lime's.
    if (load_one_so(&so_mod_game, SO_GAME_PATH, LOAD_ADDRESS_GAME,
                    "libApplicationMain.so") < 0)
        return;

    if (load_one_so(&so_mod, SO_PATH, LOAD_ADDRESS_LIME,
                    "liblime-legacy.so") < 0)
        return;

    // Apply runtime patches now that both modules' symbols are resolvable.
    so_patch();
    l_success("SO patched.");

    gl_preload();
    l_success("OpenGL preloaded.");

    jni_init();
    l_success("FalsoJNI initialized.");
}
