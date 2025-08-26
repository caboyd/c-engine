#define WIN32_LEAN_AND_MEAN

#include <stdio.h>
#include <windows.h>
#include <winioctl.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <xinput.h>

#include "base/base_core.h"
#include "base/base_math.h"

#include "cengine_platform.h"
#include "win32_cengine.h"
//----------------c files ---------------------------------

#define WINDOW_WIDTH 960
#define WINDOW_HEIGHT 540

//----------------Globals----------------------
//
global B32 global_running;
global B32 global_pause;
global Win32_Offscreen_Buffer global_back_buffer;
global U64 global_perf_count_frequency;
global F32 global_target_seconds_per_frame;
global S32 global_window_offset = 10;
//---------------------------------------------

internal void Win32_Prepend_Build_Path(Win32_State* state, char* dest, S32 dest_len, char* file_name)
{
  cstring_cat(dest, dest_len, state->build_directory, dest_len, file_name, cstring_len(file_name));
}

DEBUG_PLATFORM_FREE_FILE_MEMORY(DEBUG_Platform_Free_File_Memory)
{
  if (memory)
  {
    VirtualFree(memory, 0, MEM_RELEASE);
  }
}

DEBUG_PLATFORM_READ_ENTIRE_FILE(DEBUG_Platform_Read_Entire_File)

{
  Debug_Read_File_Result result = {};
  HANDLE file_handle = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);
  if (file_handle != INVALID_HANDLE_VALUE)
  {
    LARGE_INTEGER file_size;
    if (GetFileSizeEx(file_handle, &file_size))
    {
      U32 file_size_32 = Safe_Truncate_U64((U64)file_size.QuadPart);
      result.contents = VirtualAlloc(0, file_size_32, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
      if (result.contents)
      {
        DWORD bytes_read;
        if (ReadFile(file_handle, result.contents, file_size_32, &bytes_read, 0) && (file_size_32 == bytes_read))
        {
          result.contents_size = file_size_32;
        }
        else
        {
          DEBUG_Platform_Free_File_Memory(thread, result.contents);
          result.contents = 0;
        }
      }
      else
      {
        // TODO: Logging
      }
    }
    else
    {
      // TODO: Logging
    }
    CloseHandle(file_handle);
  }
  else
  {
    // TODO: Logging
  }
  return result;
}

DEBUG_PLATFORM_WRITE_ENTIRE_FILE(DEBUG_Platform_Write_Entire_File)
{
  B32 result = false;
  HANDLE file_handle = CreateFileA(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
  if (file_handle != INVALID_HANDLE_VALUE)
  {

    DWORD bytes_written;
    if (WriteFile(file_handle, memory, memory_size, &bytes_written, 0))
    {
      result = (bytes_written == memory_size);
    }
    else
    {
      // TODO: Logging
    }

    CloseHandle(file_handle);
  }
  else
  {
    // TODO: Logging
  }
  return result;
}

internal void Win32_WASAPI_Cleanup(Wasapi_Context* ctx)
{
  if (!ctx)
    return;

  if (ctx->wave_format)
  {
    CoTaskMemFree(ctx->wave_format);
  }
  if (ctx->render_client)
  {
    ctx->render_client->lpVtbl->Release(ctx->render_client);
  }
  if (ctx->audio_client)
  {
    ctx->audio_client->lpVtbl->Release(ctx->audio_client);
  }
  if (ctx->device)
  {
    ctx->device->lpVtbl->Release(ctx->device);
  }
  if (ctx->device_enumerator)
  {
    ctx->device_enumerator->lpVtbl->Release(ctx->device_enumerator);
  }

  CoUninitialize();
}

internal Wasapi_Status Win32_WASAPI_Init(Wasapi_Context* ctx)
{
  Wasapi_Status status = WASAPI_OK;
  HRESULT hr;
  if (status == WASAPI_OK)
  {
    hr = CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, &IID_IMMDeviceEnumerator,
                          (void**)&(ctx->device_enumerator));
    if (FAILED(hr))
    {
      status = WASAPI_ERR_DEVICE_ENUM;
    }
  }
  if (status == WASAPI_OK)
  {
    hr = ctx->device_enumerator->lpVtbl->GetDefaultAudioEndpoint(ctx->device_enumerator, eRender, eConsole,
                                                                 &(ctx->device));
    if (FAILED(hr))
    {
      status = WASAPI_ERR_AUDIO_ENDPOINT;
    }
  }

  if (status == WASAPI_OK)
  {
    hr = ctx->device->lpVtbl->Activate(ctx->device, &IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&(ctx->audio_client));
    if (FAILED(hr))
    {
      status = WASAPI_ERR_ACTIVATE;
    }
  }

  if (status == WASAPI_OK)
  {
    hr = ctx->audio_client->lpVtbl->GetMixFormat(ctx->audio_client, &(ctx->wave_format));
    if (FAILED(hr))
    {
      status = WASAPI_ERR_GET_MIX_FORMAT;
    }
    if (ctx->wave_format->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
    {
      WAVEFORMATEXTENSIBLE* wf_ext = (WAVEFORMATEXTENSIBLE*)ctx->wave_format;
      if (IsEqualGUID(&wf_ext->SubFormat, &KSDATAFORMAT_SUBTYPE_IEEE_FLOAT))
      {
        ctx->sample_format = WASAPI_SAMPLE_FORMAT_FLOAT;
      }
      else if (IsEqualGUID(&wf_ext->SubFormat, &KSDATAFORMAT_SUBTYPE_PCM))
      {
        ctx->sample_format = WASAPI_SAMPLE_FORMAT_PCM;
      }
      else
      {
        ctx->sample_format = WASAPI_SAMPLE_FORMAT_UNKNOWN;
      }
    }
  }

  if (status == WASAPI_OK)
  {
    // time in 100ns increments
    REFERENCE_TIME buffer_duration;
    // 26667 = 2.6667 ms is my device perioud
    hr = ctx->audio_client->lpVtbl->GetDevicePeriod(ctx->audio_client, 0, &(buffer_duration));

    S32 one_second = 10000000;
    // NOTE: set buffer size twice as much as we need
    // We still only use 1 frame worth plus a tiny big of padding, not the whole 2x sized buffer
    // 1600 frames at 48khz is 1/30th of a frame
    S32 min_buffer_duration = (S32)((F32)one_second * global_target_seconds_per_frame) * 2;
    if (buffer_duration < min_buffer_duration)
    {
      buffer_duration = min_buffer_duration;
    }

    hr = ctx->audio_client->lpVtbl->Initialize(ctx->audio_client, AUDCLNT_SHAREMODE_SHARED, 0, buffer_duration, 0,
                                               ctx->wave_format, NULL);
    if (FAILED(hr))
    {
      status = WASAPI_ERR_INIT;
    }
  }
  if (status == WASAPI_OK)
  {
    // minimum appears to be 1056 frames or 22ms of buffer
    hr = ctx->audio_client->lpVtbl->GetBufferSize(ctx->audio_client, &(ctx->buffer_frame_count));
    if (FAILED(hr))
    {
      status = WASAPI_ERR_GET_BUF_SIZE;
    }
  }

  if (status == WASAPI_OK)
  {
    hr = ctx->audio_client->lpVtbl->GetService(ctx->audio_client, &IID_IAudioRenderClient,
                                               (void**)&(ctx->render_client));
    if (FAILED(hr))
    {
      status = WASAPI_ERR_GET_SERVICE;
    }
  }
  if (status == WASAPI_OK)
  {
    hr = ctx->audio_client->lpVtbl->Start(ctx->audio_client);
    if (FAILED(hr))
    {
      status = WASAPI_ERR_START;
    }
  }
  if (status != WASAPI_OK)
  {
    printf("WASAPI ERROR: %d", status);
    Win32_WASAPI_Cleanup(ctx);
  }
  return status;
}

internal void Win32_Setup_Sound_Buffer(Wasapi_Context* ctx, Game_Output_Sound_Buffer* sound_buffer)
{
  sound_buffer->channel_count = ctx->wave_format->nChannels;
  if (ctx->sample_format == WASAPI_SAMPLE_FORMAT_FLOAT)
  {
    sound_buffer->sample_format = SF_F32;
  }
  else if (ctx->sample_format == WASAPI_SAMPLE_FORMAT_PCM)
  {
    if (ctx->wave_format->wBitsPerSample == 16)
    {
      sound_buffer->sample_format = SF_PCM_S16;
    }
    else if (ctx->wave_format->wBitsPerSample == 24)
    {
      sound_buffer->sample_format = SF_PCM_S24;
    }
  }
  if (sound_buffer->sample_format == SF_F32)
  {
    sound_buffer->bytes_per_sample = 4;
  }
  else if (sound_buffer->sample_format == SF_PCM_S24)
  {
    sound_buffer->bytes_per_sample = 3;
  }
  else
  {
    sound_buffer->bytes_per_sample = 2;
  }; // if(sample_format == SF_PCM_S16) return 2;
  sound_buffer->samples_per_second = (S32)ctx->wave_format->nSamplesPerSec;
  // calculate how many samples are need based on game update rate
  F32 samples_requested = (F32)ctx->wave_format->nSamplesPerSec * global_target_seconds_per_frame;
  // NOTE: adding a small buffer to prevent startup clicking
  sound_buffer->sample_count = (S32)samples_requested + 256;
}

internal void Win32_Output_Sound(Wasapi_Context* ctx, Game_Output_Sound_Buffer* sound_buffer)
{
  U32 padding;
  HRESULT hr = ctx->audio_client->lpVtbl->GetCurrentPadding(ctx->audio_client, &padding);
  if (FAILED(hr))
  {
    // TODO: Logging
  }

  U32 frames_available = ctx->buffer_frame_count - padding;
  U32 samples_available = (U32)sound_buffer->sample_count;
  S32 samples_requested = (S32)((F32)ctx->wave_format->nSamplesPerSec * global_target_seconds_per_frame);

  hr = ctx->render_client->lpVtbl->GetBuffer(ctx->render_client, frames_available, &ctx->sample_buffer);
  if (FAILED(hr))
  {
    // TODO: Logging
  }

  S32 min_padding = 64;
  S32 max_padding = min_padding + 164;
  // NOTE:Straddle low latency
  if (padding < (U32)min_padding)
  {
    sound_buffer->sample_count = (S32)samples_requested + 40;
    // printf("**low  samples frame_count: %d sample_count: %d, frame_latency: %d \n", ctx->buffer_frame_count,
    //        sound_buffer->sample_count, padding);
  }
  else if (padding > (U32)min_padding && padding < (U32)max_padding)
  {
    sound_buffer->sample_count = (S32)samples_requested + 2;
    // printf("**norm samples frame_count: %d sample_count: %d, frame_latency: %d \n", ctx->buffer_frame_count,
    //        sound_buffer->sample_count, padding);
  }
  else if (padding > (U32)max_padding)
  {
    sound_buffer->sample_count = (S32)samples_requested - 2;
    // printf("**high samples frame_count: %d sample_count: %d, frame_latency: %d \n", ctx->buffer_frame_count,
    //        sound_buffer->sample_count, padding);
  }

  ASSERT(frames_available >= samples_available);

  S32 channels = sound_buffer->channel_count;
  S32 bits_per_sample = sound_buffer->bytes_per_sample * 8;
  // Copy sound_buffer->sample_buffer to ctx->sample_buffer
  for (S32 frame = 0; frame < (S32)samples_available; ++frame)
  {
    S32 per_frame_offset = frame * channels * sound_buffer->bytes_per_sample;

    U8* src_ptr = sound_buffer->sample_buffer + per_frame_offset;
    U8* dest_ptr = ctx->sample_buffer + per_frame_offset;
    for (S32 ch = 0; ch < channels; ++ch)
    {
      if (bits_per_sample == 32)
      {
        *((F32*)(void*)dest_ptr) = *((F32*)(void*)src_ptr);
        // FIXME:temp trick to make less harsh sound when closing window
        // still makes click sound though
        if (global_running == false)
        {
          *((F32*)(void*)dest_ptr) = 0;
        };
        dest_ptr += sizeof(F32);
        src_ptr += sizeof(F32);
      }
      else if (bits_per_sample == 16)
      {
        *((S16*)(void*)dest_ptr) = *((S16*)(void*)src_ptr);
        dest_ptr += sizeof(S16);
        src_ptr += sizeof(S16);
      }
      else if (bits_per_sample == 24)
      {
        *((S8*)dest_ptr++) = *((S8*)src_ptr++);
        *((S8*)dest_ptr++) = *((S8*)src_ptr++);
        *((S8*)dest_ptr++) = *((S8*)src_ptr++);
      }
    }
  }

  hr = ctx->render_client->lpVtbl->ReleaseBuffer(ctx->render_client, samples_available, 0);
  if (FAILED(hr))
  {
    // TODO: Logging
  }
}

//-----------------X Input------------------------

#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD user_index, XINPUT_STATE* state)
typedef X_INPUT_GET_STATE(X_Input_Get_State);
X_INPUT_GET_STATE(X_Input_Get_State_Stub)
{
  return ERROR_DEVICE_NOT_CONNECTED;
}

#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD user_index, XINPUT_VIBRATION* vibration)
typedef X_INPUT_SET_STATE(X_Input_Set_State);
X_INPUT_SET_STATE(X_Input_Set_State_Stub)
{
  return ERROR_DEVICE_NOT_CONNECTED;
}

global X_Input_Get_State* XInputGetState_ = X_Input_Get_State_Stub;
#define XInputGetState XInputGetState_

global X_Input_Set_State* XInputSetState_ = X_Input_Set_State_Stub;
#define XInputSetState XInputSetState_

internal FILETIME Win32_Get_Last_Write_Time(char* file_name)
{
  FILETIME last_write_time = {};

#if 0
  WIN32_FIND_DATA find_data;
  HANDLE find_handle = FindFirstFileA(file_name, &find_data);
  if (find_handle != INVALID_HANDLE_VALUE)
  {
    last_write_time = find_data.ftLastWriteTime;
    FindClose(find_handle);
  }
#else
  WIN32_FILE_ATTRIBUTE_DATA data;
  if (GetFileAttributesEx(file_name, GetFileExInfoStandard, &data))
  {
    last_write_time = data.ftLastWriteTime;
  }
#endif
  return last_write_time;
}

internal Win32_Engine_Code Win32_Load_Engine_Code(char* source_dll_path, char* temp_dll_path)
{
  Win32_Engine_Code engine_code;

  engine_code.dll_last_write_time = Win32_Get_Last_Write_Time(source_dll_path);

  CopyFile(source_dll_path, temp_dll_path, FALSE);
  engine_code.engine_code_dll = LoadLibraryA(temp_dll_path);
  if (engine_code.engine_code_dll)
  {
    engine_code.Update_And_Render =
        (Game_Update_And_Render_Func*)(void*)GetProcAddress(engine_code.engine_code_dll, "Game_Update_And_Render");
    engine_code.Get_Sound_Samples =
        (Game_Get_Sound_Samples_Func*)(void*)GetProcAddress(engine_code.engine_code_dll, "Game_Get_Sound_Samples");

    engine_code.is_valid = engine_code.Update_And_Render && engine_code.Get_Sound_Samples;
  }
  if (!engine_code.is_valid)
  {
    engine_code.Get_Sound_Samples = 0;
    engine_code.Update_And_Render = 0;
  }
  return engine_code;
}

internal void Win32_Unload_Engine_Code(Win32_Engine_Code* engine_code)
{
  if (engine_code)
  {
    FreeLibrary(engine_code->engine_code_dll);
  }
  engine_code->engine_code_dll = 0;
  engine_code->is_valid = false;
  engine_code->Get_Sound_Samples = 0;
  engine_code->Update_And_Render = 0;
}

internal void Win32_Load_XInput(void)
{
  HMODULE xinput_lib = LoadLibraryA("xinput1_4.dll");
  if (!xinput_lib)
  {
    xinput_lib = LoadLibraryA("xinput1_3.dll");
  }
  if (xinput_lib)
  {
    XInputGetState = (X_Input_Get_State*)(void*)GetProcAddress(xinput_lib, "XInputGetState");
    XInputSetState = (X_Input_Set_State*)(void*)GetProcAddress(xinput_lib, "XInputSetState");
  }
}

//--------Definitions----------------------
internal Win32_Window_Dimension Win32_Get_Window_Dimension(HWND window)
{
  Win32_Window_Dimension result;

  RECT client_rect;
  GetClientRect(window, &client_rect);

  result.width = client_rect.right - client_rect.left;
  result.height = client_rect.bottom - client_rect.top;

  return result;
}
internal void Win32_Resize_DIB_Section(Win32_Offscreen_Buffer* buffer, int width, int height)
{
  if (buffer->memory)
  {
    VirtualFree(buffer->memory, 0, MEM_RELEASE);
  }

  buffer->width = width;
  buffer->height = height;

  memset(&buffer->info, 0, sizeof(buffer->info));
  MEM_ZERO(buffer->info);
  buffer->info.bmiHeader = (BITMAPINFOHEADER){.biSize = sizeof(BITMAPINFOHEADER),
                                              .biWidth = buffer->width,
                                              .biHeight = -buffer->height,
                                              .biPlanes = 1,
                                              .biBitCount = 32,
                                              .biCompression = BI_RGB};

  S32 bytes_per_pixel = 4;
  buffer->bytes_per_pixel = bytes_per_pixel;
  S32 bitmap_memory_size = bytes_per_pixel * (buffer->width * buffer->height);
  buffer->memory = VirtualAlloc(0, (SIZE_T)bitmap_memory_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
  buffer->pitch = width * bytes_per_pixel;
}

internal S32 Win32_Get_Window_Scale(Win32_Offscreen_Buffer* buffer, S32 window_width, S32 window_height)
{
  S32 scale_x = MAX((window_width / buffer->width), 1);
  S32 scale_y = MAX(1, (window_height / buffer->height));
  S32 scale = MAX(MIN(scale_x, scale_y), 1);
  return scale;
}

internal void Win32_Display_Buffer_In_Window(Win32_Offscreen_Buffer* buffer, HDC device_context, S32 window_width,
                                             S32 window_height)
{
  // StretchDIBits(
  //   device_context,
  //   x, y, width, height,
  //   x, y, width, height,
  //   &bitmap_memory,
  //   &bitmap_info,
  //   DIB_RGB_COLORS, SRCCOPY);
  //

  S32 offset = global_window_offset;
  // NOTE: Scale window buffer as multiple of buffer;
  S32 scale = Win32_Get_Window_Scale(buffer, window_width, window_height);
  S32 mod_width = scale * buffer->width;

  S32 mod_height = scale * buffer->height;

  PatBlt(device_context, 0, 0, offset, window_height, BLACKNESS);
  PatBlt(device_context, 0, 0, window_width, 10, BLACKNESS);
  PatBlt(device_context, mod_width + offset, 0, window_width, window_height, BLACKNESS);
  PatBlt(device_context, 0, mod_height + offset, window_width, window_height, BLACKNESS);
  // TODO: Aspect ratio correction
  // TODO: play with stretch modes
  StretchDIBits(device_context, offset, offset, mod_width, mod_height, 0, 0, buffer->width, buffer->height,
                buffer->memory, &buffer->info, DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK Win32_Wnd_Proc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
  LRESULT result = 0;

  switch (message)
  {
    case WM_DESTROY: // Fallthrough
    case WM_CLOSE:
    {
      // TODO: handle this as an error - recreate window?
      // TODO:handle this with a message to the user?
      global_running = false;
    }
    break;
    case WM_ACTIVATEAPP:
    {
    }
    break;
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_KEYDOWN:
    case WM_KEYUP:
    {
      ASSERT("Keyboard input came in through non dispatch method" != 0);
    }
    break;
    case WM_PAINT:
    {
      PAINTSTRUCT paint;
      HDC device_context = BeginPaint(window, &paint);
      Win32_Window_Dimension dim = Win32_Get_Window_Dimension(window);
      Win32_Display_Buffer_In_Window(&global_back_buffer, device_context, dim.width, dim.height);
      EndPaint(window, &paint);
    }

    break;
    default:
    {
      result = DefWindowProc(window, message, wParam, lParam);
    }
    break;
  }

  return result;
}

internal void Win32_Process_XInput_Stick(Vec2* stick, SHORT stick_axis_x, SHORT stick_axis_y, SHORT DEAD_ZONE)

{

  F32 x;
  F32 y;
  F32 scale = 1.f / (F32)(32767 - DEAD_ZONE);

  if (stick_axis_x > DEAD_ZONE)
  {
    x = stick_axis_x - DEAD_ZONE;
  }
  else if (stick_axis_x < -DEAD_ZONE)
  {
    x = stick_axis_x + DEAD_ZONE;
  }
  else
  {
    x = 0.f;
  }

  if (stick_axis_y > DEAD_ZONE)
  {
    y = stick_axis_y - DEAD_ZONE;
  }
  else if (stick_axis_y < -DEAD_ZONE)
  {
    y = stick_axis_y + DEAD_ZONE;
  }
  else
  {
    y = 0.f;
  }

  x *= scale;
  stick->x = x;

  y *= scale;
  stick->y = y;
}
internal void DEBUG_Win32_Delete_Recordings()
{
  S32 path_length = WIN32_STATE_FILE_NAME_COUNT;
  char exe_file_path[WIN32_STATE_FILE_NAME_COUNT];
  GetModuleFileNameA(0, exe_file_path, sizeof(exe_file_path));
  char* one_past_last_slash = exe_file_path;

  char* found_slash = exe_file_path;
  while ((found_slash = cstring_find_substr(one_past_last_slash, "\\")) != 0)
  {
    one_past_last_slash = found_slash + 1;
  }

  char build_directory[WIN32_STATE_FILE_NAME_COUNT] = {};
  char playback_pattern[WIN32_STATE_FILE_NAME_COUNT] = {};
  cstring_append(build_directory, path_length, exe_file_path, (S32)(one_past_last_slash - exe_file_path));
  cstring_cat(playback_pattern, path_length, build_directory, path_length, "playback_snapshot*",
              sizeof("playback_snapshot*") - 1);

  WIN32_FIND_DATA fd;
  HANDLE h = FindFirstFile(playback_pattern, &fd);
  if (h == INVALID_HANDLE_VALUE)
    return;

  do
  {
    if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
    {
      char full_path[WIN32_STATE_FILE_NAME_COUNT] = {};
      cstring_cat(full_path, path_length, build_directory, path_length, fd.cFileName, path_length);
      DeleteFile(full_path);
    }
  } while (FindNextFile(h, &fd));

  FindClose(h);
}

internal void Win32_Get_Input_File_Location(Win32_State* state, char* dest, S32 dest_len, S32 slot_index)
{
  char temp[128];
  wsprintf(temp, "playback_snapshot_%d.bin", slot_index);
  Win32_Prepend_Build_Path(state, dest, dest_len, temp);
}

internal B32 Win32_State_Is_Record(Win32_State* state)
{
  return state->input_recording_index != 0;
}
internal B32 Win32_State_Is_Playback(Win32_State* state)
{
  return state->input_playing_index != 0;
}

internal void Win32_Begin_Record_Input(Win32_State* state, int input_recording_index)
{
  state->input_recording_index = input_recording_index;
  char file_location[WIN32_STATE_FILE_NAME_COUNT];
  Win32_Get_Input_File_Location(state, file_location, sizeof(file_location), input_recording_index);
  state->recording_handle = CreateFileA(file_location, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);

  DWORD ignored;
  DeviceIoControl(state->recording_handle, FSCTL_SET_SPARSE, NULL, 0, NULL, 0, &ignored, 0);

  // NOTE: scan backwards to find end of file (first non zero byte)

  DWORD bytes_written;
  DWORD bytes_to_write = memory_last_nonzero_byte(state->game_memory_block, (U32)state->total_size);
  WriteFile(state->recording_handle, state->game_memory_block, bytes_to_write, &bytes_written, 0);

  // --- Skip the zeros by extending the file ---
  LARGE_INTEGER file_position;
  file_position.QuadPart = (LONGLONG)state->total_size;
  SetFilePointerEx(state->recording_handle, file_position, NULL, FILE_BEGIN);
  SetEndOfFile(state->recording_handle);
}

internal void Win32_Record_Input(Win32_State* state, Game_Input* new_input)
{
  DWORD bytes_written;
  WriteFile(state->recording_handle, new_input, sizeof(*new_input), &bytes_written, 0);
}

internal void Win32_End_Record_Input(Win32_State* state)
{

  CloseHandle(state->recording_handle);
  state->input_recording_index = 0;
}

internal void Win32_Begin_Input_Playback(Win32_State* state, int input_playing_index)
{

  char file_location[WIN32_STATE_FILE_NAME_COUNT];
  Win32_Get_Input_File_Location(state, file_location, sizeof(file_location), input_playing_index);
  state->playback_handle = CreateFileA(file_location, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);
  if (state->playback_handle == INVALID_HANDLE_VALUE)
  {
    // NOTE:playback doesnt exist yet
    return;
  }

  state->input_playing_index = input_playing_index;
  DWORD bytes_to_read = (DWORD)state->total_size;
  ASSERT(state->total_size == bytes_to_read);
  DWORD bytes_read;
  ReadFile(state->playback_handle, state->game_memory_block, bytes_to_read, &bytes_read, 0);

  LARGE_INTEGER file_position;
  file_position.QuadPart = (LONGLONG)state->total_size;
  SetFilePointerEx(state->playback_handle, file_position, NULL, FILE_BEGIN);
}

internal void Win32_End_Input_Playback(Win32_State* state)
{

  CloseHandle(state->playback_handle);
  state->input_playing_index = 0;
}

internal void Win32_Playback_Input(Win32_State* state, Game_Input* new_input)
{

  DWORD bytes_read;
  if (ReadFile(state->playback_handle, new_input, sizeof(*new_input), &bytes_read, 0))
  {
    // NOTE: still parsing input

    if (bytes_read == 0)
    {
      // NOTE: reached end of file, go back to beginning
      S32 playing_index = state->input_playing_index;
      Win32_End_Input_Playback(state);
      Win32_Begin_Input_Playback(state, playing_index);
      ReadFile(state->playback_handle, new_input, sizeof(*new_input), &bytes_read, 0);
    }
  }
}

internal void Win32_Process_Keyboard_Message(Game_Button_State* new_state, B32 is_down)
{
  if (new_state->ended_down != is_down)
  {
    new_state->ended_down = is_down;
    new_state->half_transition_count += 1;
  }
}

internal void Win32_Process_Record_Playback_Message(Win32_State* state, Game_Controller_Input* keyboard, B32 is_down,
                                                    B32 key_mod, S32 record_index)
{

  if (is_down && !Win32_State_Is_Playback(state))
  {
    if (!Win32_State_Is_Record(state))
    {
      if (key_mod)
      {
        Win32_Begin_Record_Input(state, record_index);
      }
      else
      {
        Win32_Begin_Input_Playback(state, record_index);
      }
    }
    else if (state->input_recording_index == record_index)
    {
      Win32_End_Record_Input(state);
      Win32_Begin_Input_Playback(state, record_index);
    }
  }
  else if (is_down && state->input_playing_index == record_index)
  {
    Win32_End_Input_Playback(state);
    MEM_ZERO(*keyboard);
  }
}

internal void Win32_Process_Pending_Messages(HWND window, Win32_State* state, Game_Input* mouse_input,
                                             Game_Controller_Input* keyboard_controller)
{
  MSG message;
  while (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
  {
    if (message.message == WM_QUIT)
    {
      global_running = false;
    }
    switch (message.message)
    {
      case WM_QUIT:
      {
        global_running = false;
      }
      break;

      case WM_MOUSEWHEEL:
      {
        mouse_input->mouse_z += GET_WHEEL_DELTA_WPARAM(message.wParam);
      }
      break;
      case WM_KEYDOWN:
      case WM_KEYUP:
      case WM_SYSKEYDOWN:
      case WM_SYSKEYUP:
      {
        S32 VK_code = (S32)message.wParam;
        B32 was_down = ((message.lParam & (1u << 30)) != 0);
        B32 is_down = ((message.lParam & (1u << 31)) == 0);
        B32 alt_key_mod = ((message.lParam & (1u << 29)) != 0);
        B32 shift_key_mod = GetKeyState(VK_LSHIFT) & (1 << 15);
        // B32 ctrl_key_mod = GetKeyState(VK_LCONTROL);

        if (was_down == is_down)
        {
          break;
        }
        // Resize window
        if (VK_code == 'M' && is_down)
        {
          RECT old;
          GetWindowRect(window, &old);
          RECT rect;
          GetClientRect(window, &rect);

          rect.right = (2 * global_window_offset + WINDOW_WIDTH) * 2;
          rect.bottom = (2 * global_window_offset + WINDOW_HEIGHT) * 2;
          AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

          SetWindowPos(window, NULL, old.left, old.top, rect.right - rect.left, rect.bottom - rect.top,
                       SWP_NOZORDER | SWP_NOACTIVATE);
        }
        else if (VK_code == 'N' && is_down)
        {
          RECT old;
          GetWindowRect(window, &old);
          RECT rect;
          GetClientRect(window, &rect);
          rect.right = (2 * global_window_offset + WINDOW_WIDTH);
          rect.bottom = (2 * global_window_offset + WINDOW_HEIGHT);
          AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

          SetWindowPos(window, NULL, old.left, old.top, rect.right - rect.left, rect.bottom - rect.top,
                       SWP_NOZORDER | SWP_NOACTIVATE);
        }

        if (VK_code == VK_F1)
        {
          Win32_Process_Record_Playback_Message(state, keyboard_controller, is_down, shift_key_mod, 1);
        }
        else if (VK_code == VK_F2)
        {
          Win32_Process_Record_Playback_Message(state, keyboard_controller, is_down, shift_key_mod, 2);
        }
        else if (VK_code == VK_F3)
        {
          Win32_Process_Record_Playback_Message(state, keyboard_controller, is_down, shift_key_mod, 3);
        }
        else if (VK_code == VK_F4)
        {
          Win32_Process_Record_Playback_Message(state, keyboard_controller, is_down, shift_key_mod, 4);
        }
        else if (VK_code == VK_F5)
        {
          // NOTE: Purge all replays
          DEBUG_Win32_Delete_Recordings();
        }
        if ((VK_code == VK_F4) && alt_key_mod)
        {
          global_running = false;
        }
        // NOTE: Only allow pause if not recording
        if (!Win32_State_Is_Record(state))
        {
          if (is_down && VK_code == 'P')
          {
            global_pause = !global_pause;
          }
        }
        if (!Win32_State_Is_Playback(state))
        {
          // NOTE: Only allow input for controller if not playing back recording

          if (VK_code == 'W')
          {
            keyboard_controller->stick_left.y += is_down ? 1.0f : -1.0f;
          }
          else if (VK_code == 'S')
          {
            keyboard_controller->stick_left.y += is_down ? -1.0f : 1.0f;
          }
          else if (VK_code == 'D')
          {
            keyboard_controller->stick_left.x += is_down ? 1.0f : -1.0f;
          }
          else if (VK_code == 'A')
          {
            keyboard_controller->stick_left.x += is_down ? -1.0f : 1.0f;
          }
          // TODO: Normalize the vector sticks left and right

          if (VK_code == '1')
          {
            Win32_Process_Keyboard_Message(&keyboard_controller->dpad_left, is_down);
          }
          else if (VK_code == '2')
          {
            Win32_Process_Keyboard_Message(&keyboard_controller->dpad_down, is_down);
          }
          else if (VK_code == '3')
          {
            Win32_Process_Keyboard_Message(&keyboard_controller->dpad_up, is_down);
          }
          else if (VK_code == '4')
          {
            Win32_Process_Keyboard_Message(&keyboard_controller->dpad_right, is_down);
          }
          else if (VK_code == 'Q')
          {
            Win32_Process_Keyboard_Message(&keyboard_controller->L1, is_down);
          }

          else if (VK_code == 'E')
          {
            Win32_Process_Keyboard_Message(&keyboard_controller->R1, is_down);
          }
          else if (VK_code == VK_UP)
          {
            Win32_Process_Keyboard_Message(&keyboard_controller->action_up, is_down);
          }
          else if (VK_code == VK_LEFT)
          {
            Win32_Process_Keyboard_Message(&keyboard_controller->action_left, is_down);
          }
          else if (VK_code == VK_DOWN)
          {
            Win32_Process_Keyboard_Message(&keyboard_controller->action_down, is_down);
          }
          else if (VK_code == VK_RIGHT)
          {
            Win32_Process_Keyboard_Message(&keyboard_controller->action_right, is_down);
          }
          else if (VK_code == VK_SPACE)
          {
            Win32_Process_Keyboard_Message(&keyboard_controller->start, is_down);
          }
          else if (VK_code == VK_ESCAPE)
          {
            Win32_Process_Keyboard_Message(&keyboard_controller->back, is_down);
          }
        }
      }
      break;
      default:
      {
        TranslateMessage(&message);
        DispatchMessage(&message);
      }
      break;
    }
  }
}
internal void Win32_Process_XInput_Buttons(DWORD xinput_button_state, Game_Button_State* old_state, DWORD button_bit,
                                           Game_Button_State* new_state)
{
  new_state->ended_down = ((xinput_button_state & button_bit) == button_bit);
  new_state->half_transition_count = (old_state->ended_down != new_state->ended_down) ? 1 : 0;
}

inline internal LARGE_INTEGER Win32_Get_Wall_Clock(void)
{
  LARGE_INTEGER result;
  QueryPerformanceCounter(&result);
  return result;
}

inline internal F32 Win32_Get_Seconds_Elapsed(LARGE_INTEGER start, LARGE_INTEGER end)
{
  F32 result = ((F32)(end.QuadPart - start.QuadPart)) / ((F32)global_perf_count_frequency);
  return result;
}

internal void Win32_Sleepms(F32 ms)
{
  HANDLE timer = CreateWaitableTimer(NULL, true, NULL);
  if (!timer)
  {
    return;
  }

  LARGE_INTEGER sleep_amount;
  // 100 ns units
  sleep_amount.QuadPart = -(S64)(ms * 10000.f); // negatiive = relative

  if (SetWaitableTimer(timer, &sleep_amount, 0, NULL, NULL, 0))

  {
    WaitForSingleObject(timer, INFINITE);
  }

  CloseHandle(timer);
}

internal void Win32_Debug_Draw_Vertical(Win32_Offscreen_Buffer* back_buffer, S32 x, S32 top, S32 bottom, U32 color)
{
  U8* pixel = (U8*)back_buffer->memory + x * back_buffer->bytes_per_pixel + top * back_buffer->pitch;
  for (S32 y = top; y < bottom; y++)
  {
    *(U32*)(void*)pixel = color;
    pixel += back_buffer->pitch;
  }
}
#if 0
internal void Win32_Debug_Sync_Display(Win32_Offscreen_Buffer* back_buffer, S32* last_play_cursor,
                                       S32 last_play_cursor_count, S32 debug_last_play_cursor_index)
{
  S32 pad_y = 16;
  S32 pad_x = 16;

  S32 top = pad_y;
  S32 bottom = back_buffer->height - pad_y;
  S32 x = pad_x;
  F32 c = (F32)(global_back_buffer.width - 2 * pad_x) / (F32)96000.0f;

  // for (S32 play_cursor_index = debug_last_play_cursor_index + 1;
  //      play_cursor_index < last_play_cursor_count; play_cursor_index++)
  // {
  //   x += (S32)(c * (F32)last_play_cursor[play_cursor_index]);
  //   ASSERT(x < back_buffer->width);
  //   Win32_Debug_Draw_Vertical(back_buffer, x, top, bottom, 0xFFFFFFFF);
  // }

  S32 max = 0;
  S32 min = 2000000;
  for (S32 i = 0; i < last_play_cursor_count; i++)
  {
    if (last_play_cursor[i] > max)
    {
      max = last_play_cursor[i];
    }
    if (last_play_cursor[i] < min)
    {
      min = last_play_cursor[i];
    }
  }

  for (S32 play_cursor_index = 0; play_cursor_index < debug_last_play_cursor_index; play_cursor_index++)
  {
    x += (S32)(c * (F32)last_play_cursor[play_cursor_index]);
    if (x < back_buffer->width)
    {
      if (last_play_cursor[play_cursor_index] == max)
      {
        Win32_Debug_Draw_Vertical(back_buffer, x, top, bottom, 0x00000000);
      }
      else if (last_play_cursor[play_cursor_index] == min)
      {
        Win32_Debug_Draw_Vertical(back_buffer, x, top, bottom, 0x00FF0000);
      }
      else
      {
        Win32_Debug_Draw_Vertical(back_buffer, x, top, bottom, 0xFFFFFFFF);
      }
    }
  }
}
#endif
internal void Win32_Get_Directories(Win32_State* state)
{
  S32 path_length = WIN32_STATE_FILE_NAME_COUNT;
  char exe_file_path[WIN32_STATE_FILE_NAME_COUNT];
  GetModuleFileNameA(0, exe_file_path, sizeof(exe_file_path));
  char* one_past_last_slash = exe_file_path;

  char* found_slash = exe_file_path;
  while ((found_slash = cstring_find_substr(one_past_last_slash, "\\")) != 0)
  {
    one_past_last_slash = found_slash + 1;
  }

  cstring_append(state->build_directory, path_length, exe_file_path, (S32)(one_past_last_slash - exe_file_path));

  char* cengine_path = cstring_find_substr(exe_file_path, "c-engine") + sizeof("c-engine");

  char* directory = "data\\";
  cstring_cat(state->data_directory, path_length, exe_file_path, (S32)(cengine_path - exe_file_path), directory,
              cstring_len(directory));
}

//**********************************************************************************
//*
//*
//**********************************************************************************
int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
  Win32_State state = {};

  LARGE_INTEGER perf_count_frequency_result;
  QueryPerformanceFrequency(&perf_count_frequency_result);
  global_perf_count_frequency = (U64)perf_count_frequency_result.QuadPart;

  Win32_Get_Directories(&state);

  char source_dll_name[WIN32_STATE_FILE_NAME_COUNT];
  Win32_Prepend_Build_Path(&state, source_dll_name, WIN32_STATE_FILE_NAME_COUNT, "cengine.dll");

  char temp_dll_name[WIN32_STATE_FILE_NAME_COUNT];
  Win32_Prepend_Build_Path(&state, temp_dll_name, WIN32_STATE_FILE_NAME_COUNT, "cengine-temp.dll");

  HRESULT hr = CoInitialize(0);
  if (FAILED(hr))
  {
    printf("CoInitialize failed\n");
  }

  Win32_Resize_DIB_Section(&global_back_buffer, WINDOW_WIDTH, WINDOW_HEIGHT);

  WNDCLASSEXW window_class = {
      .cbSize = sizeof(WNDCLASSEX),
      .style = CS_HREDRAW | CS_VREDRAW,
      .lpfnWndProc = Win32_Wnd_Proc,
      .hInstance = hInstance,
      //.hIcon
      .lpszClassName = L"CengineWindowClass",
      .hCursor = LoadCursor(0, IDC_ARROW),
      .hIcon = LoadIcon(0, IDI_APPLICATION),
  };

  U64 last_cycle_count = __rdtsc();
  if (RegisterClassExW(&window_class))
  {

    RECT rect = {0, 0, (2 * global_window_offset) + WINDOW_WIDTH, (2 * global_window_offset) + WINDOW_HEIGHT};

    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
    S32 adjusted_window_width = rect.right - rect.left;
    S32 adjusted_window_height = rect.bottom - rect.top;

    HWND window =
        CreateWindowExW(0, window_class.lpszClassName, L"cengine", WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT,
                        CW_USEDEFAULT, adjusted_window_width, adjusted_window_height, 0, 0, hInstance, 0);

    // printf("window: %p\n", (int *)window);
    // Window Message Loop
    if (window)
    {

      HDC device_context = GetDC(window);
      S32 refresh_rate = GetDeviceCaps(device_context, VREFRESH);
      ReleaseDC(window, device_context);

      S32 monitor_refresh_hz = refresh_rate;
      S32 game_update_hz = CLAMP(monitor_refresh_hz, 30, 30);
      global_target_seconds_per_frame = 1.0f / (F32)game_update_hz;

      Win32_Load_XInput();

      // INIT AUDIO ---------------------------
      Wasapi_Context wasapi_context = {0};
      Win32_WASAPI_Init(&wasapi_context);

      Game_Output_Sound_Buffer sound_buffer = {};
      U64 buffer_size = wasapi_context.buffer_frame_count * wasapi_context.wave_format->nBlockAlign;

      sound_buffer.sample_buffer = (U8*)VirtualAlloc(0, buffer_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

      Win32_Setup_Sound_Buffer(&wasapi_context, &sound_buffer);

#ifdef CENGINE_INTERNAL
      LPVOID base_address = (LPVOID)Terabytes(2);
#else
      LPVOID base_address = 0;
#endif

      Game_Memory game_memory = {};
      game_memory.permanent_storage_size = Megabytes(64);
      game_memory.transient_storage_size = Gigabytes(1);
      game_memory.DEBUG_Platform_Free_File_Memory = DEBUG_Platform_Free_File_Memory;
      game_memory.DEBUG_Platform_Read_Entire_File = DEBUG_Platform_Read_Entire_File;
      game_memory.DEBUG_Platform_Write_Entire_File = DEBUG_Platform_Write_Entire_File;

      state.total_size = game_memory.permanent_storage_size + game_memory.transient_storage_size;
      state.game_memory_block = VirtualAlloc(base_address, state.total_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
      game_memory.permanent_storage = state.game_memory_block;
      game_memory.transient_storage = ((U8*)game_memory.permanent_storage + game_memory.permanent_storage_size);

      if (!sound_buffer.sample_buffer || !game_memory.permanent_storage || !game_memory.transient_storage)
      {
        return 1;
      }

      Game_Input input[2] = {};
      Game_Input* new_input = &input[0];
      Game_Input* old_input = &input[1];

      LARGE_INTEGER last_counter = Win32_Get_Wall_Clock();

      S32 debug_last_play_cursor_index = 0;
      S32 debug_last_play_cursor[30] = {0};
      S32 hiccups = 0;
      S32 frames = 0;

      Win32_Engine_Code engine = Win32_Load_Engine_Code(source_dll_name, temp_dll_name);
      Thread_Context thread = {};

      global_running = true;
      while (global_running)
      {
        new_input->delta_time_s = global_target_seconds_per_frame;

        FILETIME new_dll_write_time = Win32_Get_Last_Write_Time(source_dll_name);
        if (0 != CompareFileTime(&new_dll_write_time, &engine.dll_last_write_time))
        {
          Win32_Unload_Engine_Code(&engine);
          engine = Win32_Load_Engine_Code(source_dll_name, temp_dll_name);
        }
        Game_Controller_Input* new_keyboard_controller = GetController(new_input, 0);
        Game_Controller_Input* old_keyboard_controller = GetController(old_input, 0);

        // NOTE: Cleanup/Setup input before next processing
        MEM_ZERO(*new_keyboard_controller);
        for (U64 i = 0; i < Array_Count(new_keyboard_controller->buttons); i++)
        {
          new_keyboard_controller->buttons[i].ended_down = old_keyboard_controller->buttons[i].ended_down;
        }
        new_keyboard_controller->stick_left = old_keyboard_controller->stick_left;
        new_keyboard_controller->stick_right = old_keyboard_controller->stick_right;
        new_input->mouse_z = 0;

        Win32_Process_Pending_Messages(window, &state, new_input, new_keyboard_controller);

        if (!global_pause)
        {

          POINT mouse_p;
          GetCursorPos(&mouse_p);
          ScreenToClient(window, &mouse_p);

          Win32_Window_Dimension dim = Win32_Get_Window_Dimension(window);

          S32 scale = Win32_Get_Window_Scale(&global_back_buffer, dim.width, dim.height);
          new_input->mouse_x = (mouse_p.x / scale) - (global_window_offset / scale);
          new_input->mouse_y = (mouse_p.y / scale) - (global_window_offset / scale);

          Win32_Process_Keyboard_Message(&new_input->mouse_buttons[0], GetKeyState(VK_LBUTTON) & (1 << 15));
          Win32_Process_Keyboard_Message(&new_input->mouse_buttons[1], GetKeyState(VK_MBUTTON) & (1 << 15));
          Win32_Process_Keyboard_Message(&new_input->mouse_buttons[2], GetKeyState(VK_RBUTTON) & (1 << 15));
          Win32_Process_Keyboard_Message(&new_input->mouse_buttons[3], GetKeyState(VK_XBUTTON1) & (1 << 15));
          Win32_Process_Keyboard_Message(&new_input->mouse_buttons[4], GetKeyState(VK_XBUTTON2) & (1 << 15));

          S32 max_controller_count = XUSER_MAX_COUNT;
          if (max_controller_count > (S32)Array_Count(new_input->controllers))
          {
            max_controller_count = (S32)Array_Count(new_input->controllers);
          }
          for (S32 controller_index = 0; controller_index < XUSER_MAX_COUNT; ++controller_index)
          {
            XINPUT_STATE input_state;
            if (XInputGetState((DWORD)controller_index, &input_state) == ERROR_SUCCESS)
            {
              // This controller is plugged in
              XINPUT_GAMEPAD* gamepad = &input_state.Gamepad;
              S32 gamepad_controller_index = controller_index + 1;
              Game_Controller_Input* old_controller = GetController(old_input, gamepad_controller_index);
              Game_Controller_Input* new_controller = GetController(new_input, gamepad_controller_index);

              Win32_Process_XInput_Buttons(gamepad->wButtons, &old_controller->action_down, XINPUT_GAMEPAD_A,
                                           &new_controller->action_down);
              Win32_Process_XInput_Buttons(gamepad->wButtons, &old_controller->action_right, XINPUT_GAMEPAD_B,
                                           &new_controller->action_right);
              Win32_Process_XInput_Buttons(gamepad->wButtons, &old_controller->action_left, XINPUT_GAMEPAD_X,
                                           &new_controller->action_left);
              Win32_Process_XInput_Buttons(gamepad->wButtons, &old_controller->action_up, XINPUT_GAMEPAD_Y,
                                           &new_controller->action_up);

              Win32_Process_XInput_Buttons(gamepad->wButtons, &old_controller->dpad_down, XINPUT_GAMEPAD_DPAD_DOWN,
                                           &new_controller->dpad_down);
              Win32_Process_XInput_Buttons(gamepad->wButtons, &old_controller->dpad_right, XINPUT_GAMEPAD_DPAD_RIGHT,
                                           &new_controller->dpad_right);
              Win32_Process_XInput_Buttons(gamepad->wButtons, &old_controller->dpad_left, XINPUT_GAMEPAD_DPAD_LEFT,
                                           &new_controller->dpad_left);
              Win32_Process_XInput_Buttons(gamepad->wButtons, &old_controller->dpad_up, XINPUT_GAMEPAD_DPAD_UP,
                                           &new_controller->dpad_up);

              Win32_Process_XInput_Buttons(gamepad->wButtons, &old_controller->R3, XINPUT_GAMEPAD_RIGHT_THUMB,
                                           &new_controller->R3);
              Win32_Process_XInput_Buttons(gamepad->wButtons, &old_controller->R1, XINPUT_GAMEPAD_RIGHT_SHOULDER,
                                           &new_controller->R1);
              if (gamepad->bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
              {
                Win32_Process_XInput_Buttons(1, &old_controller->R2, 1, &new_controller->R2);
              }

              Win32_Process_XInput_Buttons(gamepad->wButtons, &old_controller->L3, XINPUT_GAMEPAD_LEFT_THUMB,
                                           &new_controller->L3);
              Win32_Process_XInput_Buttons(gamepad->wButtons, &old_controller->L1, XINPUT_GAMEPAD_LEFT_SHOULDER,
                                           &new_controller->L1);
              if (gamepad->bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
              {
                Win32_Process_XInput_Buttons(1, &old_controller->L2, 1, &new_controller->L2);
              }

              Win32_Process_XInput_Buttons(gamepad->wButtons, &old_controller->start, XINPUT_GAMEPAD_START,
                                           &new_controller->start);
              Win32_Process_XInput_Buttons(gamepad->wButtons, &old_controller->back, XINPUT_GAMEPAD_BACK,
                                           &new_controller->back);

              // S16 stick_x = gamepad->sThumbLX;
              // S16 stick_y = gamepad->sThumbLY;
              Win32_Process_XInput_Stick(&new_controller->stick_left, gamepad->sThumbLX, gamepad->sThumbLY,
                                         XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
              Win32_Process_XInput_Stick(&new_controller->stick_right, gamepad->sThumbRX, gamepad->sThumbRY,
                                         XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
            }
            else
            {
              // The controller is not available
            }
          }

          // Update Game --------------------------
          Game_Offscreen_Buffer buffer = {.memory = global_back_buffer.memory,
                                          .height = global_back_buffer.height,
                                          .pitch_in_bytes = global_back_buffer.pitch,
                                          .width = global_back_buffer.width,
                                          .bytes_per_pixel = global_back_buffer.bytes_per_pixel};
          // Win32_Query_Sample_Count(&wasapi_context, &sound_buffer);
          //
          if (state.input_recording_index)
          {
            Win32_Record_Input(&state, new_input);
          }
          if (state.input_playing_index)
          {
            Win32_Playback_Input(&state, new_input);
          }
          if (engine.Update_And_Render)
          {
            engine.Update_And_Render(&thread, &game_memory, new_input, &buffer);
          }
        }
        if (engine.Get_Sound_Samples)
        {
          // NOTE: Keep this before sleep so its more consistent
          // sleep length is inconsistent and will mess timing
          engine.Get_Sound_Samples(&thread, &game_memory, &sound_buffer);
        }
        //---------------------------------
        U64 sleep_start = __rdtsc();

        frames++;

        LARGE_INTEGER work_counter = Win32_Get_Wall_Clock();
        F32 work_seconds_elapsed = Win32_Get_Seconds_Elapsed(last_counter, work_counter);

        F32 seconds_elapsed_for_frame = work_seconds_elapsed;
        if (seconds_elapsed_for_frame < global_target_seconds_per_frame)
        {
          F32 sleep_ms =
              (1000.0f * (global_target_seconds_per_frame - seconds_elapsed_for_frame)) - 0.4f; // dont sleep too long
          if (sleep_ms > 0.f)
          {
            Win32_Sleepms(sleep_ms);
          }
          F32 test_seconds_elapsed = Win32_Get_Seconds_Elapsed(last_counter, Win32_Get_Wall_Clock());
          if (test_seconds_elapsed > (global_target_seconds_per_frame))
          {
            // TODO: Log overslept
            hiccups++;
          }

          while (seconds_elapsed_for_frame < global_target_seconds_per_frame)
          {
            seconds_elapsed_for_frame = Win32_Get_Seconds_Elapsed(last_counter, Win32_Get_Wall_Clock());
          }
        }
        else
        {
          // TODO: MISSED FRAME RATE !!!!!!!!!
          // TODO: Log
        }
        U64 sleep_cycles = __rdtsc() - sleep_start;

        LARGE_INTEGER end_counter = Win32_Get_Wall_Clock();
        F32 ms_per_frame = 1000.0f * Win32_Get_Seconds_Elapsed(last_counter, end_counter);
        last_counter = end_counter;

        Win32_Window_Dimension dim = Win32_Get_Window_Dimension(window);
        Win32_Output_Sound(&wasapi_context, &sound_buffer);

        device_context = GetDC(window);

        Win32_Display_Buffer_In_Window(&global_back_buffer, device_context, dim.width, dim.height);

        ReleaseDC(window, device_context);

#if CENGINE_INTERNAL
        S32 cursor = (S32)wasapi_context.buffer_frame_count - sound_buffer.sample_count;
        debug_last_play_cursor[debug_last_play_cursor_index++] = cursor;
        if (debug_last_play_cursor_index >= (S32)Array_Count(debug_last_play_cursor))
        {
          debug_last_play_cursor_index = 0;
        }
#endif

        Game_Input* temp = new_input;
        new_input = old_input;
        old_input = temp;

        U64 end_cycle_count = __rdtsc();
        U64 cycles_elapsed = end_cycle_count - last_cycle_count;
        U64 cycles_unslept = cycles_elapsed - sleep_cycles;
        F32 fps = 1000.f / ms_per_frame;
        F32 mcpf = (F32)cycles_elapsed / (1000 * 1000);
        F32 mcpf2 = (F32)cycles_unslept / (1000 * 1000);

        if (1)
        {
          printf("ms/f: %.2f, f/s: %.2f, mcpf: %.2f, mcpf(unslept): %.2f, hiccups: %d, hiccups%%: %.2f \n",
                 ms_per_frame, fps, mcpf, mcpf2, hiccups, 100.f * ((F32)hiccups / (F32)frames));
        }
        last_cycle_count = end_cycle_count;
      }
    }
    else
    {
      // TODO:pile: Logging
    }
  }
  else
  {
    // TODO:pile: Logging
  }

  return 0;
};
