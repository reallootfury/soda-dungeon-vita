/*
 * Copyright (C) 2021      Andy Nguyen
 * Copyright (C) 2021      Rinnegatamante
 * Copyright (C) 2022-2023 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#include "utils/glutil.h"

#include "utils/utils.h"
#include "utils/dialog.h"
#include "utils/logger.h"
#include "cheats.h"

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/io/stat.h>

// Helpers for our handling of shaders
GLboolean skip_next_compile = GL_FALSE;
char next_shader_fname[256];
void load_shader(GLuint shader, const char * string, size_t length);

void gl_preload() {
    if (!file_exists("ur0:/data/libshacccg.suprx")
        && !file_exists("ur0:/data/external/libshacccg.suprx")) {
        fatal_error("Error: libshacccg.suprx is not installed. "
                    "Google \"ShaRKBR33D\" for quick installation.");
    }

#ifdef USE_GLSL_SHADERS
    vglSetSemanticBindingMode(VGL_MODE_POSTPONED);
#endif
}

void gl_init() {
    vglInitExtended(0, 960, 544, 6 * 1024 * 1024, SCE_GXM_MULTISAMPLE_4X);
}

void gl_swap() {
    vglSwapBuffers(GL_FALSE);
}

static void overlay_clear_rect(int x, int y, int width, int height,
                               float red, float green, float blue, float alpha)
{
    glScissor(x, y, width, height);
    glClearColor(red, green, blue, alpha);
    glClear(GL_COLOR_BUFFER_BIT);
}

#define GLYPH5(a,b,c,d,e) (((a) << 12) | ((b) << 9) | ((c) << 6) | ((d) << 3) | (e))

static unsigned int overlay_glyph(char c)
{
    static const unsigned short digit_glyphs[10] = {
        GLYPH5(7,5,5,5,7), GLYPH5(2,6,2,2,7),
        GLYPH5(7,1,7,4,7), GLYPH5(7,1,7,1,7),
        GLYPH5(5,5,7,1,1), GLYPH5(7,4,7,1,7),
        GLYPH5(7,4,7,5,7), GLYPH5(7,1,1,1,1),
        GLYPH5(7,5,7,5,7), GLYPH5(7,5,7,1,7),
    };
    if (c >= '0' && c <= '9')
        return digit_glyphs[c - '0'];
    switch (c) {
        case 'A': return GLYPH5(2,5,7,5,5);
        case 'B': return GLYPH5(6,5,6,5,6);
        case 'C': return GLYPH5(7,4,4,4,7);
        case 'D': return GLYPH5(6,5,5,5,6);
        case 'E': return GLYPH5(7,4,6,4,7);
        case 'F': return GLYPH5(7,4,6,4,4);
        case 'G': return GLYPH5(7,4,5,5,7);
        case 'H': return GLYPH5(5,5,7,5,5);
        case 'I': return GLYPH5(7,2,2,2,7);
        case 'K': return GLYPH5(5,5,6,5,5);
        case 'L': return GLYPH5(4,4,4,4,7);
        case 'M': return GLYPH5(5,7,7,5,5);
        case 'N': return GLYPH5(5,7,7,7,5);
        case 'O': return GLYPH5(7,5,5,5,7);
        case 'P': return GLYPH5(6,5,6,4,4);
        case 'R': return GLYPH5(6,5,6,5,5);
        case 'S': return GLYPH5(7,4,7,1,7);
        case 'T': return GLYPH5(7,2,2,2,2);
        case 'U': return GLYPH5(5,5,5,5,7);
        case 'W': return GLYPH5(5,5,7,7,5);
        case 'X': return GLYPH5(5,5,2,5,5);
        case 'Y': return GLYPH5(5,5,2,2,2);
        case '+': return GLYPH5(0,2,7,2,0);
        case '-': return GLYPH5(0,0,7,0,0);
        case ':': return GLYPH5(0,2,0,2,0);
        case '>': return GLYPH5(4,2,1,2,4);
        default: return 0;
    }
}

static void overlay_draw_text(int x, int y, int scale, const char *text,
                              float red, float green, float blue)
{
    for (int index = 0; text[index]; index++) {
        unsigned int glyph = overlay_glyph(text[index]);
        for (int row = 0; row < 5; row++) {
            unsigned int bits = (glyph >> ((4 - row) * 3)) & 7;
            for (int col = 0; col < 3;) {
                if (!(bits & (1u << (2 - col)))) {
                    col++;
                    continue;
                }
                int run_start = col;
                while (col < 3 && (bits & (1u << (2 - col))))
                    col++;
                overlay_clear_rect(x + index * scale * 4 + run_start * scale,
                    y + (4 - row) * scale, (col - run_start) * scale, scale,
                    red, green, blue, 1.0f);
            }
        }
    }
}

void gl_draw_cheat_menu(int selection, int cheap_mode,
                        int always_crit, int auto_arena,
                        int always_patrons, int show_registration)
{
    static const char *base_labels[CHEAT_MENU_COUNT] = {
        "+1M GOLD", "MAX GOLD", "+10K CAPS", "MAX CAPS", "+10K ESSENCE",
        "XMAS ITEMS", "ALL UPGRADES", "SUPER RELICS", "ACTIVATE PORTAL",
        "EASY BOSSES", "AUTO PARTY", "SKIP INTRO", NULL, NULL, NULL,
        NULL, NULL, "START LEVEL +1", "START LEVEL +10",
        "START LEVEL +100", "DIMENSION +1", "CLOSE"
    };
    GLboolean scissor_was_enabled = glIsEnabled(GL_SCISSOR_TEST);
    GLint old_scissor[4];
    GLfloat old_clear[4];
    GLboolean old_mask[4];
    glGetIntegerv(GL_SCISSOR_BOX, old_scissor);
    glGetFloatv(GL_COLOR_CLEAR_VALUE, old_clear);
    glGetBooleanv(GL_COLOR_WRITEMASK, old_mask);
    glEnable(GL_SCISSOR_TEST);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    overlay_clear_rect(180, 68, 600, 410, 0.03f, 0.03f, 0.04f, 1.0f);
    overlay_clear_rect(188, 76, 584, 394, 0.10f, 0.10f, 0.13f, 1.0f);
    overlay_draw_text(324, 438, 4, "CHEAT MENU", 1.0f, 0.8f, 0.15f);

    int first = selection - 4;
    if (first < 0)
        first = 0;
    if (first > CHEAT_MENU_COUNT - 10)
        first = CHEAT_MENU_COUNT - 10;
    for (int row = 0; row < 10; row++) {
        int i = first + row;
        char dynamic_label[32];
        const char *label = base_labels[i];
        if (i == 12) {
            snprintf(dynamic_label, sizeof(dynamic_label), "ALWAYS PATRONS: %s",
                     always_patrons ? "ON" : "OFF");
            label = dynamic_label;
        } else if (i == 13) {
            snprintf(dynamic_label, sizeof(dynamic_label), "SHOW REG POINTS: %s",
                     show_registration ? "ON" : "OFF");
            label = dynamic_label;
        } else if (i == 14) {
            snprintf(dynamic_label, sizeof(dynamic_label), "ALWAYS CRIT: %s",
                     always_crit ? "ON" : "OFF");
            label = dynamic_label;
        } else if (i == 15) {
            snprintf(dynamic_label, sizeof(dynamic_label), "CHEAP MODE: %s",
                     cheap_mode ? "ON" : "OFF");
            label = dynamic_label;
        } else if (i == 16) {
            snprintf(dynamic_label, sizeof(dynamic_label), "AUTO ARENA: %s",
                     auto_arena ? "ON" : "OFF");
            label = dynamic_label;
        }
        int y = 394 - row * 31;
        if (i == selection)
            overlay_clear_rect(222, y - 5, 516, 23,
                               0.22f, 0.22f, 0.28f, 1.0f);
        overlay_draw_text(238, y, 2, i == selection ? ">" : " ",
                          1.0f, 0.8f, 0.15f);
        overlay_draw_text(254, y, 2, label,
                          i == selection ? 1.0f : 0.9f,
                          i == selection ? 0.8f : 0.9f,
                          i == selection ? 0.15f : 0.9f);
    }
    overlay_draw_text(306, 86, 3, "UP DOWN SELECT   X USE",
                      0.65f, 0.65f, 0.7f);

    glClearColor(old_clear[0], old_clear[1], old_clear[2], old_clear[3]);
    glColorMask(old_mask[0], old_mask[1], old_mask[2], old_mask[3]);
    glScissor(old_scissor[0], old_scissor[1], old_scissor[2], old_scissor[3]);
    if (!scissor_was_enabled)
        glDisable(GL_SCISSOR_TEST);
}

void gl_draw_speed_indicator(unsigned int speed)
{
    if (speed <= 1)
        return;

    /* Tiny 3x5 bitmap glyphs drawn with scissored clears. This is independent
       of the game's shader and texture state, making the overlay reliable on
       the legacy OpenFL renderer. Rows are stored top-to-bottom. */
    static const unsigned char digits[10][5] = {
        { 7, 5, 5, 5, 7 }, /* 0 */
        { 2, 6, 2, 2, 7 }, /* 1 */
        { 7, 1, 7, 4, 7 }, /* 2 */
        { 7, 1, 7, 1, 7 }, /* 3 */
        { 5, 5, 7, 1, 1 }, /* 4 */
        { 7, 4, 7, 1, 7 }, /* 5 */
        { 7, 4, 7, 5, 7 }, /* 6 */
        { 7, 1, 1, 1, 1 }, /* 7 */
        { 7, 5, 7, 5, 7 }, /* 8 */
        { 7, 5, 7, 1, 7 }, /* 9 */
    };
    static const unsigned char glyph_x[5] = { 5, 5, 2, 5, 5 };

    char label[5];
    snprintf(label, sizeof(label), "%uX", speed);
    int glyph_count = (int)strlen(label);
    GLboolean scissor_was_enabled = glIsEnabled(GL_SCISSOR_TEST);
    GLint old_scissor[4];
    GLfloat old_clear[4];
    GLboolean old_mask[4];
    glGetIntegerv(GL_SCISSOR_BOX, old_scissor);
    glGetFloatv(GL_COLOR_CLEAR_VALUE, old_clear);
    glGetBooleanv(GL_COLOR_WRITEMASK, old_mask);

    glEnable(GL_SCISSOR_TEST);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    const int panel_width = 8 + glyph_count * 16;
    const int panel_x = 952 - panel_width;
    const int panel_y = 510;
    const int scale = 4;
    overlay_clear_rect(panel_x, panel_y, panel_width, 28,
                       0.05f, 0.05f, 0.05f, 1.0f);

    for (int glyph = 0; glyph < glyph_count; glyph++) {
        for (int row = 0; row < 5; row++) {
            unsigned char bits = label[glyph] == 'X' ? glyph_x[row] :
                                 digits[label[glyph] - '0'][row];
            for (int col = 0; col < 3; col++) {
                if (!(bits & (1u << (2 - col))))
                    continue;
                int x = panel_x + 4 + glyph * 16 + col * scale;
                int y = panel_y + 4 + (4 - row) * scale;
                overlay_clear_rect(x, y, scale, scale,
                                   1.0f, 1.0f, 1.0f, 1.0f);
            }
        }
    }

    glClearColor(old_clear[0], old_clear[1], old_clear[2], old_clear[3]);
    glColorMask(old_mask[0], old_mask[1], old_mask[2], old_mask[3]);
    glScissor(old_scissor[0], old_scissor[1], old_scissor[2], old_scissor[3]);
    if (!scissor_was_enabled)
        glDisable(GL_SCISSOR_TEST);
}

void glShaderSource_soloader(GLuint shader, GLsizei count,
                             const GLchar **string, const GLint *_length) {
#ifdef DEBUG_OPENGL
    sceClibPrintf("[gl_dbg] glShaderSource<%p>(shader: %i, count: %i, string: %p, length: %p)\n", __builtin_return_address(0), shader, count, string, _length);
#endif
    if (!string) {
        l_error("<%p> Shader source string is NULL, count: %i",
                   __builtin_return_address(0), count);
        skip_next_compile = GL_TRUE;
        return;
    } else if (!*string) {
        l_error("<%p> Shader source *string is NULL, count: %i",
                   __builtin_return_address(0), count);
        skip_next_compile = GL_TRUE;
        return;
    }

    size_t total_length = 0;

    for (int i = 0; i < count; ++i) {
        if (!_length) {
            total_length += strlen(string[i]);
        } else {
            total_length += _length[i];
        }
    }

    char * str = malloc(total_length+1);
    size_t l = 0;

    for (int i = 0; i < count; ++i) {
        if (!_length) {
            memcpy(str + l, string[i], strlen(string[i]));
            l += strlen(string[i]);
        } else {
            memcpy(str + l, string[i], _length[i]);
            l += _length[i];
        }
    }
    str[total_length] = '\0';

    l_debug("shader is %s", str);

    load_shader(shader, str, total_length);

    free(str);
}

void glCompileShader_soloader(GLuint shader) {
#ifdef DEBUG_OPENGL
    sceClibPrintf("[gl_dbg] glCompileShader<%p>(shader: %i)\n", __builtin_return_address(0), shader);
#endif

#ifndef USE_GXP_SHADERS
    if (!skip_next_compile) {
        glCompileShader(shader);
#ifdef DUMP_COMPILED_SHADERS
        void *bin = vglMalloc(32 * 1024);
        GLsizei len;
        vglGetShaderBinary(shader, 32 * 1024, &len, bin);
        file_save(next_shader_fname, bin, len);
        vglFree(bin);
#endif
    }
    skip_next_compile = GL_FALSE;
#endif
}

#if defined(USE_GLSL_SHADERS) && defined(DUMP_COMPILED_SHADERS)
void load_shader(GLuint shader, const char * string, size_t length) {
    char* sha_name = str_sha1sum(string, length);

    char gxp_path[256];
    snprintf(gxp_path, sizeof(gxp_path), DATA_PATH"gxp/%s.gxp", sha_name);

    if (file_exists(gxp_path)) {
        uint8_t *buffer;
        size_t size;

        file_load(gxp_path, &buffer, &size);

        glShaderBinary(1, &shader, 0, buffer, (int32_t) size);

        free(buffer);
        skip_next_compile = GL_TRUE;
    } else {
        glShaderSource(shader, 1, &string, &length);
        strcpy(next_shader_fname, gxp_path);
    }

    free(sha_name);
}
#elif defined(USE_GLSL_SHADERS)
void load_shader(GLuint shader, const char * string, size_t length) {
    glShaderSource(shader, 1, &string, &length);
}
#elif defined(USE_CG_SHADERS) && defined(DUMP_COMPILED_SHADERS)
void load_shader(GLuint shader, const char * string, size_t length) {
    char* sha_name = str_sha1sum(string, length);

    char gxp_path[256];
    char cg_path[256];
    snprintf(gxp_path, sizeof(gxp_path), DATA_PATH"gxp/%s.gxp", sha_name);
    snprintf(cg_path, sizeof(cg_path), DATA_PATH"cg/%s.cg", sha_name);

    if (file_exists(gxp_path)) {
        uint8_t *buffer;
        size_t size;

        file_load(gxp_path, &buffer, &size);

        glShaderBinary(1, &shader, 0, buffer, (int32_t) size);

        free(buffer);
        skip_next_compile = GL_TRUE;
    } else if (file_exists(cg_path)) {
        char *buffer;
        size_t size;

        file_load(cg_path, (uint8_t **) &buffer, &size);

        glShaderSource(shader, 1, &string, &size);
        strcpy(next_shader_fname, gxp_path);

        free(buffer);
        skip_next_compile = GL_FALSE;
    } else {
        l_warn("Encountered an untranslated shader %s, saving GLSL "
               "and using a dummy shader.", sha_name);

        char glsl_path[256];
        snprintf(glsl_path, sizeof(glsl_path), DATA_PATH"glsl/%s.glsl", sha_name);
        file_mkpath(glsl_path, 0777);
        file_save(glsl_path, (const uint8_t *) string, length);

        if (strstr(string, "gl_FragColor")) {
            const char *dummy_shader = "float4 main() { return float4(1.0,1.0,1.0,1.0); }";
            int32_t dummy_shader_len = (int32_t) strlen(dummy_shader);
            glShaderSource(shader, 1, &dummy_shader, &dummy_shader_len);
        } else {
            const char *dummy_shader = "void main(float4 out gl_Position : POSITION ) { gl_Position = float4(1.0,1.0,1.0,1.0); }";
            int32_t dummy_shader_len = (int32_t) strlen(dummy_shader);
            glShaderSource(shader, 1, &dummy_shader, &dummy_shader_len);
        }

        skip_next_compile = GL_FALSE;
    }

    free(sha_name);
}
#elif defined(USE_CG_SHADERS) || defined(USE_GXP_SHADERS)
void load_shader(GLuint shader, const char * string, size_t length) {
    char* sha_name = str_sha1sum(string, length);

    char path[256];
#ifdef USE_CG_SHADERS
    snprintf(path, sizeof(path), DATA_PATH"cg/%s.cg", sha_name);
#else
    snprintf(path, sizeof(path), DATA_PATH"gxp/%s.gxp", sha_name);
#endif

    if (file_exists(path)) {
#ifdef USE_CG_SHADERS
        char *buffer;
        size_t size;

        file_load(path, (uint8_t **) &buffer, &size);

        glShaderSource(shader, 1, &string, &size);

        free(buffer);
#else
        uint8_t *buffer;
        size_t size;

        file_load(path, &buffer, &size);

        glShaderBinary(1, &shader, 0, buffer, (int32_t) size);

        free(buffer);
#endif
    } else {
        l_warn("Encountered an untranslated shader %s, saving GLSL "
               "and using a dummy shader.", sha_name);

        char glsl_path[256];
        snprintf(glsl_path, sizeof(glsl_path), DATA_PATH"glsl/%s.glsl", sha_name);
        file_mkpath(glsl_path, 0777);
        file_save(glsl_path, (const uint8_t *) string, length);

        if (strstr(string, "gl_FragColor")) {
            const char *dummy_shader = "float4 main() { return float4(1.0,1.0,1.0,1.0); }";
            int32_t dummy_shader_len = (int32_t) strlen(dummy_shader);
            glShaderSource(shader, 1, &dummy_shader, &dummy_shader_len);
        } else {
            const char *dummy_shader = "void main(float4 out gl_Position : POSITION ) { gl_Position = float4(1.0,1.0,1.0,1.0); }";
            int32_t dummy_shader_len = (int32_t) strlen(dummy_shader);
            glShaderSource(shader, 1, &dummy_shader, &dummy_shader_len);
        }
    }

    free(sha_name);
}
#else
#error "Define one of (USE_GLSL_SHADERS, USE_CG_SHADERS, USE_GXP_SHADERS)"
#endif
