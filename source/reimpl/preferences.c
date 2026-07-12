/*
 * A small durable key/value store used by Lime's Android preference bridge.
 * OpenFL SharedObject serializes Soda Dungeon's saves into string values, so
 * preserving these calls is equivalent to preserving Android app data.
 */

#include "reimpl/preferences.h"

#include "utils/logger.h"
#include "utils/utils.h"

#include <psp2/io/fcntl.h>

#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define PREFS_PATH     DATA_PATH "preferences.bin"
#define PREFS_TMP_PATH DATA_PATH "preferences.tmp"
#define PREFS_BAK_PATH DATA_PATH "preferences.bak"
#define PREFS_MAGIC    "SDPREF01"
#define PREFS_MAX_FILE (32u * 1024u * 1024u)

typedef struct Preference {
    char *key;
    char *value;
    struct Preference *next;
} Preference;

static Preference *preferences;
static int initialized;
static int dirty;
static pthread_mutex_t preferences_mutex = PTHREAD_MUTEX_INITIALIZER;

static int suppress_gold_ad_hint(char *value)
{
    /* SaveFile field 25 is the integer-keyed Flag map. Flag 26 is
       KNOWS_ABOUT_GOLD_AD_BONUS in version 1.2.44. Match its neighbouring
       sequential keys as well, so an unrelated integer 26 is never changed.
       The save payload is URL encoded by SharedObject; replacement is one
       byte and therefore leaves every serialized length intact. */
    int changed = 0;
    static const char *encoded_patterns[] = {
        "%3A25f%3A26f%3A27", "%3A25t%3A26f%3A27"
    };
    for (size_t i = 0; i < sizeof(encoded_patterns) / sizeof(encoded_patterns[0]); i++) {
        const char *pattern = encoded_patterns[i];
        char *match = value;
        while ((match = strstr(match, pattern)) != NULL) {
            match[11] = 't';
            changed = 1;
            match += 12;
        }
    }
    static const char *plain_patterns[] = {
        ":25f:26f:27", ":25t:26f:27"
    };
    for (size_t i = 0; i < sizeof(plain_patterns) / sizeof(plain_patterns[0]); i++) {
        const char *pattern = plain_patterns[i];
        char *match = value;
        while ((match = strstr(match, pattern)) != NULL) {
            match[7] = 't';
            changed = 1;
            match += 8;
        }
    }
    return changed;
}

static uint32_t read_u32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void write_u32(uint8_t *p, uint32_t value)
{
    p[0] = value & 0xff;
    p[1] = (value >> 8) & 0xff;
    p[2] = (value >> 16) & 0xff;
    p[3] = (value >> 24) & 0xff;
}

static Preference *find_preference(const char *key)
{
    for (Preference *entry = preferences; entry; entry = entry->next) {
        if (!strcmp(entry->key, key))
            return entry;
    }
    return NULL;
}

static int load_preferences_file(const char *path)
{
    uint8_t *data = NULL;
    size_t size = 0;
    if (!file_exists(path) || !file_load(path, &data, &size))
        return 0;

    if (size < 12 || size > PREFS_MAX_FILE ||
        memcmp(data, PREFS_MAGIC, 8) != 0) {
        free(data);
        return 0;
    }

    uint32_t count = read_u32(data + 8);
    size_t offset = 12;
    Preference *loaded = NULL;

    for (uint32_t i = 0; i < count; i++) {
        if (offset + 8 > size)
            goto invalid;
        uint32_t key_len = read_u32(data + offset);
        uint32_t value_len = read_u32(data + offset + 4);
        offset += 8;
        if (!key_len || key_len > 4096 || value_len > PREFS_MAX_FILE ||
            offset + key_len + value_len > size)
            goto invalid;

        Preference *entry = calloc(1, sizeof(*entry));
        if (!entry)
            goto invalid;
        entry->key = malloc((size_t)key_len + 1);
        entry->value = malloc((size_t)value_len + 1);
        if (!entry->key || !entry->value) {
            free(entry->key);
            free(entry->value);
            free(entry);
            goto invalid;
        }
        memcpy(entry->key, data + offset, key_len);
        entry->key[key_len] = '\0';
        offset += key_len;
        memcpy(entry->value, data + offset, value_len);
        entry->value[value_len] = '\0';
        offset += value_len;
        entry->next = loaded;
        loaded = entry;
    }

    free(data);
    preferences = loaded;
    return 1;

invalid:
    free(data);
    while (loaded) {
        Preference *next = loaded->next;
        free(loaded->key);
        free(loaded->value);
        free(loaded);
        loaded = next;
    }
    return 0;
}

static void initialize_locked(void)
{
    if (initialized)
        return;
    initialized = 1;

    if (load_preferences_file(PREFS_PATH)) {
        l_info("Loaded persistent Soda Dungeon preferences.");
        return;
    }
    if (load_preferences_file(PREFS_BAK_PATH)) {
        l_warn("Recovered Soda Dungeon preferences from backup.");
        dirty = 1;
    }
}

static int save_locked(void)
{
    if (!dirty)
        return 1;

    uint32_t count = 0;
    size_t size = 12;
    for (Preference *entry = preferences; entry; entry = entry->next) {
        size_t key_len = strlen(entry->key);
        size_t value_len = strlen(entry->value);
        if (key_len > UINT32_MAX || value_len > UINT32_MAX ||
            size > PREFS_MAX_FILE - 8 - key_len - value_len)
            return 0;
        size += 8 + key_len + value_len;
        count++;
    }

    uint8_t *data = malloc(size);
    if (!data)
        return 0;
    memcpy(data, PREFS_MAGIC, 8);
    write_u32(data + 8, count);
    size_t offset = 12;
    for (Preference *entry = preferences; entry; entry = entry->next) {
        uint32_t key_len = (uint32_t)strlen(entry->key);
        uint32_t value_len = (uint32_t)strlen(entry->value);
        write_u32(data + offset, key_len);
        write_u32(data + offset + 4, value_len);
        offset += 8;
        memcpy(data + offset, entry->key, key_len);
        offset += key_len;
        memcpy(data + offset, entry->value, value_len);
        offset += value_len;
    }

    file_mkpath(PREFS_TMP_PATH, 0777);
    int ok = file_save(PREFS_TMP_PATH, data, size);
    free(data);
    if (!ok)
        return 0;

    if (file_exists(PREFS_PATH))
        file_copy(PREFS_PATH, PREFS_BAK_PATH);
    sceIoRemove(PREFS_PATH);
    if (sceIoRename(PREFS_TMP_PATH, PREFS_PATH) < 0) {
        /* Some filesystem layers cannot rename across their mounted view. */
        if (!file_copy(PREFS_TMP_PATH, PREFS_PATH))
            return 0;
        sceIoRemove(PREFS_TMP_PATH);
    }
    dirty = 0;
    return 1;
}

void preferences_init(void)
{
    pthread_mutex_lock(&preferences_mutex);
    initialize_locked();
    pthread_mutex_unlock(&preferences_mutex);
}

char *preferences_get(const char *key, const char *default_value)
{
    if (!key)
        key = "";
    if (!default_value)
        default_value = "";

    pthread_mutex_lock(&preferences_mutex);
    initialize_locked();
    Preference *entry = find_preference(key);
    char *result = strdup(entry ? entry->value : default_value);
    if (result)
        suppress_gold_ad_hint(result);
    pthread_mutex_unlock(&preferences_mutex);
    return result;
}

void preferences_set(const char *key, const char *value)
{
    if (!key || !*key)
        return;
    if (!value)
        value = "";

    pthread_mutex_lock(&preferences_mutex);
    initialize_locked();
    Preference *entry = find_preference(key);
    if (!entry) {
        entry = calloc(1, sizeof(*entry));
        if (entry) {
            entry->key = strdup(key);
            if (entry->key) {
                entry->next = preferences;
                preferences = entry;
            } else {
                free(entry);
                entry = NULL;
            }
        }
    }
    if (entry) {
        char *copy = strdup(value);
        if (copy) {
            suppress_gold_ad_hint(copy);
            free(entry->value);
            entry->value = copy;
            dirty = 1;
            if (!save_locked())
                l_error("Failed to persist preference '%s'.", key);
        }
    }
    pthread_mutex_unlock(&preferences_mutex);
}

void preferences_remove(const char *key)
{
    if (!key)
        return;
    pthread_mutex_lock(&preferences_mutex);
    initialize_locked();
    Preference **link = &preferences;
    while (*link) {
        Preference *entry = *link;
        if (!strcmp(entry->key, key)) {
            *link = entry->next;
            free(entry->key);
            free(entry->value);
            free(entry);
            dirty = 1;
            save_locked();
            break;
        }
        link = &entry->next;
    }
    pthread_mutex_unlock(&preferences_mutex);
}

void preferences_flush(void)
{
    pthread_mutex_lock(&preferences_mutex);
    initialize_locked();
    if (!save_locked())
        l_error("Failed to flush Soda Dungeon preferences.");
    pthread_mutex_unlock(&preferences_mutex);
}
