/* Persistent replacement for the Android SharedPreferences API. */
#ifndef SODA_PREFERENCES_H
#define SODA_PREFERENCES_H

#ifdef __cplusplus
extern "C" {
#endif

void preferences_init(void);
char *preferences_get(const char *key, const char *default_value);
void preferences_set(const char *key, const char *value);
void preferences_remove(const char *key);
void preferences_flush(void);

#ifdef __cplusplus
}
#endif

#endif
