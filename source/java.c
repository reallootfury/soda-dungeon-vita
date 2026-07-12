/*
 * Soda Dungeon · PSVita Port — FalsoJNI bridge for the lime framework.
 *
 * This file registers the Java methods and fields that liblime-legacy.so looks
 * up via JNIEnv->GetMethodID / GetStaticMethodID / GetFieldID at runtime. When
 * lime calls back into "Java", FalsoJNI dispatches it to the matching handler.
 *
 * The strategy is deliberately permissive: we implement what we know lime and
 * Soda Dungeon need, and return harmless defaults for everything else.
 *
 * Copyright (C) 2021      Andy Nguyen
 * Copyright (C) 2021      Rinnegatamante
 * Copyright (C) 2022-2024 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#include <falso_jni/FalsoJNI.h>
#include <falso_jni/FalsoJNI_Impl.h>
#include <falso_jni/FalsoJNI_Logger.h>

#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "utils/utils.h"
#include "reimpl/preferences.h"
#include "reimpl/sys.h"

/*
 * =========================================================================
 *  Directory / path accessors
 * =========================================================================
 */

jstring getFilesDir(jmethodID id, va_list args) {
    (void)id; (void)args;
    return jni->NewStringUTF(&jni, DATA_PATH "files");
}

jstring getCacheDir(jmethodID id, va_list args) {
    (void)id; (void)args;
    return jni->NewStringUTF(&jni, DATA_PATH "cache");
}

jstring getCodeCacheDir(jmethodID id, va_list args) {
    (void)id; (void)args;
    return jni->NewStringUTF(&jni, DATA_PATH "code_cache");
}

jstring getPackageName(jmethodID id, va_list args) {
    (void)id; (void)args;
    return jni->NewStringUTF(&jni, "com.armorgames.sodadungeon");
}

jstring getDataDirectory(jmethodID id, va_list args) {
    (void)id; (void)args;
    return jni->NewStringUTF(&jni, DATA_PATH);
}

/*
 * =========================================================================
 *  Locale / device info
 * =========================================================================
 */

jstring getDeviceLanguage(jmethodID id, va_list args) {
    (void)id; (void)args;
    return jni->NewStringUTF(&jni, "en");
}

jstring getAndroidVersion(jmethodID id, va_list args) {
    (void)id; (void)args;
    return jni->NewStringUTF(&jni, "4.4.2");
}

jint getSdkInt(jmethodID id, va_list args) {
    (void)id; (void)args;
    return 19;
}

/*
 * =========================================================================
 *  lime framework callbacks
 * =========================================================================
 */

void onCallbackVoid(jmethodID id, va_list args) {
    (void)args;
    fjni_logv_info("[soda] onCallback<%i>", (int)id);
}

jlong onCallbackLong(jmethodID id, va_list args) {
    (void)id; (void)args;
    return 0;
}

void releaseReference(jmethodID id, va_list args) {
    (void)args;
    fjni_logv_dbg("[soda] releaseReference<%i>", (int)id);
}

jlong getNextWake(jmethodID id, va_list args) {
    (void)id; (void)args;
    return 0;
}

void postUICallback(jmethodID id, va_list args) {
    (void)id;
    (void)va_arg(args, jlong);
}

jobject createHaxeObject(jmethodID id, va_list args) {
    (void)id;
    (void)va_arg(args, jlong);
    return (jobject)0x42424242;
}

jobject createAudioTrack(jmethodID id, va_list args) {
    (void)id;
    (void)args;
    // Audio is produced by the native Vita backend; OpenAL only needs a
    // stable non-null Java peer while it runs its Android-side bookkeeping.
    return (jobject)0x41554449; // "AUDI"
}

jobject createOptionalService(jmethodID id, va_list args) {
    (void)id;
    (void)args;
    // Ads and billing are disabled, but their Haxe wrappers require a
    // non-null result from Java initialize().
    return (jobject)0x53455256; // "SERV"
}

jfloat optionalServiceFloat(jmethodID id, va_list args) {
    (void)id;
    (void)args;
    return 0.0f;
}

jboolean optionalServiceFalse(jmethodID id, va_list args) {
    (void)id;
    (void)args;
    return JNI_FALSE;
}

jobject optionalServiceEmptyString(jmethodID id, va_list args) {
    (void)id;
    (void)args;
    return jni->NewStringUTF(&jni, "");
}

jboolean boxedIsArray(jmethodID id, va_list args) {
    (void)id; (void)args;
    return JNI_FALSE;
}

jboolean boxedBooleanValue(jmethodID id, va_list args) {
    (void)id; (void)args;
    return JNI_FALSE;
}

jdouble boxedDoubleValue(jmethodID id, va_list args) {
    (void)id; (void)args;
    return 0.0;
}

jchar boxedCharValue(jmethodID id, va_list args) {
    (void)id; (void)args;
    return 0;
}

jint sampleMethod(jmethodID id, va_list args) {
    (void)id;
    return va_arg(args, jint);
}

/*
 * =========================================================================
 *  AudioTrack stubs
 * =========================================================================
 */

void audioNoop(jmethodID id, va_list args) { (void)id; (void)args; }

jint audioWrite(jmethodID id, va_list args) {
    (void)id;
    (void)va_arg(args, jbyteArray);
    (void)va_arg(args, jint); // offset
    jint len = va_arg(args, jint);
    return len;
}

jint audioGetMinBufferSize(jmethodID id, va_list args) {
    (void)id;
    (void)va_arg(args, jint);
    (void)va_arg(args, jint);
    (void)va_arg(args, jint);
    return 4096 * 2;
}

jint audioGetNativeOutputSampleRate(jmethodID id, va_list args) {
    (void)id; (void)args;
    return 44100;
}

void audioSetVolume(jmethodID id, va_list args) {
    (void)id;
    (void)va_arg(args, jdouble);
    (void)va_arg(args, jdouble);
}

void audioSetPlaybackRate(jmethodID id, va_list args) {
    (void)id;
    (void)va_arg(args, jint);
}

void audioSetPlaybackHeadPosition(jmethodID id, va_list args) {
    (void)id;
    (void)va_arg(args, jint);
}

/*
 * =========================================================================
 *  Shared-preferences style accessors
 * =========================================================================
 */

jstring prefsGetString(jmethodID id, va_list args) {
    jstring key = va_arg(args, jstring);
    jstring def = NULL;
    /* SharedPreferences.getString has a default; Lime's convenience methods
       use one argument and default to an empty string. */
    if ((uintptr_t)id == 140)
        def = va_arg(args, jstring);
    char *keyText = key ? (char *)jni->GetStringUTFChars(&jni, key, NULL) : NULL;
    char *defText = def ? (char *)jni->GetStringUTFChars(&jni, def, NULL) : NULL;
    char *value = preferences_get(keyText, defText);
    jstring result = jni->NewStringUTF(&jni, value ? value : "");
    free(value);
    if (defText)
        jni->ReleaseStringUTFChars(&jni, def, defText);
    if (keyText)
        jni->ReleaseStringUTFChars(&jni, key, keyText);
    return result;
}

jint prefsGetInt(jmethodID id, va_list args) {
    jstring key = va_arg(args, jstring);
    jint def = va_arg(args, jint);
    char defaultText[32];
    snprintf(defaultText, sizeof(defaultText), "%d", (int)def);
    char *keyText = key ? (char *)jni->GetStringUTFChars(&jni, key, NULL) : NULL;
    char *value = preferences_get(keyText, defaultText);
    jint result = value ? (jint)strtol(value, NULL, 10) : def;
    free(value);
    if (keyText)
        jni->ReleaseStringUTFChars(&jni, key, keyText);
    return result;
}

jboolean prefsGetBool(jmethodID id, va_list args) {
    (void)id;
    jstring key = va_arg(args, jstring);
    jboolean def = va_arg(args, jint);
    char *keyText = key ? (char *)jni->GetStringUTFChars(&jni, key, NULL) : NULL;
    char *value = preferences_get(keyText, def ? "1" : "0");
    jboolean result = value && (!strcmp(value, "1") || !strcasecmp(value, "true"));
    free(value);
    if (keyText)
        jni->ReleaseStringUTFChars(&jni, key, keyText);
    return result;
}

void prefsSetString(jmethodID id, va_list args) {
    (void)id;
    jstring key = va_arg(args, jstring);
    jstring value = va_arg(args, jstring);
    char *keyText = key ? (char *)jni->GetStringUTFChars(&jni, key, NULL) : NULL;
    char *valueText = value ? (char *)jni->GetStringUTFChars(&jni, value, NULL) : NULL;
    preferences_set(keyText, valueText);
    if (valueText)
        jni->ReleaseStringUTFChars(&jni, value, valueText);
    if (keyText)
        jni->ReleaseStringUTFChars(&jni, key, keyText);
}

void prefsSetInt(jmethodID id, va_list args) {
    (void)id;
    jstring key = va_arg(args, jstring);
    jint value = va_arg(args, jint);
    char text[32];
    snprintf(text, sizeof(text), "%d", (int)value);
    char *keyText = key ? (char *)jni->GetStringUTFChars(&jni, key, NULL) : NULL;
    preferences_set(keyText, text);
    if (keyText)
        jni->ReleaseStringUTFChars(&jni, key, keyText);
}

void prefsSetBool(jmethodID id, va_list args) {
    (void)id;
    jstring key = va_arg(args, jstring);
    jboolean value = va_arg(args, jint);
    char *keyText = key ? (char *)jni->GetStringUTFChars(&jni, key, NULL) : NULL;
    preferences_set(keyText, value ? "1" : "0");
    if (keyText)
        jni->ReleaseStringUTFChars(&jni, key, keyText);
}

void prefsClear(jmethodID id, va_list args) {
    (void)id;
    jstring key = va_arg(args, jstring);
    char *keyText = key ? (char *)jni->GetStringUTFChars(&jni, key, NULL) : NULL;
    preferences_remove(keyText);
    if (keyText)
        jni->ReleaseStringUTFChars(&jni, key, keyText);
}

void prefsFlush(jmethodID id, va_list args) {
    (void)id; (void)args;
    preferences_flush();
}

/*
 * =========================================================================
 *  System methods
 * =========================================================================
 */

jlong systemCurrentTimeMillis(jmethodID id, va_list args) {
    (void)id; (void)args;
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (jlong)ts.tv_sec * 1000LL + (jlong)(ts.tv_nsec / 1000000);
}

jlong systemNanoTime(jmethodID id, va_list args) {
    (void)id; (void)args;
    return (jlong)monotonic_time_ns_soloader();
}

void systemArraycopy(jmethodID id, va_list args) {
    (void)id;
    jobject src    = va_arg(args, jobject);
    jint    srcPos = va_arg(args, jint);
    jobject dst    = va_arg(args, jobject);
    jint    dstPos = va_arg(args, jint);
    jint    length = va_arg(args, jint);
    JavaDynArray *srcArr = (JavaDynArray *)src;
    JavaDynArray *dstArr = (JavaDynArray *)dst;
    if (srcArr && dstArr && srcArr->array && dstArr->array) {
        jsize elemSize = getFieldTypeSize(srcArr->type);
        memmove((char*)dstArr->array + dstPos * elemSize,
                (char*)srcArr->array + srcPos * elemSize,
                length * elemSize);
    }
}

jstring systemGetProperty(jmethodID id, va_list args) {
    (void)id;
    (void)va_arg(args, jstring);
    return jni->NewStringUTF(&jni, "");
}

void systemLoadLibrary(jmethodID id, va_list args) {
    (void)id;
    (void)va_arg(args, jstring);
}

jstring systemGetEnv(jmethodID id, va_list args) {
    (void)id; (void)args;
    return jni->NewStringUTF(&jni, "");
}

jint systemIdentityHashCode(jmethodID id, va_list args) {
    (void)id;
    jobject obj = va_arg(args, jobject);
    return (jint)(unsigned long)obj;
}

/*
 * =========================================================================
 *  String constructors and methods
 * =========================================================================
 */

jobject stringInitDefault(jmethodID id, va_list args) {
    (void)id; (void)args;
    return jni->NewStringUTF(&jni, "");
}

jobject stringInitBytes(jmethodID id, va_list args) {
    (void)id;
    jbyteArray bytes = va_arg(args, jbyteArray);
    JavaDynArray *arr = (JavaDynArray *)bytes;
    if (arr && arr->array && arr->len > 0)
        return jni->NewStringUTF(&jni, (const char *)arr->array);
    return jni->NewStringUTF(&jni, "");
}

jobject stringInitBytesCharset(jmethodID id, va_list args) {
    (void)id;
    jbyteArray bytes = va_arg(args, jbyteArray);
    (void)va_arg(args, jstring);
    JavaDynArray *arr = (JavaDynArray *)bytes;
    if (arr && arr->array && arr->len > 0)
        return jni->NewStringUTF(&jni, (const char *)arr->array);
    return jni->NewStringUTF(&jni, "");
}

jobject stringGetBytes(jmethodID id, va_list args) {
    (void)id;
    jstring str = va_arg(args, jstring);
    const char *utf = jni->GetStringUTFChars(&jni, str, NULL);
    jsize len = strlen(utf);
    jbyteArray arr = jni->NewByteArray(&jni, len);
    jni->SetByteArrayRegion(&jni, arr, 0, len, (const jbyte *)utf);
    jni->ReleaseStringUTFChars(&jni, str, (char *)utf);
    return arr;
}

jint stringLength(jmethodID id, va_list args) {
    (void)id;
    jstring str = va_arg(args, jstring);
    return jni->GetStringLength(&jni, str);
}

jboolean stringIsEmpty(jmethodID id, va_list args) {
    (void)id;
    jstring str = va_arg(args, jstring);
    return jni->GetStringLength(&jni, str) == 0 ? JNI_TRUE : JNI_FALSE;
}

jstring stringTrim(jmethodID id, va_list args) {
    (void)id;
    return va_arg(args, jstring);
}

jstring stringSubstring(jmethodID id, va_list args) {
    (void)id;
    jstring str  = va_arg(args, jstring);
    jint    start = va_arg(args, jint);
    jint    end   = va_arg(args, jint);
    jsize   len   = jni->GetStringLength(&jni, str);
    if (start < 0) start = 0;
    if (end > len) end = len;
    if (start >= end) return jni->NewStringUTF(&jni, "");
    const char *utf = jni->GetStringUTFChars(&jni, str, NULL);
    jsize utfLen = strlen(utf);
    if (start > utfLen) start = utfLen;
    if (end > utfLen) end = utfLen;
    if (start > end) start = end;
    char *buf = (char *)malloc(end - start + 1);
    memcpy(buf, utf + start, end - start);
    buf[end - start] = '\0';
    jstring result = jni->NewStringUTF(&jni, buf);
    free(buf);
    jni->ReleaseStringUTFChars(&jni, str, (char *)utf);
    return result;
}

jstring stringReplace(jmethodID id, va_list args) {
    (void)id;
    jstring str = va_arg(args, jstring);
    (void)va_arg(args, jstring);
    (void)va_arg(args, jstring);
    return str;
}

jstring stringToLowerCase(jmethodID id, va_list args) {
    (void)id;
    jstring str = va_arg(args, jstring);
    const char *utf = jni->GetStringUTFChars(&jni, str, NULL);
    jsize len = strlen(utf);
    char *buf = (char *)malloc(len + 1);
    for (jsize i = 0; i < len; i++)
        buf[i] = (utf[i] >= 'A' && utf[i] <= 'Z') ? (utf[i] + 32) : utf[i];
    buf[len] = '\0';
    jstring result = jni->NewStringUTF(&jni, buf);
    free(buf);
    jni->ReleaseStringUTFChars(&jni, str, (char *)utf);
    return result;
}

jstring stringValueOf(jmethodID id, va_list args) {
    (void)id; (void)va_arg(args, jobject);
    return jni->NewStringUTF(&jni, "null");
}

jint stringIndexOfChar(jmethodID id, va_list args) {
    (void)id;
    jstring str = va_arg(args, jstring);
    jint ch = va_arg(args, jint);
    const char *utf = jni->GetStringUTFChars(&jni, str, NULL);
    jsize len = strlen(utf);
    for (jsize i = 0; i < len; i++) {
        if ((unsigned char)utf[i] == (unsigned char)ch) {
            jni->ReleaseStringUTFChars(&jni, str, (char *)utf);
            return i;
        }
    }
    jni->ReleaseStringUTFChars(&jni, str, (char *)utf);
    return -1;
}

jint stringCompareTo(jmethodID id, va_list args) {
    (void)id;
    jstring a = va_arg(args, jstring);
    jstring b = va_arg(args, jstring);
    const char *utfA = jni->GetStringUTFChars(&jni, a, NULL);
    const char *utfB = jni->GetStringUTFChars(&jni, b, NULL);
    int r = strcmp(utfA, utfB);
    jni->ReleaseStringUTFChars(&jni, a, (char *)utfA);
    jni->ReleaseStringUTFChars(&jni, b, (char *)utfB);
    return r;
}

jboolean stringEquals(jmethodID id, va_list args) {
    (void)id;
    jobject a = va_arg(args, jobject);
    jobject b = va_arg(args, jobject);
    return (a == b) ? JNI_TRUE : JNI_FALSE;
}

jint stringHashCode(jmethodID id, va_list args) {
    (void)id;
    jstring str = va_arg(args, jstring);
    const char *utf = jni->GetStringUTFChars(&jni, str, NULL);
    unsigned int h = 0;
    for (const char *p = utf; *p; p++) h = 31 * h + (unsigned char)*p;
    jni->ReleaseStringUTFChars(&jni, str, (char *)utf);
    return (jint)h;
}

jstring stringConcat(jmethodID id, va_list args) {
    (void)id;
    jstring a = va_arg(args, jstring);
    jstring b = va_arg(args, jstring);
    const char *uA = jni->GetStringUTFChars(&jni, a, NULL);
    const char *uB = jni->GetStringUTFChars(&jni, b, NULL);
    jsize lA = strlen(uA), lB = strlen(uB);
    char *buf = (char *)malloc(lA + lB + 1);
    memcpy(buf, uA, lA); memcpy(buf + lA, uB, lB);
    buf[lA + lB] = '\0';
    jstring r = jni->NewStringUTF(&jni, buf);
    free(buf);
    jni->ReleaseStringUTFChars(&jni, a, (char *)uA);
    jni->ReleaseStringUTFChars(&jni, b, (char *)uB);
    return r;
}

jstring stringIntern(jmethodID id, va_list args) {
    (void)id;
    return va_arg(args, jstring);
}

/*
 * =========================================================================
 *  AssetManager / InputStream methods
 * =========================================================================
 */

jobject assetManagerOpenFd(jmethodID id, va_list args) {
    (void)id; (void)args;
    return (jobject)0x42424242;
}

void closeInputStream(jmethodID id, va_list args) { (void)id; (void)args; }

jint readInputStream(jmethodID id, va_list args) {
    (void)id; (void)args;
    return -1;
}

jint availableInputStream(jmethodID id, va_list args) {
    (void)id; (void)args;
    return 0;
}

jlong skipInputStream(jmethodID id, va_list args) {
    (void)id;
    return va_arg(args, jlong);
}

void markInputStream(jmethodID id, va_list args) {
    (void)id; (void)va_arg(args, jint);
}

void resetInputStream(jmethodID id, va_list args) { (void)id; (void)args; }

jboolean markSupportedInputStream(jmethodID id, va_list args) {
    (void)id; (void)args;
    return JNI_FALSE;
}

/*
 * =========================================================================
 *  ByteBuffer methods
 * =========================================================================
 */

jobject byteBufferAllocate(jmethodID id, va_list args) {
    (void)id;
    jint cap = va_arg(args, jint);
    return jni->NewByteArray(&jni, cap);
}

jobject byteBufferAllocateDirect(jmethodID id, va_list args) {
    (void)id;
    return byteBufferAllocate(id, args);
}

jobject byteBufferWrap(jmethodID id, va_list args) {
    (void)id;
    return va_arg(args, jbyteArray);
}

jint byteBufferCapacity(jmethodID id, va_list args) {
    (void)id;
    jobject buf = va_arg(args, jobject);
    return buf ? jni->GetArrayLength(&jni, buf) : 0;
}

jobject byteBufferOrder(jmethodID id, va_list args) {
    (void)id; (void)args;
    return (jobject)0x42424242;
}

jobject byteBufferPut(jmethodID id, va_list args) {
    (void)id;
    return va_arg(args, jobject);
}

jobject byteBufferPutBuffer(jmethodID id, va_list args) {
    (void)id;
    jobject self = va_arg(args, jobject);
    (void)va_arg(args, jobject);
    return self;
}

jobject byteBufferGet(jmethodID id, va_list args) {
    (void)id; (void)args;
    return (jobject)0;
}

void byteBufferGetBuffer(jmethodID id, va_list args) { (void)id; (void)args; }

jobject byteBufferFlip(jmethodID id, va_list args) {
    (void)id;
    return va_arg(args, jobject);
}

jobject byteBufferRewind(jmethodID id, va_list args) {
    (void)id;
    return va_arg(args, jobject);
}

jobject byteBufferClear(jmethodID id, va_list args) {
    (void)id;
    return va_arg(args, jobject);
}

jobject byteBufferPosition(jmethodID id, va_list args) {
    (void)id; (void)args;
    return (jobject)0x42424242;
}

jobject byteBufferLimit(jmethodID id, va_list args) {
    (void)id; (void)args;
    return (jobject)0x42424242;
}

jobject byteBufferCompact(jmethodID id, va_list args) {
    (void)id;
    return va_arg(args, jobject);
}

jint byteBufferPositionInt(jmethodID id, va_list args) {
    (void)id; (void)args;
    return 0;
}

jint byteBufferLimitInt(jmethodID id, va_list args) {
    (void)id; (void)args;
    return 0;
}

jobject byteBufferAsReadOnlyBuffer(jmethodID id, va_list args) {
    (void)id;
    return va_arg(args, jobject);
}

jobject byteBufferSlice(jmethodID id, va_list args) {
    (void)id;
    return va_arg(args, jobject);
}

jobject byteBufferDuplicate(jmethodID id, va_list args) {
    (void)id;
    return va_arg(args, jobject);
}

/*
 * =========================================================================
 *  Integer / Float / Double / Boolean boxed type methods
 * =========================================================================
 */

jint integerParseInt(jmethodID id, va_list args) {
    (void)id; (void)va_arg(args, jstring);
    return 0;
}

jint integerIntValue(jmethodID id, va_list args) {
    (void)id;
    return (jint)(unsigned long)va_arg(args, jobject);
}

jobject integerValueOf(jmethodID id, va_list args) {
    (void)id;
    jint val = va_arg(args, jint);
    return (jobject)(unsigned long)val;
}

jfloat floatFloatValue(jmethodID id, va_list args) {
    (void)id; (void)va_arg(args, jobject);
    return 0.0f;
}

jdouble doubleDoubleValue(jmethodID id, va_list args) {
    (void)id; (void)va_arg(args, jobject);
    return 0.0;
}

jboolean booleanBooleanValue(jmethodID id, va_list args) {
    (void)id; (void)va_arg(args, jobject);
    return JNI_FALSE;
}

jboolean booleanParseBoolean(jmethodID id, va_list args) {
    (void)id; (void)va_arg(args, jstring);
    return JNI_FALSE;
}

/*
 * =========================================================================
 *  Class / Object methods
 * =========================================================================
 */

jstring classNameGetName(jmethodID id, va_list args) {
    (void)id; (void)va_arg(args, jobject);
    return jni->NewStringUTF(&jni, "java.lang.Object");
}

jobject classGetMethod(jmethodID id, va_list args) {
    (void)id; (void)args;
    return (jobject)0x42424242;
}

jobject classGetDeclaredMethod(jmethodID id, va_list args) {
    (void)id; (void)args;
    return (jobject)0x42424242;
}

jobject classGetField(jmethodID id, va_list args) {
    (void)id; (void)args;
    return (jobject)0x42424242;
}

jobject classGetDeclaredField(jmethodID id, va_list args) {
    (void)id; (void)args;
    return (jobject)0x42424242;
}

jobject classGetConstructor(jmethodID id, va_list args) {
    (void)id; (void)args;
    return (jobject)0x42424242;
}

jobject classNewInstance(jmethodID id, va_list args) {
    (void)id; (void)args;
    return (jobject)0x42424242;
}

jboolean classIsArray(jmethodID id, va_list args) {
    (void)id; (void)args;
    return JNI_FALSE;
}

jboolean classIsPrimitive(jmethodID id, va_list args) {
    (void)id; (void)args;
    return JNI_FALSE;
}

jint classGetModifiers(jmethodID id, va_list args) {
    (void)id; (void)args;
    return 1;
}

jobject classGetComponentType(jmethodID id, va_list args) {
    (void)id; (void)args;
    return NULL;
}

jobject classGetSuperclass(jmethodID id, va_list args) {
    (void)id; (void)args;
    return (jobject)0x42424242;
}

jobject objectGetClass(jmethodID id, va_list args) {
    (void)id; (void)args;
    return (jobject)0x42424242;
}

jobject objectNotifyAll(jmethodID id, va_list args) {
    (void)id; (void)args;
    return (jobject)0x42424242;
}

jobject objectWait(jmethodID id, va_list args) {
    (void)id; (void)args;
    return (jobject)0x42424242;
}

jobject classForName(jmethodID id, va_list args) {
    (void)id; (void)va_arg(args, jstring);
    return (jobject)0x42424242;
}

/*
 * =========================================================================
 *  Float / Math helpers
 * =========================================================================
 */

jfloat floatParseFloat(jmethodID id, va_list args) {
    (void)id;
    jstring s = va_arg(args, jstring);
    const char *utf = jni->GetStringUTFChars(&jni, s, NULL);
    jfloat r = (jfloat)atof(utf);
    jni->ReleaseStringUTFChars(&jni, s, (char *)utf);
    return r;
}

jdouble doubleParseDouble(jmethodID id, va_list args) {
    (void)id;
    jstring s = va_arg(args, jstring);
    const char *utf = jni->GetStringUTFChars(&jni, s, NULL);
    jdouble r = atof(utf);
    jni->ReleaseStringUTFChars(&jni, s, (char *)utf);
    return r;
}

jlong doubleDoubleToLongBits(jmethodID id, va_list args) {
    (void)id;
    jdouble d = va_arg(args, jdouble);
    jlong r;
    memcpy(&r, &d, sizeof(r));
    return r;
}

jfloat floatIntBitsToFloat(jmethodID id, va_list args) {
    (void)id;
    jint bits = va_arg(args, jint);
    jfloat r;
    memcpy(&r, &bits, sizeof(r));
    return r;
}

jint floatFloatToIntBits(jmethodID id, va_list args) {
    (void)id;
    jfloat f = (jfloat)va_arg(args, jdouble);
    jint r;
    memcpy(&r, &f, sizeof(r));
    return r;
}

jfloat floatMax(jmethodID id, va_list args) {
    (void)id;
    jfloat a = (jfloat)va_arg(args, jdouble);
    jfloat b = (jfloat)va_arg(args, jdouble);
    return a > b ? a : b;
}

jfloat floatMin(jmethodID id, va_list args) {
    (void)id;
    jfloat a = (jfloat)va_arg(args, jdouble);
    jfloat b = (jfloat)va_arg(args, jdouble);
    return a < b ? a : b;
}

/*
 * =========================================================================
 *  Display / Window metrics
 * =========================================================================
 */

jobject getWindowManager(jmethodID id, va_list args) {
    (void)id; (void)args;
    return (jobject)0x42424242;
}

jobject getDefaultDisplay(jmethodID id, va_list args) {
    (void)id; (void)args;
    return (jobject)0x42424242;
}

jint getDisplayWidth(jmethodID id, va_list args) {
    (void)id; (void)args;
    return 960;
}

jint getDisplayHeight(jmethodID id, va_list args) {
    (void)id; (void)args;
    return 544;
}

jobject getDisplayMetrics(jmethodID id, va_list args) {
    (void)id; (void)args;
    return (jobject)0x42424242;
}

jobject getResources(jmethodID id, va_list args) {
    (void)id; (void)args;
    return (jobject)0x42424242;
}

jobject getConfiguration(jmethodID id, va_list args) {
    (void)id; (void)args;
    return (jobject)0x42424242;
}

jint getIdentifier(jmethodID id, va_list args) {
    (void)id;
    (void)va_arg(args, jstring);
    (void)va_arg(args, jstring);
    (void)va_arg(args, jstring);
    return 0;
}

jobject getAssets(jmethodID id, va_list args) {
    (void)id; (void)args;
    return (jobject)0x42424242;
}

jobject getApplicationContext(jmethodID id, va_list args) {
    (void)id; (void)args;
    return (jobject)0x42424242;
}

jobject getPackageManager(jmethodID id, va_list args) {
    (void)id; (void)args;
    return (jobject)0x42424242;
}

jobject getAppPackageInfo(jmethodID id, va_list args) {
    (void)id; (void)args;
    return (jobject)0x42424242;
}

jstring getPackageVersionName(jmethodID id, va_list args) {
    (void)id; (void)args;
    return jni->NewStringUTF(&jni, "1.2.44");
}

jint getPackageVersionCode(jmethodID id, va_list args) {
    (void)id; (void)args;
    return 1244;
}

/*
 * =========================================================================
 *  Runnable / Handler / UI thread helpers
 * =========================================================================
 */

void runnableRun(jmethodID id, va_list args) { (void)id; (void)args; }

jobject handlerPost(jmethodID id, va_list args) {
    (void)id; (void)args;
    return (jobject)0x42424242;
}

/*
 * =========================================================================
 *  Mirror / hxcpp reflection stubs
 * =========================================================================
 */

jobject methodInvoke(jmethodID id, va_list args) {
    (void)id; (void)args;
    return (jobject)0x42424242;
}

jstring methodGetName(jmethodID id, va_list args) {
    (void)id; (void)args;
    return jni->NewStringUTF(&jni, "unknown");
}

jobject methodGetParameterTypes(jmethodID id, va_list args) {
    (void)id; (void)args;
    return (jobject)0x42424242;
}

jobject methodGetReturnType(jmethodID id, va_list args) {
    (void)id; (void)args;
    return (jobject)0x42424242;
}

jint methodGetModifiers(jmethodID id, va_list args) {
    (void)id; (void)args;
    return 1;
}

/*
 * =========================================================================
 *  Method registry
 * =========================================================================
 */

NameToMethodID nameToMethodId[] = {
    { 100, "getFilesDir",                    METHOD_TYPE_OBJECT },
    { 101, "getCacheDir",                    METHOD_TYPE_OBJECT },
    { 102, "getCodeCacheDir",                METHOD_TYPE_OBJECT },
    { 103, "getPackageName",                 METHOD_TYPE_OBJECT },
    { 104, "getDataDirectory",               METHOD_TYPE_OBJECT },
    { 105, "getApplicationInfo",             METHOD_TYPE_OBJECT },
    { 106, "getContext",                     METHOD_TYPE_OBJECT },
    { 110, "getDeviceLanguage",              METHOD_TYPE_OBJECT },
    { 111, "getAndroidVersion",              METHOD_TYPE_OBJECT },
    { 112, "getSdkInt",                      METHOD_TYPE_INT },
    { 113, "CapabilitiesGetLanguage",         METHOD_TYPE_OBJECT },
    { 120, "onCallback",                     METHOD_TYPE_VOID },
    { 121, "releaseReference",               METHOD_TYPE_VOID },
    { 122, "getNextWake",                    METHOD_TYPE_LONG },
    { 123, "postUICallback",                  METHOD_TYPE_VOID },
    { 124, "create",                          METHOD_TYPE_OBJECT },
    { 125, "charValue",                       METHOD_TYPE_CHAR },
    { 126, "getDouble",                       METHOD_TYPE_DOUBLE },
    { 127, "sampleMethod",                    METHOD_TYPE_INT },
    { 128, "android/media/AudioTrack/<init>", METHOD_TYPE_OBJECT },
    { 130, "play",                           METHOD_TYPE_VOID },
    { 131, "pause",                          METHOD_TYPE_VOID },
    { 132, "stop",                           METHOD_TYPE_VOID },
    { 133, "release",                        METHOD_TYPE_VOID },
    { 134, "flush",                          METHOD_TYPE_VOID },
    { 135, "write",                          METHOD_TYPE_INT },
    { 136, "setVolume",                      METHOD_TYPE_VOID },
    { 137, "getMinBufferSize",               METHOD_TYPE_INT },
    { 138, "getNativeOutputSampleRate",      METHOD_TYPE_INT },
    { 139, "setPlaybackRate",                METHOD_TYPE_VOID },
    { 140, "getString",                      METHOD_TYPE_OBJECT },
    { 141, "getInt",                         METHOD_TYPE_INT },
    { 142, "getBoolean",                     METHOD_TYPE_BOOLEAN },
    { 143, "putString",                      METHOD_TYPE_VOID },
    { 144, "putInt",                         METHOD_TYPE_VOID },
    { 145, "putBoolean",                     METHOD_TYPE_VOID },
    { 146, "getStringForKey",                METHOD_TYPE_OBJECT },
    { 147, "getIntForKey",                   METHOD_TYPE_INT },
    { 148, "setStringForKey",                METHOD_TYPE_VOID },
    { 149, "setIntForKey",                   METHOD_TYPE_VOID },
    { 150, "flush",                          METHOD_TYPE_VOID },
    { 151, "getUserPreference",              METHOD_TYPE_OBJECT },
    { 152, "setUserPreference",              METHOD_TYPE_VOID },
    { 153, "clearUserPreference",            METHOD_TYPE_VOID },
    { 160, "toString",                       METHOD_TYPE_OBJECT },
    { 161, "getBytes",                       METHOD_TYPE_OBJECT },
    { 162, "length",                         METHOD_TYPE_INT },
    { 163, "hashCode",                       METHOD_TYPE_INT },
    { 164, "isEmpty",                        METHOD_TYPE_BOOLEAN },
    { 165, "trim",                           METHOD_TYPE_OBJECT },
    { 166, "substring",                      METHOD_TYPE_OBJECT },
    { 167, "valueOf",                        METHOD_TYPE_OBJECT },
    { 168, "replace",                        METHOD_TYPE_OBJECT },
    { 169, "toLowerCase",                    METHOD_TYPE_OBJECT },
    { 170, "indexOf",                        METHOD_TYPE_INT },
    { 171, "compareTo",                      METHOD_TYPE_INT },
    { 172, "equals",                         METHOD_TYPE_BOOLEAN },
    { 173, "concat",                         METHOD_TYPE_OBJECT },
    { 174, "intern",                         METHOD_TYPE_OBJECT },
    { 175, "getBytescs",                     METHOD_TYPE_OBJECT },
    { 200, "currentTimeMillis",              METHOD_TYPE_LONG },
    { 201, "nanoTime",                       METHOD_TYPE_LONG },
    { 202, "arraycopy",                      METHOD_TYPE_VOID },
    { 203, "getProperty",                    METHOD_TYPE_OBJECT },
    { 204, "loadLibrary",                    METHOD_TYPE_VOID },
    { 205, "getenv",                         METHOD_TYPE_OBJECT },
    { 206, "identityHashCode",               METHOD_TYPE_INT },
    { 300, "java.lang.String/<init>",        METHOD_TYPE_OBJECT },
    { 301, "java.lang.String/<init>bytes",   METHOD_TYPE_OBJECT },
    { 302, "java.lang.String/<init>bytescs", METHOD_TYPE_OBJECT },
    { 400, "close",                          METHOD_TYPE_VOID },
    { 401, "read",                           METHOD_TYPE_INT },
    { 402, "available",                      METHOD_TYPE_INT },
    { 403, "skip",                           METHOD_TYPE_LONG },
    { 404, "mark",                           METHOD_TYPE_VOID },
    { 405, "reset",                          METHOD_TYPE_VOID },
    { 406, "markSupported",                  METHOD_TYPE_BOOLEAN },
    { 410, "openFd",                         METHOD_TYPE_OBJECT },
    { 411, "open",                           METHOD_TYPE_OBJECT },
    { 412, "list",                           METHOD_TYPE_OBJECT },
    { 500, "allocate",                       METHOD_TYPE_OBJECT },
    { 501, "allocateDirect",                 METHOD_TYPE_OBJECT },
    { 502, "wrap",                           METHOD_TYPE_OBJECT },
    { 503, "capacity",                       METHOD_TYPE_INT },
    { 504, "order",                          METHOD_TYPE_OBJECT },
    { 505, "put",                            METHOD_TYPE_OBJECT },
    { 506, "get",                            METHOD_TYPE_OBJECT },
    { 507, "flip",                           METHOD_TYPE_OBJECT },
    { 508, "rewind",                         METHOD_TYPE_OBJECT },
    { 509, "clear",                          METHOD_TYPE_OBJECT },
    { 510, "position",                       METHOD_TYPE_OBJECT },
    { 511, "limit",                          METHOD_TYPE_OBJECT },
    { 512, "compact",                        METHOD_TYPE_OBJECT },
    { 513, "asReadOnlyBuffer",               METHOD_TYPE_OBJECT },
    { 514, "slice",                          METHOD_TYPE_OBJECT },
    { 515, "duplicate",                      METHOD_TYPE_OBJECT },
    { 516, "putBuffer",                      METHOD_TYPE_OBJECT },
    { 517, "getBuffer",                      METHOD_TYPE_VOID },
    { 518, "positionInt",                    METHOD_TYPE_INT },
    { 519, "limitInt",                       METHOD_TYPE_INT },
    { 520, "remaining",                      METHOD_TYPE_INT },
    { 521, "hasRemaining",                   METHOD_TYPE_BOOLEAN },
    { 600, "parseInt",                       METHOD_TYPE_INT },
    { 601, "intValue",                       METHOD_TYPE_INT },
    { 602, "floatValue",                     METHOD_TYPE_FLOAT },
    { 603, "doubleValue",                    METHOD_TYPE_DOUBLE },
    { 604, "booleanValue",                   METHOD_TYPE_BOOLEAN },
    { 605, "parseFloat",                     METHOD_TYPE_FLOAT },
    { 606, "parseDouble",                    METHOD_TYPE_DOUBLE },
    { 607, "doubleToLongBits",               METHOD_TYPE_LONG },
    { 608, "floatToIntBits",                 METHOD_TYPE_INT },
    { 609, "intBitsToFloat",                 METHOD_TYPE_FLOAT },
    { 610, "max",                            METHOD_TYPE_FLOAT },
    { 611, "min",                            METHOD_TYPE_FLOAT },
    { 612, "parseBoolean",                   METHOD_TYPE_BOOLEAN },
    { 613, "valueOfInt",                     METHOD_TYPE_INT },
    { 614, "valueOf",                        METHOD_TYPE_OBJECT },
    { 615, "toString",                       METHOD_TYPE_OBJECT },
    { 616, "equals",                         METHOD_TYPE_BOOLEAN },
    { 617, "hashCode",                       METHOD_TYPE_INT },
    { 618, "longValue",                      METHOD_TYPE_LONG },
    { 619, "floatToRawIntBits",              METHOD_TYPE_INT },
    { 700, "getName",                        METHOD_TYPE_OBJECT },
    { 701, "getMethod",                      METHOD_TYPE_OBJECT },
    { 702, "getDeclaredMethod",              METHOD_TYPE_OBJECT },
    { 703, "getField",                       METHOD_TYPE_OBJECT },
    { 704, "getDeclaredField",               METHOD_TYPE_OBJECT },
    { 705, "getConstructor",                 METHOD_TYPE_OBJECT },
    { 706, "newInstance",                    METHOD_TYPE_OBJECT },
    { 707, "isArray",                        METHOD_TYPE_BOOLEAN },
    { 708, "isPrimitive",                    METHOD_TYPE_BOOLEAN },
    { 709, "getModifiers",                   METHOD_TYPE_INT },
    { 710, "getComponentType",               METHOD_TYPE_OBJECT },
    { 711, "getSuperclass",                  METHOD_TYPE_OBJECT },
    { 712, "forName",                        METHOD_TYPE_OBJECT },
    { 720, "getClass",                       METHOD_TYPE_OBJECT },
    { 721, "notifyAll",                      METHOD_TYPE_OBJECT },
    { 722, "wait",                           METHOD_TYPE_OBJECT },
    { 723, "notify",                         METHOD_TYPE_VOID },
    { 800, "getWindowManager",               METHOD_TYPE_OBJECT },
    { 801, "getDefaultDisplay",              METHOD_TYPE_OBJECT },
    { 802, "getWidth",                       METHOD_TYPE_INT },
    { 803, "getHeight",                      METHOD_TYPE_INT },
    { 804, "getDisplayMetrics",              METHOD_TYPE_OBJECT },
    { 805, "getResources",                   METHOD_TYPE_OBJECT },
    { 806, "getConfiguration",               METHOD_TYPE_OBJECT },
    { 807, "getIdentifier",                  METHOD_TYPE_INT },
    { 808, "getAssets",                      METHOD_TYPE_OBJECT },
    { 809, "getApplicationContext",          METHOD_TYPE_OBJECT },
    { 810, "getPackageManager",              METHOD_TYPE_OBJECT },
    { 811, "getPackageInfo",                 METHOD_TYPE_OBJECT },
    { 812, "versionName",                    METHOD_TYPE_OBJECT },
    { 813, "versionCode",                    METHOD_TYPE_INT },
    { 900, "run",                            METHOD_TYPE_VOID },
    { 901, "post",                           METHOD_TYPE_OBJECT },
    { 1000,"invoke",                         METHOD_TYPE_OBJECT },
    { 1001,"getParameterTypes",              METHOD_TYPE_OBJECT },
    { 1002,"getReturnType",                  METHOD_TYPE_OBJECT },
    { 1003,"getModifiersMethod",             METHOD_TYPE_INT },
    { 1300,"setPlaybackHeadPosition",        METHOD_TYPE_VOID },
    { 1301,"getSampleRate",                  METHOD_TYPE_INT },
    { 1302,"getPlaybackHeadPosition",        METHOD_TYPE_INT },
    { 1303,"getState",                       METHOD_TYPE_INT },
    { 1304,"setNotificationMarkerPosition",  METHOD_TYPE_VOID },
    { 1305,"setPositionNotificationPeriod",  METHOD_TYPE_VOID },
    { 1600,"displayNotification",             METHOD_TYPE_VOID },
    { 1601,"cancelExistingNotifications",     METHOD_TYPE_VOID },
    { 1602,"initUnityAds",                    METHOD_TYPE_VOID },
    { 1603,"showUnityAd",                     METHOD_TYPE_VOID },
    { 1604,"androidToast",                    METHOD_TYPE_VOID },
    { 1605,"androidLog",                      METHOD_TYPE_VOID },
    { 1606,"getSoftKeyboardHeight",           METHOD_TYPE_FLOAT },
    { 1610,"initialize",                      METHOD_TYPE_OBJECT },
    { 1611,"loadRewarded",                    METHOD_TYPE_VOID },
    { 1612,"showRewarded",                    METHOD_TYPE_VOID },
    { 1613,"loadInterstitial",                METHOD_TYPE_VOID },
    { 1614,"showInterstitial",                METHOD_TYPE_VOID },
    { 1615,"loadBanner",                      METHOD_TYPE_VOID },
    { 1616,"showBanner",                      METHOD_TYPE_VOID },
    { 1617,"hideBanner",                      METHOD_TYPE_VOID },
    { 1618,"renderNow",                       METHOD_TYPE_VOID },
    { 1620,"init",                            METHOD_TYPE_VOID },
    { 1621,"login",                           METHOD_TYPE_VOID },
    { 1622,"displaySavedGames",               METHOD_TYPE_VOID },
    { 1623,"discardAndCloseGame",             METHOD_TYPE_BOOLEAN },
    { 1624,"commitAndCloseGame",              METHOD_TYPE_BOOLEAN },
    { 1625,"loadSavedGame",                   METHOD_TYPE_VOID },
    { 1626,"displayScoreboard",               METHOD_TYPE_BOOLEAN },
    { 1627,"displayAllScoreboards",           METHOD_TYPE_BOOLEAN },
    { 1628,"setScore",                        METHOD_TYPE_BOOLEAN },
    { 1629,"displayAchievements",             METHOD_TYPE_BOOLEAN },
    { 1630,"unlock",                          METHOD_TYPE_BOOLEAN },
    { 1631,"increment",                       METHOD_TYPE_BOOLEAN },
    { 1632,"reveal",                          METHOD_TYPE_BOOLEAN },
    { 1633,"setSteps",                        METHOD_TYPE_BOOLEAN },
    { 1634,"getPlayerScore",                  METHOD_TYPE_BOOLEAN },
    { 1635,"getAchievementStatus",            METHOD_TYPE_BOOLEAN },
    { 1636,"getCurrentAchievementSteps",      METHOD_TYPE_BOOLEAN },
    { 1637,"getPlayerId",                     METHOD_TYPE_OBJECT },
    { 1638,"getPlayerDisplayName",            METHOD_TYPE_OBJECT },
    { 1639,"loadInvitablePlayers",            METHOD_TYPE_BOOLEAN },
    { 1640,"loadConnectedPlayers",            METHOD_TYPE_BOOLEAN },
};

MethodsBoolean methodsBoolean[] = {
    { 142, prefsGetBool }, { 164, stringIsEmpty }, { 172, stringEquals },
    { 406, markSupportedInputStream }, { 521, stringIsEmpty },
    { 604, booleanBooleanValue }, { 612, booleanParseBoolean },
    { 616, stringEquals }, { 707, classIsArray }, { 708, classIsPrimitive },
    { 1623, optionalServiceFalse }, { 1624, optionalServiceFalse },
    { 1626, optionalServiceFalse }, { 1627, optionalServiceFalse },
    { 1628, optionalServiceFalse }, { 1629, optionalServiceFalse },
    { 1630, optionalServiceFalse }, { 1631, optionalServiceFalse },
    { 1632, optionalServiceFalse }, { 1633, optionalServiceFalse },
    { 1634, optionalServiceFalse }, { 1635, optionalServiceFalse },
    { 1636, optionalServiceFalse }, { 1639, optionalServiceFalse },
    { 1640, optionalServiceFalse },
};
MethodsByte methodsByte[] = {};
MethodsChar methodsChar[] = {
    { 125, boxedCharValue },
};
MethodsDouble methodsDouble[] = {
    { 126, boxedDoubleValue },
    { 603, doubleDoubleValue }, { 606, doubleParseDouble },
};
MethodsFloat methodsFloat[] = {
    { 602, floatFloatValue }, { 605, floatParseFloat },
    { 609, floatIntBitsToFloat }, { 610, floatMax }, { 611, floatMin },
    { 1606, optionalServiceFloat },
};
MethodsInt methodsInt[] = {
    { 112, getSdkInt }, { 127, sampleMethod }, { 135, audioWrite },
    { 141, prefsGetInt }, { 147, prefsGetInt },
    { 162, stringLength }, { 163, stringHashCode },
    { 170, stringIndexOfChar }, { 171, stringCompareTo },
    { 206, systemIdentityHashCode }, { 401, readInputStream },
    { 402, availableInputStream }, { 503, byteBufferCapacity },
    { 518, byteBufferPositionInt }, { 519, byteBufferLimitInt },
    { 520, byteBufferPositionInt },
    { 600, integerParseInt }, { 601, integerIntValue },
    { 608, floatFloatToIntBits }, { 613, integerIntValue },
    { 617, stringHashCode }, { 619, floatFloatToIntBits },
    { 709, classGetModifiers }, { 802, getDisplayWidth },
    { 803, getDisplayHeight }, { 807, getIdentifier },
    { 813, getPackageVersionCode }, { 1003, classGetModifiers },
    { 1300, getSdkInt }, { 1301, audioGetNativeOutputSampleRate },
    { 1302, getSdkInt }, { 1303, getSdkInt },
    { 137, audioGetMinBufferSize }, { 138, audioGetNativeOutputSampleRate },
};
MethodsLong methodsLong[] = {
    { 122, getNextWake }, { 200, systemCurrentTimeMillis },
    { 201, systemNanoTime }, { 403, skipInputStream },
    { 607, doubleDoubleToLongBits }, { 618, onCallbackLong },
};
MethodsObject methodsObject[] = {
    { 100, getFilesDir }, { 101, getCacheDir }, { 102, getCodeCacheDir },
    { 103, getPackageName }, { 104, getDataDirectory },
    { 105, getPackageName }, { 106, getPackageName },
    { 110, getDeviceLanguage }, { 111, getAndroidVersion },
    { 113, getDeviceLanguage }, { 124, createHaxeObject },
    { 128, createAudioTrack },
    { 1610, createOptionalService },
    { 1637, optionalServiceEmptyString },
    { 1638, optionalServiceEmptyString },
    { 140, prefsGetString }, { 146, prefsGetString },
    { 151, prefsGetString },
    { 160, getPackageName }, { 161, stringGetBytes },
    { 165, stringTrim }, { 166, stringSubstring },
    { 167, stringValueOf }, { 168, stringReplace },
    { 169, stringToLowerCase }, { 173, stringConcat },
    { 174, stringIntern }, { 175, stringGetBytes },
    { 203, systemGetProperty }, { 205, systemGetEnv },
    { 300, stringInitDefault }, { 301, stringInitBytes },
    { 302, stringInitBytesCharset },
    { 410, assetManagerOpenFd }, { 411, getPackageName },
    { 412, getPackageName },
    { 500, byteBufferAllocate }, { 501, byteBufferAllocateDirect },
    { 502, byteBufferWrap }, { 504, byteBufferOrder },
    { 505, byteBufferPut }, { 506, byteBufferGet },
    { 507, byteBufferFlip }, { 508, byteBufferRewind },
    { 509, byteBufferClear }, { 510, byteBufferPosition },
    { 511, byteBufferLimit }, { 512, byteBufferCompact },
    { 513, byteBufferAsReadOnlyBuffer }, { 514, byteBufferSlice },
    { 515, byteBufferDuplicate }, { 516, byteBufferPutBuffer },
    { 601, integerValueOf }, { 613, integerValueOf },
    { 614, integerValueOf }, { 615, getPackageName },
    { 700, classNameGetName }, { 701, classGetMethod },
    { 702, classGetDeclaredMethod }, { 703, classGetField },
    { 704, classGetDeclaredField }, { 705, classGetConstructor },
    { 706, classNewInstance }, { 710, classGetComponentType },
    { 711, classGetSuperclass }, { 712, classForName },
    { 720, objectGetClass }, { 721, objectNotifyAll },
    { 722, objectWait }, { 723, objectGetClass },
    { 800, getWindowManager }, { 801, getDefaultDisplay },
    { 804, getDisplayMetrics }, { 805, getResources },
    { 806, getConfiguration }, { 808, getAssets },
    { 809, getApplicationContext }, { 810, getPackageManager },
    { 811, getAppPackageInfo }, { 812, getPackageVersionName },
    { 900, getPackageName }, { 901, handlerPost },
    { 1000, methodInvoke }, { 1001, methodGetParameterTypes },
    { 1002, methodGetReturnType },
};
MethodsShort methodsShort[] = {};
MethodsVoid methodsVoid[] = {
    { 120, onCallbackVoid }, { 121, releaseReference },
    { 123, postUICallback },
    { 130, audioNoop }, { 131, audioNoop }, { 132, audioNoop },
    { 133, audioNoop }, { 134, audioNoop },
    { 136, audioSetVolume }, { 139, audioSetPlaybackRate },
    { 400, closeInputStream }, { 404, markInputStream },
    { 405, resetInputStream }, { 517, byteBufferGetBuffer },
    { 723, audioNoop }, { 1300, audioSetPlaybackHeadPosition },
    { 1304, audioNoop }, { 1305, audioNoop },
    { 143, prefsSetString }, { 144, prefsSetInt },
    { 145, prefsSetBool }, { 148, prefsSetString },
    { 149, prefsSetInt }, { 150, prefsFlush },
    { 152, prefsSetString }, { 153, prefsClear },
    { 1600, audioNoop }, { 1601, audioNoop },
    { 1602, audioNoop }, { 1603, audioNoop },
    { 1604, audioNoop }, { 1605, audioNoop },
    { 1610, audioNoop }, { 1611, audioNoop },
    { 1612, audioNoop }, { 1613, audioNoop },
    { 1614, audioNoop }, { 1615, audioNoop },
    { 1616, audioNoop }, { 1617, audioNoop }, { 1618, audioNoop },
    { 1620, audioNoop }, { 1621, audioNoop },
    { 1622, audioNoop }, { 1625, audioNoop },
    { 1630, audioNoop }, { 1631, audioNoop },
    { 1632, audioNoop }, { 1633, audioNoop },
    { 202, systemArraycopy }, { 204, systemLoadLibrary },
};

/*
 * =========================================================================
 *  Field registry
 * =========================================================================
 */

const int SDK_INT = 19;

NameToFieldID nameToFieldId[] = {
    { 0, "SDK_INT",     FIELD_TYPE_INT }, { 1, "SDK",         FIELD_TYPE_INT },
    { 2, "RELEASE",     FIELD_TYPE_OBJECT }, { 3, "MODEL", FIELD_TYPE_OBJECT },
    { 4, "MANUFACTURER",FIELD_TYPE_OBJECT }, { 5, "WINDOW_SERVICE", FIELD_TYPE_OBJECT },
    { 6, "DISPLAY",     FIELD_TYPE_OBJECT }, { 7, "BOARD", FIELD_TYPE_OBJECT },
    { 8, "BRAND",       FIELD_TYPE_OBJECT }, { 9, "DEVICE", FIELD_TYPE_OBJECT },
    { 10, "HARDWARE",   FIELD_TYPE_OBJECT }, { 11, "PRODUCT", FIELD_TYPE_OBJECT },
    { 12, "FINGERPRINT",FIELD_TYPE_OBJECT }, { 13, "TYPE", FIELD_TYPE_OBJECT },
    { 14, "TAGS",       FIELD_TYPE_OBJECT }, { 15, "ID", FIELD_TYPE_OBJECT },
    { 16, "SERIAL",     FIELD_TYPE_OBJECT }, { 17, "USER", FIELD_TYPE_OBJECT },
    { 18, "CODENAME",   FIELD_TYPE_OBJECT }, { 19, "INCREMENTAL", FIELD_TYPE_OBJECT },
    { 20, "density",    FIELD_TYPE_INT }, { 21, "widthPixels", FIELD_TYPE_INT },
    { 22, "heightPixels",FIELD_TYPE_INT }, { 23, "scaledDensity", FIELD_TYPE_FLOAT },
    { 24, "xdpi",       FIELD_TYPE_FLOAT }, { 25, "ydpi", FIELD_TYPE_FLOAT },
    { 26, "flags",      FIELD_TYPE_INT }, { 27, "orientation", FIELD_TYPE_INT },
    { 28, "mcc",        FIELD_TYPE_INT }, { 29, "mnc", FIELD_TYPE_INT },
    { 30, "locale",     FIELD_TYPE_OBJECT }, { 31, "screenLayout", FIELD_TYPE_INT },
    { 32, "fontScale",  FIELD_TYPE_FLOAT }, { 33, "touchscreen", FIELD_TYPE_INT },
    { 34, "keyboard",   FIELD_TYPE_INT }, { 35, "navigation", FIELD_TYPE_INT },
    { 36, "packageName",FIELD_TYPE_OBJECT }, { 37, "versionName", FIELD_TYPE_OBJECT },
    { 38, "versionCode", FIELD_TYPE_INT }, { 39, "applicationInfo", FIELD_TYPE_OBJECT },
    { 40, "metaData",   FIELD_TYPE_OBJECT }, { 41, "configChanges", FIELD_TYPE_INT },
    { 42, "targetSdkVersion", FIELD_TYPE_INT }, { 43, "minSdkVersion", FIELD_TYPE_INT },
    { 44, "requestedOrientation", FIELD_TYPE_INT }, { 45, "screenOrientation", FIELD_TYPE_INT },
    { 46, "config",     FIELD_TYPE_OBJECT },
    { 47, "__haxeHandle", FIELD_TYPE_LONG },
    { 48, "assetManager", FIELD_TYPE_OBJECT },
};

char FIELD_RELEASE[]        = "4.4.2";
char FIELD_MODEL[]          = "PSVita";
char FIELD_MANUFACTURER[]   = "Sony";
char FIELD_WINDOW_SERVICE[] = "window";
char FIELD_DISPLAY[]        = "Vita_Display";
char FIELD_BOARD[]          = "vita3k";
char FIELD_BRAND[]          = "Sony";
char FIELD_DEVICE[]         = "vita";
char FIELD_HARDWARE[]       = "vita";
char FIELD_PRODUCT[]        = "vita";
char FIELD_FINGERPRINT[]    = "Sony/PSVita/vita:4.4.2/KTU84Q/12345678:user/release-keys";
char FIELD_TYPE_STR[]       = "user";
char FIELD_TAGS[]           = "release-keys";
char FIELD_ID_STR[]         = "KTU84Q";
char FIELD_SERIAL[]         = "N/A";
char FIELD_USER[]           = "build_user";
char FIELD_CODENAME[]       = "REL";
char FIELD_INCREMENTAL[]    = "12345678";
char FIELD_LOCALE[]         = "en_US";
char FIELD_PACKAGENAME[]    = "com.armorgames.sodadungeon";

FieldsBoolean fieldsBoolean[] = {};
FieldsByte   fieldsByte[]   = {};
FieldsChar   fieldsChar[]   = {};
FieldsDouble fieldsDouble[] = {};
FieldsFloat  fieldsFloat[]  = {
    { 23, 2.0f }, { 24, 192.0f }, { 25, 192.0f }, { 32, 1.0f },
};
FieldsInt    fieldsInt[]    = {
    { 0, SDK_INT }, { 1, SDK_INT }, { 20, 2 }, { 21, 960 }, { 22, 544 },
    { 26, 0 }, { 27, 0 }, { 28, 310 }, { 29, 260 }, { 31, 3 },
    { 33, 1 }, { 34, 1 }, { 35, 1 }, { 38, 1244 }, { 41, 0 },
    { 42, 19 }, { 43, 14 }, { 44, 0 }, { 45, 1 },
};
FieldsLong   fieldsLong[]   = {
    { 47, 0 },
};
FieldsObject fieldsObject[] = {
    { 2, FIELD_RELEASE }, { 3, FIELD_MODEL }, { 4, FIELD_MANUFACTURER },
    { 5, FIELD_WINDOW_SERVICE }, { 6, FIELD_DISPLAY }, { 7, FIELD_BOARD },
    { 8, FIELD_BRAND }, { 9, FIELD_DEVICE }, { 10, FIELD_HARDWARE },
    { 11, FIELD_PRODUCT }, { 12, FIELD_FINGERPRINT }, { 13, FIELD_TYPE_STR },
    { 14, FIELD_TAGS }, { 15, FIELD_ID_STR }, { 16, FIELD_SERIAL },
    { 17, FIELD_USER }, { 18, FIELD_CODENAME }, { 19, FIELD_INCREMENTAL },
    { 30, FIELD_LOCALE }, { 36, FIELD_PACKAGENAME }, { 37, FIELD_RELEASE },
    { 39, FIELD_PACKAGENAME }, { 40, FIELD_PACKAGENAME }, { 46, FIELD_PACKAGENAME },
    { 48, (jobject)0x42424242 },
};
FieldsShort  fieldsShort[]  = {};

__FALSOJNI_IMPL_CONTAINER_SIZES
