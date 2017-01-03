/*
 *
 *  Emulator bindings
 *
 */

#include <stdlib.h>
#include <memory.h>
#include <jni.h>
#include <cstdio>
#include <emu_adapter.h>

#include "./emu_bindings.h"

static const int rawVideoBufferSize = 160 * 320 * 4;
static void* rawVideoBuffer = NULL;

static int rawAudioBufferSize = 0;
static void* rawAudioBuffer = NULL;

typedef struct
{
    uint32_t render_width;
    uint32_t render_height;
    uint32_t render_ystart;
} emu_stats_t;

static emu_stats_t emuStats;

static bool emuReady = false;

static void setAudioBuffer(int sz)
{
    if (sz > 0 && sz <= rawAudioBufferSize)
    {
        return;
    }

    if (NULL != rawAudioBuffer)
    {
        delete [] (unsigned char*) rawAudioBuffer;
        rawAudioBuffer = NULL;
    }

    if (sz > 0)
    {
        rawAudioBuffer = new unsigned char[sz];
    }

    rawAudioBufferSize = sz;
}

extern "C"
{

JNIEXPORT jint JNICALL Java_emu_NativeInterface_init(JNIEnv* env, jobject obj, jstring prefs, jint flags)
{
    if (NULL == rawVideoBuffer)
    {
        rawVideoBuffer = new unsigned char[rawVideoBufferSize];
    }

    memset(rawVideoBuffer, 0, rawVideoBufferSize);

    //setAudioBuffer(4096); // should be more than enough for one fragment

    const char *nativeString = env->GetStringUTFChars(prefs, 0);

    int result = emu_init(nativeString, (int) flags);

    env->ReleaseStringUTFChars(prefs, nativeString);

    return result;
}

JNIEXPORT jint JNICALL Java_emu_NativeInterface_input(JNIEnv* env, jobject obj, jint keyCode, jint state)
{
    return emu_input((int) keyCode, (int) state);
}

JNIEXPORT jint JNICALL Java_emu_NativeInterface_load(JNIEnv* env, jobject obj,
                                                                 jint dataType,
                                                                 jbyteArray data,
                                                                 jint dataSize,
                                                                 jstring filename)
{
    emuReady = false;

    jboolean isCopy;
    jbyte* rawjBytes = env->GetByteArrayElements(data, &isCopy);
    const char *nativeString = env->GetStringUTFChars(filename, 0);

    int result = emu_load((int) dataType, rawjBytes, (int) dataSize, nativeString);

    env->ReleaseByteArrayElements(data, rawjBytes, 0);
    env->ReleaseStringUTFChars(filename, nativeString);

    emuReady = (0 == result);

    return result;
}

JNIEXPORT jint JNICALL Java_emu_NativeInterface_store(JNIEnv* env, jobject obj,
                                                                  jint dataType,
                                                                  jbyteArray data,
                                                                  jint dataSize)
{
    jboolean isCopy;
    jbyte* rawjBytes = env->GetByteArrayElements(data, &isCopy);

    int result = emu_store((int) dataType, (void*) rawjBytes, (int) dataSize);

    env->ReleaseByteArrayElements(data, rawjBytes, 0);

    return result;
}

JNIEXPORT jint JNICALL Java_emu_NativeInterface_command(JNIEnv* env, jobject obj, jint command, jint param)
{
    return emu_command((int) command, (int) param);
}

JNIEXPORT jint JNICALL Java_emu_NativeInterface_updateInput(JNIEnv* env, jobject obj,
                                                       jint joystickInput,
                                                       int flags)
{
    if (false == emuReady) return 0;

    return emu_update_input((int) joystickInput, (int) flags);
}

JNIEXPORT jint JNICALL Java_emu_NativeInterface_updateVideo(JNIEnv* env, jobject obj,
                                                       jbyteArray videoOutput,
                                                       jbyteArray emuStatsBuffer,
                                                       int flags)
{
    if (false == emuReady) return 0;

    emu_update_info_t updateInfo;

    int result = emu_update_video(&updateInfo, (int) flags);

    if (0 == result) return 0;

    int w = updateInfo.video_width;
    int h = updateInfo.video_height;
    int s = updateInfo.video_ystart;
    const uint32_t* palette = updateInfo.palette;

    ////

    const uint8_t* src = (const uint8_t*) updateInfo.video_buffer;
    uint32_t* dest = (uint32_t*) rawVideoBuffer;

    //LOG("EmuBindings frame: %p (%d/%d/%d)", (const void*) src, w, h, s);

    for (int y=0; y<h; y++)
    {
        const uint8_t* srcLine = src + w*y;
        uint32_t* destLine = dest + 160*y; // fixed width: 160 pixels

        for (int x=0; x<w; x++)
        {
            uint8_t p = *(srcLine++);
            uint32_t c = palette[p];
            *(destLine++) = 0xFF000000 | c;
        }
    }

    emuStats.render_width = (uint32_t) updateInfo.video_width;
    emuStats.render_height = (uint32_t) updateInfo.video_height;
    emuStats.render_ystart = (uint32_t) updateInfo.video_ystart;

    env->SetByteArrayRegion(emuStatsBuffer, 0, (jsize) sizeof(emuStats), (const jbyte*) &emuStats);
    env->SetByteArrayRegion(videoOutput, 0, (jsize) rawVideoBufferSize, (const jbyte*) rawVideoBuffer);

    return result;
}

JNIEXPORT jint JNICALL Java_emu_NativeInterface_updateAudio(JNIEnv* env, jobject obj,
                                                       jbyteArray audioOutputBuffer,
                                                       jint audioOutputBufferSize)
{
    if (false == emuReady)
    {
        return 0;
    }

    setAudioBuffer(audioOutputBufferSize);
    if (NULL == audioOutputBuffer)
    {
        return 0;
    }

    //if (NULL == audioOutputBuffer || audioOutputBufferSize > rawAudioBufferSize) return 0;

    int result = emu_update_audio(rawAudioBuffer, (int) audioOutputBufferSize, 0);
    if (0 == result)
    {
        return 0;
    }

    env->SetByteArrayRegion(audioOutputBuffer, 0, (jsize) audioOutputBufferSize, (const jbyte*) rawAudioBuffer);

    return result;
}

JNIEXPORT jstring JNICALL Java_emu_NativeInterface_get(JNIEnv* env, jobject obj, jstring key)
{
    char buf[512];

    const char* cstrKey = env->GetStringUTFChars(key, NULL);

    emu_get(cstrKey, buf, sizeof(buf));

    env->ReleaseStringUTFChars(key, cstrKey);

    jstring result = env->NewStringUTF(buf);

    return result;
}

JNIEXPORT jint JNICALL Java_emu_NativeInterface_shutdown(JNIEnv* env, jobject obj)
{
    emuReady = false;

    if (rawVideoBuffer)
    {
        delete [] (unsigned char*) rawVideoBuffer;
        rawVideoBuffer = NULL;
    }

    setAudioBuffer(0);

    int result = emu_shutdown();

    return result;
}

} // extern "C"
