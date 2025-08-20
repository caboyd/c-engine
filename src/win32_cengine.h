#ifndef WIN32_CENGINE_H
#define WIN32_CENGINE_H

#include "cengine.h"

const IID IID_IAudioClient = {0x1CB9AD4C, 0xDBFA, 0x4c32, {0xB1, 0x78, 0xC2, 0xF5, 0x68, 0xA7, 0x03, 0xB2}};
const GUID IID_IAudioRenderClient = {0xF294ACFC, 0x3146, 0x4483, {0xA7, 0xBF, 0xAD, 0xDC, 0xA7, 0xC2, 0x60, 0xE2}};
const GUID CLSID_MMDeviceEnumerator = {0xBCDE0395, 0xE52F, 0x467C, {0x8E, 0x3D, 0xC4, 0x57, 0x92, 0x91, 0x69, 0x2E}};
const GUID IID_IMMDeviceEnumerator = {0xA95664D2, 0x9614, 0x4F35, {0xA7, 0x46, 0xDE, 0x8D, 0xB6, 0x36, 0x17, 0xE6}};
const GUID PcmSubformatGuid = {0x00000001, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71}};

typedef struct win32_offscreen_buffer Win32_Offscreen_Buffer;
struct win32_offscreen_buffer
{
  BITMAPINFO info;
  void* memory;
  S32 width;
  S32 height;
  S32 pitch;
  S32 bytes_per_pixel;
};

typedef struct Win32_Window_Dimension Win32_Window_Dimension;
struct Win32_Window_Dimension
{
  S32 width;
  S32 height;
};

typedef enum Wasapi_Status
{
  WASAPI_OK = 0,
  WASAPI_ERR_DEVICE_ENUM,
  WASAPI_ERR_AUDIO_ENDPOINT,
  WASAPI_ERR_ACTIVATE,
  WASAPI_ERR_GET_MIX_FORMAT,
  WASAPI_ERR_INIT,
  WASAPI_ERR_GET_SERVICE,
  WASAPI_ERR_START,
  WASAPI_ERR_GET_BUF_SIZE,
  WASAPI_STATUS_COUNT,
} Wasapi_Status;

typedef enum Wasapi_Sample_Format
{
  WASAPI_SAMPLE_FORMAT_UNKNOWN = 0,
  WASAPI_SAMPLE_FORMAT_PCM,
  WASAPI_SAMPLE_FORMAT_FLOAT,
  WASAPI_SAMPLE_FORMAT_COUNT,
} Wasapi_Sample_Format;

typedef struct Wasapi_Context Wasapi_Context;
struct Wasapi_Context
{
  IMMDeviceEnumerator* device_enumerator;
  IMMDevice* device;
  IAudioClient* audio_client;
  IAudioRenderClient* render_client;
  WAVEFORMATEX* wave_format;
  Wasapi_Sample_Format sample_format;
  U32 buffer_frame_count;
  U8* sample_buffer;
};

typedef struct Win32_Engine_Code Win32_Engine_Code;

struct Win32_Engine_Code
{
  HMODULE engine_code_dll;
  FILETIME dll_last_write_time;
  B32 is_valid;

  //IMPORTANT: Functions may be NULL, check before calling
  Game_Update_And_Render_Func* Update_And_Render;
  Game_Get_Sound_Samples_Func* Get_Sound_Samples;
};

#define WIN32_STATE_FILE_NAME_COUNT MAX_PATH
typedef struct Win32_State Win32_State;
struct Win32_State
{
  U64 total_size;
  void* game_memory_block;

  HANDLE recording_handle;
  S32 input_recording_index;

  HANDLE playback_handle;
  S32 input_playing_index;

  char build_directory[WIN32_STATE_FILE_NAME_COUNT];
  char data_directory[WIN32_STATE_FILE_NAME_COUNT];
};

#endif
