#include <stdio.h>
#include <windows.h>

#include "helper.h"

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, int nCmdShow) {
  printf("%s\n", "hello world");
  helper_print();
  return 0;
};
