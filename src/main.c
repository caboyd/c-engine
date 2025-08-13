#include "base/base_core.h"
#include "base/base_math.h"
#include <stdbool.h>
#include <stdio.h>
#include <windows.h>
#include <xinput.h>
#include <audioclient.h>
#include <mmdeviceapi.h>
#include <math.h>
//--------Declarations--------------

typedef struct os_w32_offscreen_buffer os_w32_offscreen_buffer;
struct os_w32_offscreen_buffer
{
  BITMAPINFO info;
  void *memory;
  S32 width;
  S32 height;
  S32 pitch;
};

typedef struct os_w32_window_dimension os_w32_window_dimension;
struct os_w32_window_dimension
{
  S32 width;
  S32 height;
};

//----------------Globals----------------------
global bool global_running;
global os_w32_offscreen_buffer global_back_buffer;
global F32 global_volume = 0.04f;
global F32 global_frequency = 440.0;
//-----------------WASAPI ---------------------

const IID IID_IAudioClient = {
    0x1CB9AD4C, 0xDBFA, 0x4c32, {0xB1, 0x78, 0xC2, 0xF5, 0x68, 0xA7, 0x03, 0xB2}};
const GUID IID_IAudioRenderClient = {
    0xF294ACFC, 0x3146, 0x4483, {0xA7, 0xBF, 0xAD, 0xDC, 0xA7, 0xC2, 0x60, 0xE2}};
const GUID CLSID_MMDeviceEnumerator = {
    0xBCDE0395, 0xE52F, 0x467C, {0x8E, 0x3D, 0xC4, 0x57, 0x92, 0x91, 0x69, 0x2E}};
const GUID IID_IMMDeviceEnumerator = {
    0xA95664D2, 0x9614, 0x4F35, {0xA7, 0x46, 0xDE, 0x8D, 0xB6, 0x36, 0x17, 0xE6}};
const GUID PcmSubformatGuid = {
    0x00000001, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71}};
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
  IMMDeviceEnumerator *device_enumerator;
  IMMDevice *device;
  IAudioClient *audio_client;
  IAudioRenderClient *render_client;
  WAVEFORMATEX *wave_format;
  Wasapi_Sample_Format sample_format;
  U32 buffer_frame_count;
};

internal void OS_W32_WASAPI_Cleanup(Wasapi_Context *ctx)
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

internal Wasapi_Status OS_W32_WASAPI_Init(Wasapi_Context *ctx)
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
    REFERENCE_TIME buffer_duration;
    hr = ctx->audio_client->lpVtbl->GetDevicePeriod(ctx->audio_client, 0, &(buffer_duration));
    buffer_duration = 10000 * 1000; // 1 second

    hr = ctx->audio_client->lpVtbl->Initialize(ctx->audio_client, AUDCLNT_SHAREMODE_SHARED, 0,
                                               buffer_duration, 0, ctx->wave_format, NULL);
    if (FAILED(hr))
    {
      status = WASAPI_ERR_INIT;
    }
  }
  if (status == WASAPI_OK)
  {
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
    OS_W32_WASAPI_Cleanup(ctx);
  }
  return status;
}

internal void OS_W32_WASAPI_Generate_Tone(Wasapi_Context *ctx, F32 frequency, F32 volume)
{
  U32 padding;
  ctx->audio_client->lpVtbl->GetCurrentPadding(ctx->audio_client, &padding);
  U32 frames_available = (U32)ctx->buffer_frame_count - padding;
  if (frames_available == 0)
  {
    return;
  }
  U8 *buffer = NULL;
  HRESULT hr = ctx->render_client->lpVtbl->GetBuffer(ctx->render_client, frames_available, &buffer);
  if (FAILED(hr))
  {
    return;
  }

  U32 sample_rate = ctx->wave_format->nSamplesPerSec;
  U32 channels = ctx->wave_format->nChannels;
  U32 bits_per_sample = ctx->wave_format->wBitsPerSample;
  U32 block_align = ctx->wave_format->nBlockAlign;

  F64 phase_increment = 2.0 * M_PI * frequency / sample_rate;
  local_persist F64 sine_phase = 0.0;
  for (U32 frame = 0; frame < frames_available; ++frame)
  {
    float sample_value = (float)sin(sine_phase);
    sample_value *= volume;

    sine_phase += phase_increment;
    if (sine_phase > 2.0 * M_PI)
      sine_phase -= 2.0 * M_PI;

    U8 *frame_ptr = buffer + frame * block_align;

    for (U32 ch = 0; ch < channels; ++ch)
    {
      U8 *sample_ptr = frame_ptr + ch * (bits_per_sample / 8);
      switch (bits_per_sample)
      {
        case 16:
        {
          S16 sample = INT16_MAX * (S16)(sample_value);
          *((S16 *)(void *)sample_ptr) = sample;
        }
        break;
        case 24:
        {
          // TODO: Add logging unsupported
        }
        case 32:
        {
          if (ctx->sample_format == WASAPI_SAMPLE_FORMAT_PCM)
          {
            *((S32 *)(void *)sample_ptr) = INT32_MAX * (S32)(sample_value);
          }
          else if (ctx->sample_format == WASAPI_SAMPLE_FORMAT_FLOAT)
          {
            *((F32 *)(void *)sample_ptr) = sample_value;
          }
          else if (ctx->sample_format == WASAPI_SAMPLE_FORMAT_UNKNOWN)
          {
            // TODO add logging
          }
        }
        break;
      }
    }
  }

  ctx->render_client->lpVtbl->ReleaseBuffer(ctx->render_client, frames_available, 0);
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

internal void OS_W32_Load_XInput(void)
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

internal os_w32_window_dimension OS_W32_Get_Window_Dimension(HWND window)
{
  os_w32_window_dimension result;

  RECT client_rect;
  GetClientRect(window, &client_rect);
  result.width = client_rect.right - client_rect.left;
  result.height = client_rect.bottom - client_rect.top;

  return result;
}

internal void Render_Weird_Gradient(os_w32_offscreen_buffer *buffer, S32 x_offset, S32 y_offset)
{

  U32 *row = (U32 *)buffer->memory;
  for (int y = 0; y < buffer->height; ++y)
  {
    U32 *pixel = row;
    for (int x = 0; x < buffer->width; ++x)

    {
      // NOTE: Color      0x  AA RR GG BB
      //       Steel blue 0x  00 46 82 B4
      U32 blue = (U32)(x + x_offset);
      U32 green = (U32)(y + y_offset);

      *pixel++ = (green << 8 | blue << 8) | blue | green;
    }
    // NOTE:because row is U32 we move 4 bytes * width
    row += buffer->width;
  }
}

internal void OS_W32_Resize_DIB_Section(os_w32_offscreen_buffer *buffer, int width, int height)
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

internal void OS_W32_Display_Buffer_In_Window(os_w32_offscreen_buffer *buffer, HDC device_context,
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

LRESULT CALLBACK OS_W32_Wnd_Proc(HWND window, UINT message, WPARAM w_param, LPARAM l_param)
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
    {
      if (w_param == VK_F4)
      {
        result = DefWindowProc(window, message, w_param, l_param);
      }
    } // fallthrough;
    case WM_KEYDOWN:
    case WM_KEYUP:
    {
      S32 VK_code = (S32)w_param;
      bool was_down = ((l_param & (1u << 30)) != 0);
      bool is_down = ((l_param & (1u << 31)) == 0);
      if (was_down == is_down)
        break;
      if (VK_code == 'W')
      {
      }
      else if (VK_code == 'A')
      {
      }
      else if (VK_code == 'S')
      {
      }
      else if (VK_code == 'D')
      {
      }
      else if (VK_code == 'Q')
      {
      }
      else if (VK_code == 'E')
      {
      }
      else if (VK_code == VK_UP)
      {
      }
      else if (VK_code == VK_LEFT)
      {
      }
      else if (VK_code == VK_DOWN)
      {
      }
      else if (VK_code == VK_RIGHT)
      {
      }
      else if (VK_code == VK_F4)
      {
        OutputDebugString("ESCAPE: ");
        if (is_down)
        {
          OutputDebugString("is_down");
        }
        if (was_down)
        {
          OutputDebugString("was_down");
        }
        OutputDebugString("\n");
      }
      else if (VK_code == VK_SPACE)
      {
      }
      B32 alt_key_mod = (l_param & (1 << 29));
      if ((VK_code == VK_F4) && alt_key_mod)
      {
        global_running = false;
      }
    }
    break;
    case WM_PAINT:
    {
      PAINTSTRUCT paint;
      HDC device_context = BeginPaint(window, &paint);
      os_w32_window_dimension dim = OS_W32_Get_Window_Dimension(window);
      OS_W32_Display_Buffer_In_Window(&global_back_buffer, device_context, dim.width, dim.height);
      EndPaint(window, &paint);
    }

    break;
    default:
    {
      result = DefWindowProc(window, message, w_param, l_param);
    }
    break;
  }

  return result;
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance ATTRIBUTE_UNUSED,
                     LPSTR lpCmdLine ATTRIBUTE_UNUSED, int nCmdShow ATTRIBUTE_UNUSED)
{

  HRESULT hr = CoInitialize(0);
  if (FAILED(hr))
  {
    printf("CoInitialize failed\n");
  }
  OS_W32_Load_XInput();

  Wasapi_Context wasapi_context = {0};
  OS_W32_WASAPI_Init(&wasapi_context);
  OS_W32_Resize_DIB_Section(&global_back_buffer, 1600, 900);

  WNDCLASSEXW window_class = {
      .cbSize = sizeof(WNDCLASSEX),
      .style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC,
      .lpfnWndProc = OS_W32_Wnd_Proc,
      .hInstance = hInstance,
      //.hIcon
      .lpszClassName = L"CengineWindowClass",
      .hCursor = LoadCursor(0, IDC_ARROW),
      .hIcon = LoadIcon(0, IDI_APPLICATION),
  };

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
      S32 x_offset = 0;
      S32 y_offset = 0;
      global_running = true;
      while (global_running)
      {
        MSG message;
        while (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
        {
          if (message.message == WM_QUIT)
          {
            global_running = false;
          }

          TranslateMessage(&message);
          DispatchMessage(&message);
        }
        OS_W32_WASAPI_Generate_Tone(&wasapi_context, global_frequency, global_volume);
        for (DWORD controller_index = 0; controller_index < XUSER_MAX_COUNT; ++controller_index)
        {
          XINPUT_STATE state;
          if (XInputGetState(controller_index, &state) == ERROR_SUCCESS)
          {
            // This controller is plugged in
            XINPUT_GAMEPAD *gamepad = &state.Gamepad;

            // bool up = (gamepad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
            // bool down = (gamepad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
            // bool left = (gamepad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
            // bool right = (gamepad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
            //
            // bool start = (gamepad->wButtons & XINPUT_GAMEPAD_START);
            // bool back = (gamepad->wButtons & XINPUT_GAMEPAD_BACK);
            // bool left_shoulder = (gamepad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
            // bool right_shoulder = (gamepad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
            bool a_button = (gamepad->wButtons & XINPUT_GAMEPAD_A);
            bool b_button = (gamepad->wButtons & XINPUT_GAMEPAD_B);
            // bool x_button = (gamepad->wButtons & XINPUT_GAMEPAD_X);
            // bool y_button = (gamepad->wButtons & XINPUT_GAMEPAD_Y);

            // S16 stick_x = gamepad->sThumbLX;
            // S16 stick_y = gamepad->sThumbLY;

            XINPUT_VIBRATION vib = {};
            if (a_button)
            {
              y_offset += 2;
            }
            if (b_button)
            {
              vib.wLeftMotorSpeed = 30000;
              vib.wRightMotorSpeed = 30000;
              XInputSetState(0, &vib);
            }
            else
            {
              vib.wLeftMotorSpeed = 0;
              vib.wRightMotorSpeed = 0;
              XInputSetState(0, &vib);
            }
          }
          else
          {
            // The controller is not available
          }
        }
        Render_Weird_Gradient(&global_back_buffer, ++x_offset, y_offset);

        os_w32_window_dimension dim = OS_W32_Get_Window_Dimension(window);
        OS_W32_Display_Buffer_In_Window(&global_back_buffer, device_context, dim.width, dim.height);
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
