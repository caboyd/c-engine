#include "base/base_core.h"
#include "base/base_math.h"
#include <stdbool.h>
#include <windows.h>

global bool running;
global BITMAPINFO bitmap_info;
global void* bitmap_memory;
global HBITMAP bitmap_handle;
global HDC bitmap_device_context;

internal void os_w32_resize_DIB_section(int width, int height) 
{
  
  if(bitmap_handle)
  {
    DeleteObject(bitmap_handle); 
  }
  if(!bitmap_device_context)
  {
    bitmap_device_context = CreateCompatibleDC(0);
  }

  bitmap_info.bmiHeader = (BITMAPINFOHEADER){
    .biSize = sizeof(BITMAPINFOHEADER),
    .biWidth = width,
    .biHeight = height,
    .biPlanes = 1,
    .biBitCount = 32,
    .biCompression = BI_RGB
  };

  bitmap_handle = CreateDIBSection(
    bitmap_device_context,
    &bitmap_info,
    DIB_RGB_COLORS,
    &bitmap_memory,
    0, 0);

}

internal void os_w32_update_window(HDC device_context, int x, int y, int width, int height)
{
  StretchDIBits(
    device_context,
    x, y, width, height, 
    x, y, width, height,
    &bitmap_memory, 
    &bitmap_info, 
    DIB_RGB_COLORS, SRCCOPY);

    // 4. BitBlt from memory DC to target DC (e.g. window)
    BitBlt(device_context, 0, 0, width, height, bitmap_device_context, 0, 0, SRCCOPY);
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
      os_w32_update_window(window, x, y, width, height);

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
      // .hCursor = LoadCursorA(0, IDC_ARROW),
      // .hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(1)),
  };

  if (RegisterClassExW(&window_class))
  {
    // clang-format off
    HWND window_handle  = CreateWindowExW(
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

    printf("window_handle: %p\n", (int *)window_handle);
    // Window Message Loop
    if (window_handle)
    {
      running = true;
      while (running)
      {
        MSG message;
        BOOL message_result = GetMessage(&message, 0, 0, 0);
        if (message_result > 0)
        {
          TranslateMessage(&message);
          DispatchMessage(&message);
        }
        else
        {
          break;
        }
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
