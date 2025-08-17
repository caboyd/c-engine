
#include <fileapi.h>
#include <handleapi.h>
#include <memoryapi.h>
#include <winnt.h>
#define WIN32_LEAN_AND_MEAN
#include <stdbool.h>
#include <stdio.h>

#include "base/base_core.h"

#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <xinput.h>

#include "win32_cengine.h"
#include "cengine.h"

//----------------c files ---------------------------------
#include "cengine.c"

//----------------Globals----------------------
global bool global_running;
global win32_offscreen_buffer global_back_buffer;

//---------------------------------------------
internal Debug_Read_File_Result DEBUG_Platform_Read_Entire_File(char *filename)
{
  Debug_Read_File_Result result = {};
  HANDLE file_handle =
      CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);
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
        if (ReadFile(file_handle, result.contents, file_size_32, &bytes_read, 0) &&
            (file_size_32 == bytes_read))
        {
          result.contents_size = file_size_32;
        }
        else
        {
          DEBUG_Plaftorm_Free_File_Memory(result.contents);
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
internal void DEBUG_Plaftorm_Free_File_Memory(void *memory)
{
  if (memory)
  {
    VirtualFree(memory, 0, MEM_RELEASE);
  }
}
internal B32 DEBUG_Platform_Write_Entire_File(char *filename, U32 memory_size, void *memory)
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

internal void Win32_WASAPI_Cleanup(Wasapi_Context *ctx)
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

internal Wasapi_Status Win32_WASAPI_Init(Wasapi_Context *ctx)
{
  Wasapi_Status status = WASAPI_OK;
  HRESULT hr;
  if (status == WASAPI_OK)
  {
    hr = CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, &IID_IMMDeviceEnumerator,
                          (void **)&(ctx->device_enumerator));
    if (FAILED(hr))
    {
      status = WASAPI_ERR_DEVICE_ENUM;
    }
  }
  if (status == WASAPI_OK)
  {
    hr = ctx->device_enumerator->lpVtbl->GetDefaultAudioEndpoint(ctx->device_enumerator, eRender,
                                                                 eConsole, &(ctx->device));
    if (FAILED(hr))
    {
      status = WASAPI_ERR_AUDIO_ENDPOINT;
    }
  }

  if (status == WASAPI_OK)
  {
    hr = ctx->device->lpVtbl->Activate(ctx->device, &IID_IAudioClient, CLSCTX_ALL, NULL,
                                       (void **)&(ctx->audio_client));
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
      WAVEFORMATEXTENSIBLE *wf_ext = (WAVEFORMATEXTENSIBLE *)ctx->wave_format;
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
    // Get shortest buffer duration possible
    // 26667 = 2.6667 ms is the devicie perioud
    hr = ctx->audio_client->lpVtbl->GetDevicePeriod(ctx->audio_client, 0, &(buffer_duration));

    // min appears to be 22ms allowed
    S32 min_buffer_duration = 220000;
    if (buffer_duration < min_buffer_duration)
    {
#ifdef DEBUG
      // More forgiving buffer in debug builds with lower fps
      // 44ms at the moment
      min_buffer_duration *= 4;
#endif
      buffer_duration = min_buffer_duration;
    }

    hr = ctx->audio_client->lpVtbl->Initialize(ctx->audio_client, AUDCLNT_SHAREMODE_SHARED, 0,
                                               buffer_duration, 0, ctx->wave_format, NULL);
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
                                               (void **)&(ctx->render_client));
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

internal void Win32_Setup_Sound_Buffer(Wasapi_Context *ctx, Game_Output_Sound_Buffer *sound_buffer)
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
  sound_buffer->samples_per_second = (S32)ctx->wave_format->nSamplesPerSec;
}

internal void Win32_Query_Sample_Count(Wasapi_Context *ctx, Game_Output_Sound_Buffer *sound_buffer)
{
  U32 padding;
  HRESULT hr = ctx->audio_client->lpVtbl->GetCurrentPadding(ctx->audio_client, &padding);
  if (FAILED(hr))
  {
    // TODO: Logging
  }
  U32 frames_available = ctx->buffer_frame_count - padding;
  sound_buffer->sample_count = (S32)frames_available;
}

internal void Win32_Output_Sound(Wasapi_Context *ctx, Game_Output_Sound_Buffer *sound_buffer)
{

  U32 frames_available = (U32)sound_buffer->sample_count;
  HRESULT hr = ctx->render_client->lpVtbl->GetBuffer(ctx->render_client, frames_available,
                                                     &ctx->sample_buffer);
  if (FAILED(hr))
  {
    // TODO: Logging
  }

  S32 channels = sound_buffer->channel_count;
  S32 sample_format_bytes = Sample_Format_Bytes(sound_buffer->sample_format);
  S32 bits_per_sample = sample_format_bytes * 8;
  // Copy sound_buffer->sample_buffer to ctx->sample_buffer
  for (S32 frame = 0; frame < sound_buffer->sample_count; ++frame)
  {
    S32 per_frame_offset = frame * channels * sample_format_bytes;

    U8 *src_ptr = sound_buffer->sample_buffer + per_frame_offset;
    U8 *dest_ptr = ctx->sample_buffer + per_frame_offset;
    for (S32 ch = 0; ch < channels; ++ch)
    {
      if (bits_per_sample == 32)
      {
        *((F32 *)(void *)dest_ptr) = *((F32 *)(void *)src_ptr);
        // FIXME:temp trick to make less harsh sound when closing window
        // still makes click sound though
        if (global_running == false)
        {
          *((F32 *)(void *)dest_ptr) = 0;
        };
        dest_ptr += sizeof(F32);
        src_ptr += sizeof(F32);
      }
      else if (bits_per_sample == 16)
      {
        *((S16 *)(void *)dest_ptr) = *((S16 *)(void *)src_ptr);
        dest_ptr += sizeof(S16);
        src_ptr += sizeof(S16);
      }
      else if (bits_per_sample == 24)
      {
        *((S8 *)dest_ptr++) = *((S8 *)src_ptr++);
        *((S8 *)dest_ptr++) = *((S8 *)src_ptr++);
        *((S8 *)dest_ptr++) = *((S8 *)src_ptr++);
      }
    }
  }

  hr = ctx->render_client->lpVtbl->ReleaseBuffer(ctx->render_client,
                                                 (U32)sound_buffer->sample_count, 0);
  if (FAILED(hr))
  {
    // TODO: Logging
  }
}

//-----------------X Input------------------------

#define X_INPUT_GET_STATE(name)                                                                    \
  DWORD WINAPI name(DWORD user_index ATTRIBUTE_UNUSED, XINPUT_STATE *state ATTRIBUTE_UNUSED)
typedef X_INPUT_GET_STATE(X_Input_Get_State);
X_INPUT_GET_STATE(X_Input_Get_State_Stub)
{
  return ERROR_DEVICE_NOT_CONNECTED;
}

#define X_INPUT_SET_STATE(name)                                                                    \
  DWORD WINAPI name(DWORD user_index ATTRIBUTE_UNUSED, XINPUT_VIBRATION *vibration ATTRIBUTE_UNUSED)
typedef X_INPUT_SET_STATE(X_Input_Set_State);
X_INPUT_SET_STATE(X_Input_Set_State_Stub)
{
  return ERROR_DEVICE_NOT_CONNECTED;
}

global X_Input_Get_State *XInputGetState_ = X_Input_Get_State_Stub;
#define XInputGetState XInputGetState_

global X_Input_Set_State *XInputSetState_ = X_Input_Set_State_Stub;
#define XInputSetState XInputSetState_

internal void Win32_Load_XInput(void)
{
  HMODULE xinput_lib = LoadLibraryA("xinput1_4.dll");
  if (!xinput_lib)
  {
    xinput_lib = LoadLibraryA("xinput1_3.dll");
  }
  if (xinput_lib)
  {
    XInputGetState = (X_Input_Get_State *)(void *)GetProcAddress(xinput_lib, "XInputGetState");
    XInputSetState = (X_Input_Set_State *)(void *)GetProcAddress(xinput_lib, "XInputSetState");
  }
}

//--------Definitions----------------------

internal win32_window_dimension Win32_Get_Window_Dimension(HWND window)
{
  win32_window_dimension result;

  RECT client_rect;
  GetClientRect(window, &client_rect);
  result.width = client_rect.right - client_rect.left;
  result.height = client_rect.bottom - client_rect.top;

  return result;
}

internal void Win32_Resize_DIB_Section(win32_offscreen_buffer *buffer, int width, int height)
{
  if (buffer->memory)
  {
    VirtualFree(buffer->memory, 0, MEM_RELEASE);
  }

  buffer->width = width;
  buffer->height = height;

  memset(&buffer->info, 0, sizeof(buffer->info));
  buffer->info.bmiHeader = (BITMAPINFOHEADER){.biSize = sizeof(BITMAPINFOHEADER),
                                              .biWidth = buffer->width,
                                              .biHeight = -buffer->height,
                                              .biPlanes = 1,
                                              .biBitCount = 32,
                                              .biCompression = BI_RGB};

  S32 bytes_per_pixel = 4;
  S32 bitmap_memory_size = bytes_per_pixel * (buffer->width * buffer->height);
  buffer->memory =
      VirtualAlloc(0, (SIZE_T)bitmap_memory_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
  buffer->pitch = width * bytes_per_pixel;
}

internal void Win32_Display_Buffer_In_Window(win32_offscreen_buffer *buffer, HDC device_context,
                                             S32 window_width, S32 window_height)
{
  // StretchDIBits(
  //   device_context,
  //   x, y, width, height,
  //   x, y, width, height,
  //   &bitmap_memory,
  //   &bitmap_info,
  //   DIB_RGB_COLORS, SRCCOPY);

  // TODO: Aspcet ratio correction
  // TODO: play with stretch modes
  StretchDIBits(device_context, 0, 0, window_width, window_height, 0, 0, buffer->width,
                buffer->height, buffer->memory, &buffer->info, DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK Win32_Wnd_Proc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
  LRESULT result = 0;

  switch (message)
  {
    case WM_SIZE:
    {
      OutputDebugStringA("WM_SIZE\n");
    }
    break;
    case WM_DESTROY:
    {
      // TODO: handle this as an error - recreate window?
      global_running = false;
    }
    break;
    case WM_CLOSE:
    {
      // TODO:handle this with a message to the user?
      global_running = false;
    }
    break;
    case WM_ACTIVATEAPP:
    {
      OutputDebugStringA("WM_ACTIVATEAPP\n");
    }
    break;
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_KEYDOWN:
    case WM_KEYUP:
    {
      ASSERT(!"Keyboard input came in through non dispatch method");
    }
    break;
    case WM_PAINT:
    {
      PAINTSTRUCT paint;
      HDC device_context = BeginPaint(window, &paint);
      win32_window_dimension dim = Win32_Get_Window_Dimension(window);
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

internal void Win32_Process_XInput_Stick(Vec2 *stick, SHORT stick_axis_x, SHORT stick_axis_y,
                                         SHORT DEAD_ZONE)

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

internal void Win32_Process_Keyboard_Message(Game_Button_State *new_state, B32 is_down)
{
  ASSERT(new_state->ended_down != is_down);
  new_state->ended_down = is_down;
  new_state->half_transition_count += 1;
}
internal void Win32_Process_Pending_Messages(Game_Controller_Input *keyboard_controller)
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
      case WM_KEYDOWN:
      case WM_KEYUP:
      case WM_SYSKEYDOWN:
      case WM_SYSKEYUP:
      {
        S32 VK_code = (S32)message.wParam;
        bool was_down = ((message.lParam & (1u << 30)) != 0);
        bool is_down = ((message.lParam & (1u << 31)) == 0);

        if (was_down == is_down)
        {
          break;
        }

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
        //TODO: Normalize the vector sticks left and right

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
        B32 alt_key_mod = (message.lParam & (1 << 29));
        if ((VK_code == VK_F4) && alt_key_mod)
        {
          global_running = false;
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
internal void Win32_Process_XInput_Buttons(DWORD xinput_button_state, Game_Button_State *old_state,
                                           DWORD button_bit, Game_Button_State *new_state)
{
  new_state->ended_down = ((xinput_button_state & button_bit) == button_bit);
  new_state->half_transition_count = (old_state->ended_down != new_state->ended_down) ? 1 : 0;
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance ATTRIBUTE_UNUSED,
                     LPSTR lpCmdLine ATTRIBUTE_UNUSED, int nCmdShow ATTRIBUTE_UNUSED)
{

  HRESULT hr = CoInitialize(0);
  if (FAILED(hr))
  {
    printf("CoInitialize failed\n");
  }
  Win32_Load_XInput();

  // INIT AUDIO ---------------------------
  Wasapi_Context wasapi_context = {0};
  Win32_WASAPI_Init(&wasapi_context);

  //--------------------------------------

  Win32_Resize_DIB_Section(&global_back_buffer, 1600, 900);
  Game_Offscreen_Buffer buffer = {.memory = global_back_buffer.memory,
                                  .height = global_back_buffer.height,
                                  .pitch = global_back_buffer.pitch,
                                  .width = global_back_buffer.width};

  WNDCLASSEXW window_class = {
      .cbSize = sizeof(WNDCLASSEX),
      .style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC,
      .lpfnWndProc = Win32_Wnd_Proc,
      .hInstance = hInstance,
      //.hIcon
      .lpszClassName = L"CengineWindowClass",
      .hCursor = LoadCursor(0, IDC_ARROW),
      .hIcon = LoadIcon(0, IDI_APPLICATION),
  };

  LARGE_INTEGER perf_count_freq_result;
  QueryPerformanceFrequency(&perf_count_freq_result);
  S64 perf_count_freq = perf_count_freq_result.QuadPart;
  U64 last_cycle_count = __rdtsc();
  if (RegisterClassExW(&window_class))
  {
    HWND window = CreateWindowExW(0, window_class.lpszClassName, L"c-engine",
                                  WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
                                  1600, 900, 0, 0, hInstance, 0);

    printf("window: %p\n", (int *)window);
    // Window Message Loop
    if (window)
    {
      HDC device_context = GetDC(window);
      Game_Output_Sound_Buffer sound_buffer = {};
      U64 buffer_size = wasapi_context.buffer_frame_count * wasapi_context.wave_format->nBlockAlign;

      sound_buffer.sample_buffer =
          (U8 *)VirtualAlloc(0, buffer_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

      Win32_Setup_Sound_Buffer(&wasapi_context, &sound_buffer);

#ifdef CENGINE_INTERNAL
      LPVOID base_address = (LPVOID)Terabytes(2);
#else
      LPVOID base_address = 0;
#endif

      Game_Memory game_memory = {};
      game_memory.permanent_storage_size = Megabytes(64);
      game_memory.transient_storage_size = Gigabytes(1);
      game_memory.permanent_storage = VirtualAlloc(
          base_address, game_memory.permanent_storage_size + game_memory.transient_storage_size,
          MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

      game_memory.transient_storage =
          ((U8 *)game_memory.permanent_storage + game_memory.permanent_storage_size);

      if (!sound_buffer.sample_buffer || !game_memory.permanent_storage ||
          !game_memory.transient_storage)
      {
        return 1;
      }

      Game_Input input[2] = {};
      Game_Input *new_input = &input[0];
      Game_Input *old_input = &input[1];

      LARGE_INTEGER last_counter;
      QueryPerformanceCounter(&last_counter);

      global_running = true;
      while (global_running)
      {
        Game_Controller_Input *new_keyboard_controller = GetController(new_input, 0);
        Game_Controller_Input *old_keyboard_controller = GetController(old_input, 0);
        MEM_ZERO(*new_keyboard_controller);
        for (U64 i = 0; i < Array_Count(new_keyboard_controller->buttons); i++)
        {

          new_keyboard_controller->buttons[i].ended_down =
              old_keyboard_controller->buttons[i].ended_down;
        }
        new_keyboard_controller->stick_left = old_keyboard_controller->stick_left;
        new_keyboard_controller->stick_right = old_keyboard_controller->stick_right;

        Win32_Process_Pending_Messages(new_keyboard_controller);
        S32 max_controller_count = XUSER_MAX_COUNT;
        if (max_controller_count > (S32)Array_Count(new_input->controllers))
        {
          max_controller_count = (S32)Array_Count(new_input->controllers);
        }
        for (S32 controller_index = 0; controller_index < XUSER_MAX_COUNT; ++controller_index)
        {
          XINPUT_STATE state;
          if (XInputGetState((DWORD)controller_index, &state) == ERROR_SUCCESS)
          {
            // This controller is plugged in
            XINPUT_GAMEPAD *gamepad = &state.Gamepad;
            S32 gamepad_controller_index = controller_index + 1;
            Game_Controller_Input *old_controller =
                GetController(old_input, gamepad_controller_index);
            Game_Controller_Input *new_controller =
                GetController(new_input, gamepad_controller_index);

            Win32_Process_XInput_Buttons(gamepad->wButtons, &old_controller->action_down,
                                         XINPUT_GAMEPAD_A, &new_controller->action_down);
            Win32_Process_XInput_Buttons(gamepad->wButtons, &old_controller->action_right,
                                         XINPUT_GAMEPAD_B, &new_controller->action_right);
            Win32_Process_XInput_Buttons(gamepad->wButtons, &old_controller->action_left,
                                         XINPUT_GAMEPAD_X, &new_controller->action_left);
            Win32_Process_XInput_Buttons(gamepad->wButtons, &old_controller->action_up,
                                         XINPUT_GAMEPAD_Y, &new_controller->action_up);

            Win32_Process_XInput_Buttons(gamepad->wButtons, &old_controller->dpad_down,
                                         XINPUT_GAMEPAD_DPAD_DOWN, &new_controller->dpad_down);
            Win32_Process_XInput_Buttons(gamepad->wButtons, &old_controller->dpad_right,
                                         XINPUT_GAMEPAD_DPAD_RIGHT, &new_controller->dpad_right);
            Win32_Process_XInput_Buttons(gamepad->wButtons, &old_controller->dpad_left,
                                         XINPUT_GAMEPAD_DPAD_LEFT, &new_controller->dpad_left);
            Win32_Process_XInput_Buttons(gamepad->wButtons, &old_controller->dpad_up,
                                         XINPUT_GAMEPAD_DPAD_UP, &new_controller->dpad_up);

            Win32_Process_XInput_Buttons(gamepad->wButtons, &old_controller->R3,
                                         XINPUT_GAMEPAD_RIGHT_THUMB, &new_controller->R3);
            Win32_Process_XInput_Buttons(gamepad->wButtons, &old_controller->R1,
                                         XINPUT_GAMEPAD_RIGHT_SHOULDER, &new_controller->R1);
            if (gamepad->bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
            {
              Win32_Process_XInput_Buttons(1, &old_controller->R2, 1, &new_controller->R2);
            }

            Win32_Process_XInput_Buttons(gamepad->wButtons, &old_controller->L3,
                                         XINPUT_GAMEPAD_LEFT_THUMB, &new_controller->L3);
            Win32_Process_XInput_Buttons(gamepad->wButtons, &old_controller->L1,
                                         XINPUT_GAMEPAD_LEFT_SHOULDER, &new_controller->L1);
            if (gamepad->bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
            {
              Win32_Process_XInput_Buttons(1, &old_controller->L2, 1, &new_controller->L2);
            }

            Win32_Process_XInput_Buttons(gamepad->wButtons, &old_controller->start,
                                         XINPUT_GAMEPAD_START, &new_controller->start);
            Win32_Process_XInput_Buttons(gamepad->wButtons, &old_controller->back,
                                         XINPUT_GAMEPAD_BACK, &new_controller->back);

            // S16 stick_x = gamepad->sThumbLX;
            // S16 stick_y = gamepad->sThumbLY;
            Win32_Process_XInput_Stick(&new_controller->stick_left, gamepad->sThumbLX,
                                       gamepad->sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
            Win32_Process_XInput_Stick(&new_controller->stick_right, gamepad->sThumbRX,
                                       gamepad->sThumbRY, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
          }
          else
          {
            // The controller is not available
          }
        }

        // Update Game --------------------------

        Win32_Query_Sample_Count(&wasapi_context, &sound_buffer);
        Game_Update_And_Render(&game_memory, new_input, &buffer, &sound_buffer);
        Win32_Output_Sound(&wasapi_context, &sound_buffer);

        win32_window_dimension dim = Win32_Get_Window_Dimension(window);
        Win32_Display_Buffer_In_Window(&global_back_buffer, device_context, dim.width, dim.height);

        //---------------------------------

        U64 end_cycle_count = __rdtsc();
        U64 cycle_elapsed = end_cycle_count - last_cycle_count;

        LARGE_INTEGER end_counter;
        QueryPerformanceCounter(&end_counter);
        S64 counter_elapsed = end_counter.QuadPart - last_counter.QuadPart;
        F32 ms_per_frame = (1000 * (F32)counter_elapsed) / (F32)perf_count_freq;
        F32 fps = ((F32)perf_count_freq / (F32)counter_elapsed);
        F32 mcpf = (F32)cycle_elapsed / (1000 * 1000);

        if (0)
        {
          printf("ms/f: %.2f, f/s: %.2f, mcpf: %.2f \n", ms_per_frame, fps, mcpf);
        }
        last_counter = end_counter;
        last_cycle_count = end_cycle_count;

        Game_Input *temp = new_input;
        new_input = old_input;
        old_input = temp;
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
