#include "base/base_core.h"
#include "base/base_math.h"
#include <windows.h>

LRESULT CALLBACK os_w32_wnd_proc(HWND window, UINT message, WPARAM w_param, LPARAM l_param)
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
      OutputDebugStringA("WM_DESTROY\n");
      PostQuitMessage(0);
    }
    break;
    case WM_CLOSE:
    {
      OutputDebugStringA("WM_CLOSE\n");
      DestroyWindow(window);
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
      static U32 rop = WHITENESS;
      PatBlt(device_context, x, y, width, height, rop);

      if (rop == WHITENESS)
      {
        rop = BLACKNESS;
      }
      else if (rop == BLACKNESS)
      {
        rop = WHITENESS;
      };

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
      MSG message;
      for (;;)
      {
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
