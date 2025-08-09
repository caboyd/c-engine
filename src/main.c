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

  WNDCLASS window_class = {
      .style = CS_HREDRAW | CS_VREDRAW,
      .lpfnWndProc = os_w32_wnd_proc,
      .hInstance = hInstance,
      //.hIcon
      .lpszClassName = "CengineWindowClass",
      // .hCursor = LoadCursorA(0, IDC_ARROW),
      // .hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(1)),
  };
  ATOM rc = RegisterClass(&window_class);
  if (rc)
  {
    printf("RegisterClass: %d\n", (int)GetLastError());
    // clang-format off
    HWND window_handle  = CreateWindowEx(
      0 ,
      window_class.lpszClassName,
      "c-engine",
      WS_OVERLAPPEDWINDOW | WS_VISIBLE,
      CW_USEDEFAULT,
        CW_USEDEFAULT,
      CW_USEDEFAULT, CW_USEDEFAULT,
           0,   0,
      hInstance,
      0);
    // clang-format on

    printf("window_handle: %p\n", (int *)window_handle);
    printf("CreateWindow: %d\n", (int)GetLastError());
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
      OutputDebugStringA("CreateWindowEx Failed");
    }
  }
  else
  {
    // TODO:pile: Logging
    OutputDebugStringA("RegisterClass failed");
  }

  return 0;
};
