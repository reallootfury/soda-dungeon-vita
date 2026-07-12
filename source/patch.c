/*
 * Soda Dungeon · PSVita Port — binary patches.
 *
 * Per-game runtime patches applied after the .so is relocated and resolved
 * but before its init_array runs. These fix assumptions the Android binary
 * makes that don't hold on the Vita (e.g. hard-coded paths, timing calls).
 *
 * The FalsoJNI bridge and symbol resolver handle most Android/Vita differences;
 * binary-specific fixes that cannot be expressed as imported symbols live here.
 *
 * Copyright (C) 2023      Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#include <kubridge.h>
#include <so_util/so_util.h>
#include <psp2/ctrl.h>

#include "reimpl/sys.h"
#include "cheats.h"
#include "utils/logger.h"

extern so_module so_mod;       // liblime-legacy.so
extern so_module so_mod_game;  // libApplicationMain.so

/* Soda Dungeon 1.2.44 Main_obj boot instructions. These are patched before
 * HXCPP_main so the UI is built with unsupported ads/IAP hidden and internal
 * debug flags available to the Vita-native cheat panel. */
typedef struct {
    uint32_t offset;
    uint32_t expected;
    uint32_t replacement;
    const char *description;
} SodaInstructionPatch;

static const SodaInstructionPatch soda_boot_patches[] = {
    { 0x004749DC, 0xE5C47F5C, 0xE5C45F5C, "SHOW_IAP=false" },
    { 0x004749E0, 0xE5C47F5E, 0xE5C45F5E, "ALLOW_IAP=false" },
    { 0x004749EC, 0xE5C45004, 0xE5C47004, "ALLOW_DEBUG=true" },
    { 0x004749F0, 0xE5C45F75, 0xE5C47F75, "SKIP_SPLASH=true" },
    { 0x004749FC, 0xE5C47F5F, 0xE5C45F5F, "ALLOW_ADS=false" },
};

#define MAIN_ON_KEY_DOWN_OFFSET 0x004639B4
#define MAIN_STATIC_BASE_OFFSET 0x006452F0
#define MAIN_CHEAP_MODE_OFFSET  (MAIN_STATIC_BASE_OFFSET + 0xF6C)
#define MAIN_AUTO_ARENA_OFFSET  (MAIN_STATIC_BASE_OFFSET + 0xF6D)
#define MAIN_ALWAYS_CRIT_OFFSET (MAIN_STATIC_BASE_OFFSET + 0xF74)
#define MAIN_ALWAYS_PATRONS_OFFSET (MAIN_STATIC_BASE_OFFSET + 0xF5D)
#define MAIN_SHOW_REG_POINTS_OFFSET (MAIN_STATIC_BASE_OFFSET + 0xF60)

typedef void *(*HxCffiResolver)(const char *name);
typedef int (*CffiValId)(const char *name);
typedef void *(*CffiValField)(void *object, int field);
typedef void *(*CffiValOCall1)(void *object, int field, void *argument);
typedef void *(*CffiAllocInt)(int value);
typedef void (*CffiGcSetTopOfStack)(int *top, int force);
typedef int (*CffiValFunNargs)(void *function);
typedef void *(*CffiValCall0)(void *function);
typedef void *(*CffiValCall1)(void *function, void *argument);

static so_hook main_on_key_down_hook;
static CffiValId cffi_val_id;
static CffiValField cffi_val_field;
static CffiValOCall1 cffi_val_ocall1;
static CffiAllocInt cffi_alloc_int;
static CffiGcSetTopOfStack cffi_gc_set_top_of_stack;
static CffiValFunNargs cffi_val_fun_nargs;
static CffiValCall0 cffi_val_call0;
static CffiValCall1 cffi_val_call1;
static int earn_gold_field = -1;
static int spend_caps_field = -1;
static int spend_essence_field = -1;
static int debug_menu_field = -1;
static int click_debug_field = -1;
static int close_field = -1;
static void *last_application_main;
static int native_debug_open;
static int native_debug_selection;
static int native_debug_wait_release;
static unsigned int native_debug_generation;
static int ad_handlers_disabled;

static void *get_main_instance(void)
{
    /* Main.onKeyDown is an ApplicationMain_obj method.  The receiver captured
     * by the hook is therefore already the live object exposing earnGold,
     * spendCaps, and the ad callbacks.  Its `main` field is a static entry
     * closure, not another instance. */
    return last_application_main;
}

static uintptr_t disabled_haxe_handler(void *result)
{
    if (result)
        *(void **)result = NULL;
    return (uintptr_t)result;
}

static int disable_ad_handler(void *main_instance, const char *name)
{
    int field = cffi_val_id(name);
    void *closure = cffi_val_field(main_instance, field);
    if (!closure)
        return 0;
    uintptr_t wrapper = *(uintptr_t *)((uint8_t *)closure + 8);
    if (wrapper < so_mod_game.text_base ||
        wrapper >= so_mod_game.text_base + so_mod_game.text_size)
        return 0;
    hook_arm(wrapper, (uintptr_t)&disabled_haxe_handler);
    kuKernelFlushCaches((void *)wrapper, 8);
    l_info("Disabled unsupported ad handler %s at %p.", name, (void *)wrapper);
    return 1;
}

static void disable_reward_ads(void)
{
    if (ad_handlers_disabled || !cffi_val_id || !cffi_val_field)
        return;
    void *main_instance = get_main_instance();
    if (!main_instance)
        return;
    int disabled = 0;
    disabled += disable_ad_handler(main_instance, "click_btnWatchAd");
    disabled += disable_ad_handler(main_instance, "onRewardYes");
    disabled += disable_ad_handler(main_instance, "showGameAd");
    if (disabled)
        ad_handlers_disabled = 1;
}

static int read_cheat_flag(uintptr_t offset)
{
    uint8_t value = 0;
    kuKernelCpuUnrestrictedMemcpy(&value,
        (const void *)(so_mod_game.text_base + offset), sizeof(value));
    return value != 0;
}

static void toggle_cheat_flag(uintptr_t offset)
{
    uint8_t value = !read_cheat_flag(offset);
    kuKernelCpuUnrestrictedMemcpy(
        (void *)(so_mod_game.text_base + offset), &value, sizeof(value));
}

static void call_currency_method(int field, int amount)
{
    void *main_instance = get_main_instance();
    if (!main_instance || !cffi_val_ocall1 || !cffi_alloc_int ||
        !cffi_gc_set_top_of_stack || field < 0)
        return;
    /* Android detaches its UI thread from HXCPP after each callback.  Loader
     * menu actions happen between callbacks, so temporarily register this
     * stack before allocating a Dynamic or invoking a Haxe method. */
    int stack_top;
    cffi_gc_set_top_of_stack(&stack_top, 1);
    void *argument = cffi_alloc_int(amount);
    if (argument)
        cffi_val_ocall1(main_instance, field, argument);
    cffi_gc_set_top_of_stack(NULL, 1);
}

static int call_reflected(void *object, int field)
{
    if (!object || field < 0 || !cffi_val_field || !cffi_val_fun_nargs ||
        !cffi_val_call0 || !cffi_val_call1)
        return 0;
    void *function = cffi_val_field(object, field);
    if (!function)
        return 0;
    int nargs = cffi_val_fun_nargs(function);
    if (nargs == 0)
        cffi_val_call0(function);
    else if (nargs == 1)
        cffi_val_call1(function, NULL);
    else
        return 0;
    return 1;
}

static int call_debug_cheat(const char *name)
{
    void *main_instance = get_main_instance();
    if (!main_instance || !cffi_gc_set_top_of_stack || !cffi_val_id ||
        !cffi_val_field || debug_menu_field < 0 || click_debug_field < 0)
        return 0;
    int stack_top;
    cffi_gc_set_top_of_stack(&stack_top, 1);
    void *debug_menu = cffi_val_field(main_instance, debug_menu_field);
    if (!debug_menu) {
        call_reflected(main_instance, click_debug_field);
        debug_menu = cffi_val_field(main_instance, debug_menu_field);
    }
    int called = debug_menu && call_reflected(debug_menu, cffi_val_id(name));
    if (debug_menu)
        call_reflected(debug_menu, close_field);
    cffi_gc_set_top_of_stack(NULL, 1);
    if (!called)
        l_warn("Native cheat action '%s' was unavailable.", name);
    return called;
}

static uintptr_t main_on_key_down_intercept(void *main_object, void *event)
{
    last_application_main = main_object;
    disable_reward_ads();
    uintptr_t result = SO_CONTINUE(uintptr_t, main_on_key_down_hook,
                                   main_object, event);
    return result;
}

void cheats_runtime_init(void)
{
    HxCffiResolver hx_cffi =
        (HxCffiResolver)so_symbol(&so_mod_game, "hx_cffi");
    if (!hx_cffi)
        return;
    cffi_val_id = (CffiValId)hx_cffi("val_id");
    cffi_val_field = (CffiValField)hx_cffi("val_field");
    cffi_val_ocall1 = (CffiValOCall1)hx_cffi("val_ocall1");
    cffi_alloc_int = (CffiAllocInt)hx_cffi("alloc_int");
    cffi_gc_set_top_of_stack =
        (CffiGcSetTopOfStack)hx_cffi("gc_set_top_of_stack");
    cffi_val_fun_nargs = (CffiValFunNargs)hx_cffi("val_fun_nargs");
    cffi_val_call0 = (CffiValCall0)hx_cffi("val_call0");
    cffi_val_call1 = (CffiValCall1)hx_cffi("val_call1");
    if (cffi_val_id && cffi_val_field && cffi_val_ocall1 &&
        cffi_alloc_int && cffi_gc_set_top_of_stack && cffi_val_fun_nargs &&
        cffi_val_call0 && cffi_val_call1) {
        earn_gold_field = cffi_val_id("earnGold");
        spend_caps_field = cffi_val_id("spendCaps");
        spend_essence_field = cffi_val_id("spendEssence");
        debug_menu_field = cffi_val_id("debugMenu");
        click_debug_field = cffi_val_id("click_btnDebug");
        close_field = cffi_val_id("close");
        l_info("Native cheat panel initialized.");
    }
}

void cheats_request_debug_menu(void)
{
    native_debug_open = !native_debug_open;
    native_debug_wait_release = native_debug_open;
    native_debug_generation++;
    l_info("Native cheat panel %s.", native_debug_open ? "opened" : "closed");
}

void cheats_poll(void)
{
    disable_reward_ads();
}

uint32_t cheats_handle_input(uint32_t pressed, uint32_t held)
{
    if (!native_debug_open)
        return 0;
    uint32_t consumed = SCE_CTRL_UP | SCE_CTRL_DOWN | SCE_CTRL_LEFT |
        SCE_CTRL_RIGHT | SCE_CTRL_CROSS | SCE_CTRL_CIRCLE |
        SCE_CTRL_SQUARE | SCE_CTRL_TRIANGLE | SCE_CTRL_START |
        SCE_CTRL_SELECT;
    if (native_debug_wait_release) {
        if (!(held & consumed))
            native_debug_wait_release = 0;
        return consumed;
    }
    if (pressed & SCE_CTRL_UP) {
        native_debug_selection =
            (native_debug_selection + CHEAT_MENU_COUNT - 1) % CHEAT_MENU_COUNT;
        native_debug_wait_release = 1;
        native_debug_generation++;
    }
    if (pressed & SCE_CTRL_DOWN) {
        native_debug_selection = (native_debug_selection + 1) % CHEAT_MENU_COUNT;
        native_debug_wait_release = 1;
        native_debug_generation++;
    }
    if (pressed & SCE_CTRL_CIRCLE) {
        native_debug_open = 0;
        native_debug_generation++;
    } else if (pressed & SCE_CTRL_CROSS) {
        switch (native_debug_selection) {
            case 0: call_currency_method(earn_gold_field, 1000000); break;
            case 1: call_currency_method(earn_gold_field, 2000000000); break;
            case 2: call_currency_method(spend_caps_field, -10000); break;
            case 3: call_debug_cheat("chkMaxCaps"); break;
            case 4: call_currency_method(spend_essence_field, -10000); break;
            case 5: call_debug_cheat("chkXmasItems"); break;
            case 6: call_debug_cheat("chkAllUpgrades"); break;
            case 7: call_debug_cheat("chkSuperRelics"); break;
            case 8: call_debug_cheat("chkActivatePortal"); break;
            case 9: call_debug_cheat("chkEasyBosses"); break;
            case 10: call_debug_cheat("chkAutoParty"); break;
            case 11: call_debug_cheat("chkSkipIntro"); break;
            case 12: toggle_cheat_flag(MAIN_ALWAYS_PATRONS_OFFSET); break;
            case 13: toggle_cheat_flag(MAIN_SHOW_REG_POINTS_OFFSET); break;
            case 14: toggle_cheat_flag(MAIN_ALWAYS_CRIT_OFFSET); break;
            case 15: toggle_cheat_flag(MAIN_CHEAP_MODE_OFFSET); break;
            case 16: toggle_cheat_flag(MAIN_AUTO_ARENA_OFFSET); break;
            case 17: call_debug_cheat("click_btnLevelRight"); break;
            case 18: call_debug_cheat("click_btnLevelRight10"); break;
            case 19: call_debug_cheat("click_btnLevelRight100"); break;
            case 20: call_debug_cheat("click_btnDimRight"); break;
            case 21: native_debug_open = 0; break;
        }
        native_debug_wait_release = 1;
        native_debug_generation++;
    }
    return consumed;
}

int cheats_menu_is_open(void) { return native_debug_open; }
int cheats_menu_selection(void) { return native_debug_selection; }
unsigned int cheats_menu_generation(void) { return native_debug_generation; }
int cheats_flag_cheap(void) { return read_cheat_flag(MAIN_CHEAP_MODE_OFFSET); }
int cheats_flag_crit(void) { return read_cheat_flag(MAIN_ALWAYS_CRIT_OFFSET); }
int cheats_flag_auto(void) { return read_cheat_flag(MAIN_AUTO_ARENA_OFFSET); }
int cheats_flag_patrons(void) { return read_cheat_flag(MAIN_ALWAYS_PATRONS_OFFSET); }
int cheats_flag_registration(void) { return read_cheat_flag(MAIN_SHOW_REG_POINTS_OFFSET); }

/* Android's ARM Linux kernel exposes this atomic helper at a fixed address.
 * Vita does not map that helper page, so replace its literal references before
 * any .so init-array code can call through them. */
#define KUSER_CMPXCHG_ADDR 0xffff0fc0u

void so_patch_kuser_helpers(so_module *mod)
{
    const uintptr_t replacement = (uintptr_t)&__atomic_cmpxchg;
    unsigned int patched = 0;

    for (size_t offset = 0; offset + sizeof(uint32_t) <= mod->text_size;
         offset += sizeof(uint32_t)) {
        uintptr_t address = mod->text_base + offset;
        uint32_t value;

        kuKernelCpuUnrestrictedMemcpy(&value, (const void *)address,
                                      sizeof(value));
        if (value != KUSER_CMPXCHG_ADDR)
            continue;

        kuKernelCpuUnrestrictedMemcpy((void *)address, &replacement,
                                      sizeof(replacement));
        patched++;
    }

    if (patched) {
        kuKernelFlushCaches((void *)mod->text_base, mod->text_size);
        l_info("Patched %u Android kuser cmpxchg helper(s).", patched);
    }
}

void so_patch(void)
{
    unsigned int applied = 0;
    for (size_t i = 0; i < sizeof(soda_boot_patches) /
                            sizeof(soda_boot_patches[0]); i++) {
        const SodaInstructionPatch *patch = &soda_boot_patches[i];
        uintptr_t address = so_mod_game.text_base + patch->offset;
        uint32_t current;
        kuKernelCpuUnrestrictedMemcpy(&current, (const void *)address,
                                      sizeof(current));
        if (current != patch->expected) {
            l_error("Soda 1.2.44 patch mismatch at 0x%08X (%s): 0x%08X.",
                    patch->offset, patch->description, current);
            continue;
        }
        kuKernelCpuUnrestrictedMemcpy((void *)address, &patch->replacement,
                                      sizeof(patch->replacement));
        applied++;
    }
    if (applied) {
        kuKernelFlushCaches((void *)so_mod_game.text_base,
                            so_mod_game.text_size);
        l_info("Applied %u Soda Dungeon feature patches.", applied);
    }

    uintptr_t key_handler = so_mod_game.text_base + MAIN_ON_KEY_DOWN_OFFSET;
    uint32_t prologue[2];
    kuKernelCpuUnrestrictedMemcpy(prologue, (const void *)key_handler,
                                  sizeof(prologue));
    if (prologue[0] == 0xE92D4FF0 && prologue[1] == 0xE1A05000) {
        main_on_key_down_hook =
            hook_arm(key_handler, (uintptr_t)&main_on_key_down_intercept);
        kuKernelFlushCaches((void *)key_handler, sizeof(prologue));
        l_info("Installed Main.onKeyDown debug-menu hook.");
    } else {
        l_error("Main.onKeyDown patch mismatch: 0x%08X 0x%08X.",
                prologue[0], prologue[1]);
    }
}
