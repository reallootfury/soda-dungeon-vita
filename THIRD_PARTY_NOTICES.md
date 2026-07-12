# Third-party notices

This project is derived from and includes portions of several open-source
projects. Copyright notices in individual source files remain authoritative.

## Included source

- **soloader-boilerplate / Android SO Loader components** — Copyright Andy
  Nguyen, Rinnegatamante, Volodymyr Atamanenko, and other named contributors;
  MIT License. Upstream:
  <https://github.com/v-atamanenko/soloader-boilerplate>
- **FalsoJNI** — Copyright Volodymyr Atamanenko and named contributors; MIT
  License. Its license is preserved at `lib/falso_jni/LICENSE`. Some JNI
  compatibility definitions retain their Android Open Source Project notices.
  Upstream: <https://github.com/v-atamanenko/FalsoJNI>
- **Android/Bionic compatibility definitions** — Copyright The Android Open
  Source Project; the applicable Apache-2.0 or BSD-style notice is preserved in
  each source file.
- **Time::y2038 time64 implementation** — Copyright Michael G Schwern; MIT
  License, reproduced in `source/reimpl/time64.c` and
  `source/reimpl/time64.h`.
- **SHA-1 implementation** — Brad Conte; released as public-domain source by
  its upstream project. Upstream:
  <https://github.com/B-Con/crypto-algorithms>
- **NetBSD ctype table** — J.T. Conklin; public domain, as recorded in
  `source/reimpl/bits/_ctype.c`.

## Build-time dependencies not included

The repository does not distribute precompiled copies of VitaSDK, vitaGL,
vitashark, OpenSLES, zlib, math-neon, SceShaccCgExt, pthread, kubridge, or Sony
runtime libraries. Builders obtain these separately and must comply with each
project's license and distribution terms. The dependency helper also fetches
the libsndfile public header at build time; URI decoding is stubbed and the
libsndfile binary is not linked or distributed.

## Components linked into locally built VPKs

- **OpenSLES Vita port** — includes Android Open Source Project code under the
  Apache License 2.0 and Khronos OpenSL ES interface material under its
  permissive notice. Source: <https://github.com/frangarcj/opensles>. A copy of
  Apache-2.0 is included at `LICENSES/Apache-2.0.txt`.
- **zlib** — Copyright Jean-loup Gailly and Mark Adler; zlib License. Source:
  <https://github.com/madler/zlib>. The license is included at
  `LICENSES/Zlib.txt`.

The Khronos OpenSL ES interface notice permits use, copying, modification,
merging, publication, distribution, sublicensing, and sale provided its
copyright and permission notice are included. The material is provided “as
is,” without warranty. Copyright (c) 2007-2009 The Khronos Group Inc.

## Excluded proprietary content

No Soda Dungeon APK, Android game library, extracted game asset, official
logo, promotional image, music, or other game content is included. Users must
supply compatible files from a copy they are legally entitled to use.
