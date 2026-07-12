/*
 * Soda Dungeon · PSVita Port — loader entry point.
 *
 * Soda Dungeon's Android build is a Haxe/OpenFL ("lime-legacy") application.
 * Its native side is split across two cooperating ARM .so files shipped in the
 * APK under lib/armeabi/:
 *
 *   liblime-legacy.so    the lime framework host. Exports JNI_OnLoad plus a set
 *                        of Java_org_haxe_lime_Lime_* entry points that the
 *                        Java side (GameActivity / MainView) drives from the
 *                        UI thread: onRender, onResize, onTouch, onKeyChange,
 *                        onPoll, onCallback, ...
 *
 *   libApplicationMain.so the compiled Haxe game code. Exports a single entry
 *                        point, Java_org_haxe_HXCPP_main, which lime calls back
 *                        into through the JNI bridge once the framework is up.
 *
 * The boot sequence on Android is:
 *   1. Java loads liblime-legacy.so and calls JNI_OnLoad.
 *   2. lime, during JNI_OnLoad, dlopens libApplicationMain.so and registers the
 *      Haxe runtime via HXCPP_main.
 *   3. The Java Renderer loop calls Lime.onRender / Lime.onResize / Lime.onPoll
 *      every frame, and forwards input through Lime.onTouch / Lime.onKeyChange.
 *
 * We reproduce that sequence here. We load both .so files with so_util at fixed
 * addresses, resolve their bionic/Android imports against the Vita newlib +
 * FalsoJNI + vitaGL, then drive lime's render entry points from our own loop.
 *
 * Copyright (C) 2021      Andy Nguyen
 * Copyright (C) 2021      Rinnegatamante
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

#include <psp2/kernel/threadmgr.h>

#include <falso_jni/FalsoJNI.h>
#include <falso_jni/FalsoJNI_ImplBridge.h>
#include <so_util/so_util.h>

#include <pthread.h>
#include "reimpl/controls.h"
#include "reimpl/sys.h"
#include "cheats.h"

int _newlib_heap_size_user = 256 * 1024 * 1024;

#ifdef USE_SCELIBC_IO
int sceLibcHeapSize = 4 * 1024 * 1024;
#endif

#define SCREEN_WIDTH 960
#define SCREEN_HEIGHT 544

/*
 * liblime-legacy.so statically contains an old openal-android backend.  Both
 * Lime and that backend originally defined JNI_OnLoad, but the linker kept
 * only Lime's public entry point.  Consequently Lime receives the JavaVM
 * while OpenAL's private javaVM slot remains NULL and alcOpenDevice crashes
 * when it tries to call AttachCurrentThread.
 *
 * This offset is the openal-android javaVM slot in Soda Dungeon 1.2.44's
 * liblime-legacy.so.  The port intentionally supports that exact APK build.
 */
#define LIME_LOAD_ADDRESS            0x98000000u
#define OPENAL_ANDROID_JAVAVM_OFFSET 0x0047BD7Cu

// lime-legacy is the framework host. Its JNI_OnLoad brings up the lime runtime
// and pulls in libApplicationMain.so (the game) via dlopen.
so_module so_mod;

// libApplicationMain.so holds the compiled Haxe game. It is loaded at a second
// fixed address so the two images don't collide. lime's JNI_OnLoad references
// it through the dlopen/dlsym shim we provide in dynlib.c.
so_module so_mod_game;

// Frame loop function pointers, resolved from liblime-legacy.so after load.
// These correspond 1:1 to the Java_org_haxe_lime_Lime_* JNI exports.
void (*Lime_onResize)(void *env, jobject thiz, jint width, jint height);
void (*Lime_onRender)(void *env, jobject thiz);
void (*Lime_onPoll)(void *env, jobject thiz);
jlong (*Lime_getNextWake)(void *env, jobject thiz);

// ApplicationMain is normally started by Android Java after Lime loads. The
// Vita loader owns that sequence, so it calls the exported Haxe entry point.
void (*HXCPP_main)(void *env, jobject thiz);

// Input entry points driven from our controls polling.
/* Soda Dungeon's Lime 2.x JNI signatures (confirmed from classes.dex):
 *   onTouch(int type, float x, float y, int id, float sizeX, float sizeY)
 *   onKeyChange(int keyCode, int charCode, boolean isDown)
 */
jint (*Lime_onTouch)(void *env, jobject thiz, jint type, jfloat x, jfloat y,
                     jint id, jfloat sizeX, jfloat sizeY);
jint (*Lime_onKeyChange)(void *env, jobject thiz, jint keyCode, jint charCode,
                         jboolean isDown);

// Touch tracking: lime expects a single down/move/up stream per pointer id,
// with Android-style ACTION_DOWN/MOVE/UP codes.
JavaDynArray *touch_event;

// Native NME/Lime EventType values used by this exact liblime-legacy build.
#define LIME_TOUCH_DOWN  15
#define LIME_TOUCH_MOVE  16
#define LIME_TOUCH_UP    17

// Forward declarations from init.c — loading of the second .so lives there so
// the loader bootstrap stays single-sourced.
void soloader_init_all(void);

void controls_handler_key(int32_t keycode, ControlsAction action)
{
    if (!Lime_onKeyChange)
        return;

    // The second argument is a Unicode character code, not an Android action.
    // Gamepad buttons do not have a printable character equivalent.
    jboolean is_down = (action != CONTROLS_ACTION_UP) ? JNI_TRUE : JNI_FALSE;
    Lime_onKeyChange(&jni, NULL, keycode, 0, is_down);
}

void controls_handler_touch(int32_t id, float x, float y, ControlsAction action)
{
    if (!Lime_onTouch)
        return;

    int type;
    switch (action)
    {
    case CONTROLS_ACTION_DOWN: type = LIME_TOUCH_DOWN; break;
    case CONTROLS_ACTION_UP:   type = LIME_TOUCH_UP;   break;
    default:                   type = LIME_TOUCH_MOVE; break;
    }

    Lime_onTouch(&jni, NULL, type, x, y, id, 1.0f, 1.0f);
}

void controls_handler_analog(ControlsStickId which, float x, float y, ControlsAction action)
{
    // lime-legacy supports an onTrackball callback; Soda Dungeon doesn't use
    // trackball input, so we leave analog sticks unhandled for now. The touch
    // screen fully drives the menu-driven UI.
    return;
}

int main()
{
    l_info("Soda Dungeon Vita loader starting.");

    soloader_init_all();
    controls_init();
    gl_init();

    touch_event = jda_alloc(1, FIELD_TYPE_INT);

    // --- Resolve the lime entry points we'll drive from our render loop. ---
    Lime_onResize    = (void *)so_symbol(&so_mod, "Java_org_haxe_lime_Lime_onResize");
    Lime_onRender    = (void *)so_symbol(&so_mod, "Java_org_haxe_lime_Lime_onRender");
    Lime_onPoll      = (void *)so_symbol(&so_mod, "Java_org_haxe_lime_Lime_onPoll");
    Lime_getNextWake = (void *)so_symbol(&so_mod, "Java_org_haxe_lime_Lime_getNextWake");
    Lime_onTouch     = (void *)so_symbol(&so_mod, "Java_org_haxe_lime_Lime_onTouch");
    Lime_onKeyChange = (void *)so_symbol(&so_mod, "Java_org_haxe_lime_Lime_onKeyChange");
    HXCPP_main       = (void *)so_symbol(&so_mod_game, "Java_org_haxe_HXCPP_main");

    if (!Lime_onRender) {
        l_fatal("Could not resolve Java_org_haxe_lime_Lime_onRender. Aborting.");
        fatal_error("Failed to locate the lime render entry point. The data "
                    "files may be missing or from an unsupported version.");
    }

    l_success("lime entry points resolved.");

    // --- Boot the lime runtime. ---
    // On Android, JNI_OnLoad is the first thing Java calls after System.load-
    // Library("lime-legacy"). Inside, lime sets up its renderer, dlopens the
    // game .so, and registers the Haxe entry point. We must have FalsoJNI and
    // the GL context ready before this point — both are, via init.c.
    int (*JNI_OnLoad)(void *vm) = (void *)so_symbol(&so_mod, "JNI_OnLoad");
    if (!JNI_OnLoad) {
        l_fatal("liblime-legacy.so has no JNI_OnLoad export.");
        fatal_error("The lime framework host could not be initialized.");
    }

    l_info("Calling lime JNI_OnLoad...");
    JNI_OnLoad(&jvm);
    l_success("lime JNI_OnLoad returned.");

    // Supply the same VM to the embedded OpenAL Android backend.  On Android
    // its own JNI_OnLoad performs this assignment before alcOpenDevice runs.
    *(JavaVM **)(LIME_LOAD_ADDRESS + OPENAL_ANDROID_JAVAVM_OFFSET) = &jvm;
    l_success("OpenAL Android JavaVM bridge initialized.");

    if (!HXCPP_main) {
        l_fatal("Could not resolve Java_org_haxe_HXCPP_main.");
        fatal_error("Failed to locate Soda Dungeon's Haxe entry point.");
    }

    l_info("Starting Soda Dungeon Haxe runtime...");
    HXCPP_main(&jni, NULL);
    l_success("Soda Dungeon Haxe runtime started.");
    cheats_runtime_init();

    // Notify lime of the viewport size once before the first frame. Some lime
    // builds queue the initial resize inside onRender; calling it explicitly
    // avoids a black first frame.
    if (Lime_onResize)
        Lime_onResize(&jni, NULL, SCREEN_WIDTH, SCREEN_HEIGHT);

    l_success("Entering render loop.");

    // --- Main loop. Mirrors org.haxe.lime.MainView$Renderer.onDrawFrame. ---
    unsigned int cheat_generation = 0;
    while (1)
    {
        controls_poll();
        cheats_poll();

        if (cheats_menu_is_open()) {
            unsigned int generation = cheats_menu_generation();
            if (generation != cheat_generation) {
                cheat_generation = generation;
                /* Draw and present one complete menu frame. Do not continue
                 * swapping the other stale back buffers while the game is
                 * paused: doing so makes selections and toggles flicker. */
                gl_draw_cheat_menu(cheats_menu_selection(),
                                   cheats_flag_cheap(), cheats_flag_crit(),
                                   cheats_flag_auto(), cheats_flag_patrons(),
                                   cheats_flag_registration());
                gl_swap();
            } else {
                sceKernelDelayThread(8000);
            }
            continue;
        }

        if (Lime_onPoll)
            Lime_onPoll(&jni, NULL);

        Lime_onRender(&jni, NULL);

        gl_draw_speed_indicator(time_scale_get());
        gl_swap();
    }

    return sceKernelExitDeleteThread(0);
}
