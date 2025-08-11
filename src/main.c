// clang-format off
#include "base/base_core.h"
#include "base/base_math.h"
#include <stdbool.h>
#include <windows.h>

global bool running;
global BITMAPINFO bitmap_info;
global void* bitmap_memory;
global S32 bitmap_width;
global S32 bitmap_height;

internal void render_weird_gradient(S32 x_offset, S32 y_offset)
{
  S32 width = bitmap_width;
  S32 height = bitmap_height;

  U32* row = (U32*)bitmap_memory;
  for(int y = 0; y < height; ++y)
  {
    U32* pixel = row;
    for(int x = 0; x < width; ++x)
    {
      //Color  0x  AA RR GG BB
      //Steel blue 0x004682B4
      U32 blue = (U32)(x + x_offset);
      U32 green = (U32)(y + y_offset);

      *pixel++ = (green << 8 | blue << 8) | blue | green;
    }

    row += width;
  }
}

internal void os_w32_resize_DIB_section(int width, int height) 
{
  if(bitmap_memory)
  {
    VirtualFree(bitmap_memory, 0, MEM_RELEASE);
  }
 
  bitmap_width = width;
  bitmap_height = height;
  memset(&bitmap_info, 0, sizeof(bitmap_info));
  bitmap_info.bmiHeader = (BITMAPINFOHEADER){
    .biSize = sizeof(BITMAPINFOHEADER),
    .biWidth = bitmap_width,
    .biHeight = -bitmap_height,
    .biPlanes = 1,
    .biBitCount = 32, 
    .biCompression = BI_RGB
  };
  
  S32 bytes_per_pixel = 4;
  S32 bitmap_memory_size = bytes_per_pixel * (bitmap_width * bitmap_height);
  bitmap_memory = VirtualAlloc(0, (SIZE_T)bitmap_memory_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);


 }

internal void os_w32_update_window(HDC device_context, RECT client_rect, int x, int y, int width, int height)
{
  S32 window_width = client_rect.right - client_rect.left;
  S32 window_height = client_rect.bottom - client_rect.top;
  // StretchDIBits(
  //   device_context,
  //   x, y, width, height, 
  //   x, y, width, height,
  //   &bitmap_memory, 
  //   &bitmap_info, 
  //   DIB_RGB_COLORS, SRCCOPY);

  StretchDIBits(
    device_context,
    0, 0, window_width, window_height,
    0, 0, bitmap_width, bitmap_height,
    bitmap_memory,
    &bitmap_info,
    DIB_RGB_COLORS, SRCCOPY);

}
  
LRESULT CALLBACK os_w32_wnd_proc(HWND window, UINT message, WPARAM w_param, LPARAM l_param)
 {
  LRESULT result = 0;

  switch (message)
  {
    case WM_SIZE:
    {
      OutputDebugStringA("WM_SIZE\n");
      RECT client_rect;

      GetClientRect(window, &client_rect);
      S32 width = client_rect.right - client_rect.left;
      S32 height = client_rect.bottom - client_rect.top;
      os_w32_resize_DIB_section(width, height);
      InvalidateRect(window, 0, true);
    }
    break;
    case WM_DESTROY:
    {
      // TODO: handle this as an error - recreate window?
      running = false;
    }
    break;
    case WM_CLOSE:
    {
      // TODO:handle this with a message to the user?
      running = false;
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
      RECT rc = paint.rcPaint;
      S32 x = rc.left;
      S32 y = rc.top;
      S32 width = rc.right - rc.left;
      S32 height = rc.bottom - rc.top;
      RECT client_rect;
      GetClientRect(window, &client_rect);
      os_w32_update_window(device_context, client_rect, x, y, width, height);
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

  WNDCLASSEXW window_class = {
      .cbSize = sizeof(WNDCLASSEX),
      .style = CS_HREDRAW | CS_VREDRAW,
      .lpfnWndProc = os_w32_wnd_proc,
      .hInstance = hInstance,
      //.hIcon
      .lpszClassName = L"CengineWindowClass",
      .hCursor = LoadCursor(0, IDC_ARROW),
      .hIcon = LoadIcon(0, IDI_APPLICATION),
  };

  if (RegisterClassExW(&window_class))
  {
    // clang-format off
    HWND window = CreateWindowExW(
      0 ,
      window_class.lpszClassName,
      L"c-engine",
      WS_OVERLAPPEDWINDOW | WS_VISIBLE,
      CW_USEDEFAULT,
      CW_USEDEFAULT,
      1600, 900,
         0,   0,
      hInstance,
      0);
    // clang-format on

    printf("window: %p\n", (int *)window);
    // Window Message Loop
    if (window)
    {
      S32 x_offset = 0;
      S32 y_offset = 0;
      running = true;
      while (running)
      {
        MSG message;
        while(PeekMessage(&message, 0, 0, 0, PM_REMOVE))
        {
          if(message.message == WM_QUIT) { running = false; }
          
          TranslateMessage(&message);
          DispatchMessage(&message);
        }

        render_weird_gradient(x_offset++,y_offset++);
        HDC device_context = GetDC(window);
        RECT client_rect;
        GetClientRect(window, &client_rect);
        S32 window_width = client_rect.right - client_rect.left;
        S32 window_height = client_rect.bottom - client_rect.top;
        os_w32_update_window(device_context, client_rect, 0, 0, window_width, window_height);
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
