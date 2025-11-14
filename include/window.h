#ifndef WINDOW_H
#define WINDOW_H

#include <windows.h>

#define WINDOW_CLASS_NAME "SecondBurstWindowClass"

#define WINDOW_WIDTH 400
#define WINDOW_HEIGHT 300

BOOL InitWindowClass(HINSTANCE hInstance);

HWND CreateMainWindow(HINSTANCE hInstance);

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

#endif 

