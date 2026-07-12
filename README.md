# Soda Dungeon · PS Vita loader

*just to note, i had AI go through any copyright issues and get rid of them, the bubble will not have any artwork containing images done by Armor Games

An unofficial PS Vita loader for the Android release of **Soda Dungeon**. It
runs the original ARMv7 libraries inside a small Android compatibility layer.
The repository and VPK contain no Soda Dungeon executable code, game assets,
APK, or official promotional artwork.

The current target is Soda Dungeon **1.2.44**
(`com.armorgames.sodadungeon`). Other versions are not expected to work.

> [!IMPORTANT]
> This fan project is not affiliated with or endorsed by AN Productions,
> Poxpower, Armor Games Studios, or Sony Interactive Entertainment. Soda
> Dungeon and its original artwork, code, and other game content belong to
> their respective owners. The names are used only to identify compatibility.

## Install and data setup

You must provide the required files from a copy of version 1.2.44 that you are
legally entitled to use. Do not distribute an APK or extracted game files with
this loader.

1. Install [kubridge](https://github.com/TheOfficialFloW/kubridge/releases) and
 RePatch under `*KERNEL` in
   `ur0:tai/config.txt`.
2. Copy `libshacccg.suprx` to `ur0:data/` using a lawful dump from your own
   device.
3. From your version 1.2.44 APK, copy the following files to
   `ux0:data/soda/`:

   ```text
   assets/                         (the APK assets directory)
   liblime-legacy.so               (from lib/armeabi/)
   libApplicationMain.so           (from lib/armeabi/)
   ```

4. Install the locally built `soda.vpk`.

The resulting data directory is:

```text
ux0:data/soda/
├── assets/
├── libApplicationMain.so
└── liblime-legacy.so
```

## Build from source

Use a soft-float VitaSDK and install/build its soft-float variants of vitaGL,
vitashark, math-neon, SceShaccCgExt, pthread, and kubridge stubs. The helper
below builds the two ABI-sensitive archives (OpenSLES and zlib) from pinned
upstream source. The project intentionally does not bundle precompiled
third-party libraries.

Clone the repository normally; FalsoJNI is vendored because this port carries
a small game-specific bridge change:

```bash
export VITASDK=/path/to/vitasdk-softfp
export PATH="$VITASDK/bin:$PATH"
./extras/scripts/build-softfp-deps.sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j2
```

The generated VPK is `build/soda.vpk`. The `build/`, `.deps-src/`, and
`.deps-softfp/` directories and all VPK, APK, shared-library, and crash-dump
files are ignored by Git and must not be committed.

For private artwork that you have the right to use, put correctly sized
`icon0.png`, `pic0.png`, `startup.png`, and `bg0.png` files in the ignored
`.local-livearea/` directory and configure with
`-DLIVEAREA_DIR="$PWD/.local-livearea"`. Never commit that directory unless
you have redistribution permission for every image.

OpenSLES provides the game's simple-buffer-queue PCM output. SoLoud is not
used by this game and is intentionally not linked.

## Status

Version 1.2.44 boots to the main menu with rendering, touchscreen input,
controller input, and soft-float OpenSLES audio output working. Compatibility
is specific to that APK version.

Save data is stored in `ux0:data/soda/preferences.bin`, with the preceding
version retained as `preferences.bak`. Ads and in-app purchases are hidden
because their Android services are unavailable on Vita; this prevents the
original "Loading ad" screen from waiting indefinitely. Touches on the
lower-left rewarded-ad gold icon are intentionally ignored, and the associated
one-time gold-ad tutorial is treated as already acknowledged.

The Vita-native Cheat Menu provides gold, caps, and essence modifiers; the
game's built-in Xmas Items, All Upgrades, Super Relics, Activate Portal, Easy
Bosses, Auto Party, Skip Intro, dungeon-start, and dimension actions; plus
toggles for Always Patrons, registration markers, Always Crit, Cheap Mode, and
Auto Arena. Save-changing actions operate on the active slot, so keep a backup
before using them. A loader-side speed cheat is also available: hold **L + R**
and press **Up** to cycle 1x, 2x, 4x, 8x, and 16x.
The active multiplier is displayed in the upper-right corner.
Hold **L + R** and press **Down** to open the Vita-native Cheat Menu. Use the
D-pad and Cross to navigate and activate an entry. The longer list scrolls as
the selection moves. Circle closes the panel.

## Repository content and licenses

- Loader code and the project-created geometric LiveArea artwork are available
  under the [MIT License](LICENSE). The artwork uses plain text only; it does
  not contain the game's logo, screenshots, or extracted assets.
- Vendored components retain their own copyright and license notices; see
  [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
- No license is granted for Soda Dungeon, PlayStation, PS Vita, or other
  third-party names and marks.

Publishing a repository cannot guarantee that nobody will raise a legal
claim. The separation of the loader from user-supplied game data, original
unbranded artwork, preserved notices, and non-affiliation statement are
intended to keep the repository limited to redistributable port code.
