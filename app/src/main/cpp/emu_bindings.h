/*
 *
 *  Emulator bindings
 *
 */
#ifndef _EMU_BINDINGS_H
#define _EMU_BINDINGS_H

#ifdef EMU_BUILD_DLL
#define DLLBINDING __declspec(dllexport)
#else
#define DLLBINDING /* */
#endif

#define VKEY_CONSOLE_RESET 0x10000
#define VKEY_CONSOLE_SELECT 0x20000

typedef struct
{
    const void* video_buffer;
    int video_width;
    int video_height;
    int video_ystart;
    const uint32_t* palette;
} emu_update_info_t;

extern "C" int DLLBINDING emu_init(const char* prefs, int flags);
extern "C" int DLLBINDING emu_input(int keyCode, int state);
extern "C" int DLLBINDING emu_load(int data_type, const void* data, int data_size, const char* filename);
extern "C" int DLLBINDING emu_store(int data_type, void* buffer, int buffer_size);
extern "C" int DLLBINDING emu_command(int command, int param);
extern "C" int DLLBINDING emu_get(const char* key, char* buffer, int buffer_size);

extern "C" int DLLBINDING emu_update_input(int joystickInput, int flags);
extern "C" int DLLBINDING emu_update_audio(void* buffer, int bufferLen, int flags);
extern "C" int DLLBINDING emu_update_video(emu_update_info_t* update_info, int flags);

extern "C" int DLLBINDING emu_shutdown();

#ifndef __DEF_ANDROID_LOGGING
#define __DEF_ANDROID_LOGGING

#include <android/log.h>

#define  ANDROID_LOG_TAG    "droid2600"

#define  LOG(...)  __android_log_print(ANDROID_LOG_INFO, ANDROID_LOG_TAG, __VA_ARGS__)
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG, ANDROID_LOG_TAG, __VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, ANDROID_LOG_TAG, __VA_ARGS__)

#endif

#endif /* _EMU_BINDINGS_H */
