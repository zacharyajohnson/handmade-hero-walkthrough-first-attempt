#include <windows.h>

int CALLBACK WinMain(
  HINSTANCE hInstance,
  HINSTANCE hPrevInstance,
  LPSTR     lpCmdLine,
  int       nShowCmd
) {
    MessageBox(0, "This is handmade hero", "Handmade Hero", MB_OK | MB_ICONINFORMATION);
  return 0;
}