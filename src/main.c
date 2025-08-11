// clang-format on
#include "base/base_core.h"
// #include "base/base_math.h"
#include <stdbool.h>
#include <windows.h>

typedef struct os_w32_offscreen_buffer os_w32_offscreen_buffer;
struct os_w32_offscreen_buffer
{
  BITMAPINFO info;
  void *memory;
  S32 width;
  S32 height;
  S32 pitch;
};

global bool global_running;
global os_w32_offscreen_buffer global_back_buffer;

typedef struct os_w32_window_dimension os_w32_window_dimension;
struct os_w32_window_dimension
{
  S32 width;
  S32 height;
};

internal os_w32_window_dimension OS_W32_Get_Window_Dimension(HWND window)
{
  os_w32_window_dimension result;

  RECT client_rect;
  GetClientRect(window, &client_rect);
  result.width = client_rect.right - client_rect.left;
  result.height = client_rect.bottom - client_rect.top;

  return result;
}

internal void Render_Weird_Gradient(os_w32_offscreen_buffer buffer, S32 x_offset, S32 y_offset)
{

  U32 *row = (U32 *)buffer.memory;
  for (int y = 0; y < buffer.height; ++y)
  {
    U32 *pixel = row;
    for (int x = 0; x < buffer.width; ++x)

    {
      // NOTE: Color      0x  AA RR GG BB
      //       Steel blue 0x  00 46 82 B4
      U32 blue = (U32)(x + x_offset);
      U32 green = (U32)(y + y_offset);

      *pixel++ = (green << 8 | blue << 8) | blue | green;
    }
    // NOTE:because row is U32 we move 4 bytes * width
    row += buffer.width;
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

internal void OS_W32_Display_Buffer_In_Window(HDC device_context, os_w32_offscreen_buffer buffer,
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
  StretchDIBits(device_context, 0, 0, window_width, window_height, 0, 0, buffer.width,
                buffer.height, buffer.memory, &buffer.info, DIB_RGB_COLORS, SRCCOPY);
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
    case WM_PAINT:
    {
      PAINTSTRUCT paint;
      HDC device_context = BeginPaint(window, &paint);
      os_w32_window_dimension dim = OS_W32_Get_Window_Dimension(window);
      OS_W32_Display_Buffer_In_Window(device_context, global_back_buffer, dim.width, dim.height);
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

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
  (void)hPrevInstance;
  (void)lpCmdLine;
  (void)nCmdShow;

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

        Render_Weird_Gradient(global_back_buffer, x_offset++, y_offset++);
        os_w32_window_dimension dim = OS_W32_Get_Window_Dimension(window);
        OS_W32_Display_Buffer_In_Window(device_context, global_back_buffer, dim.width, dim.height);
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
