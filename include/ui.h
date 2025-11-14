#ifndef UI_H
#define UI_H

#include <windows.h>

#define COLOR_BACKGROUND RGB(255, 255, 255)
#define COLOR_PRIMARY RGB(0, 120, 215)  
#define COLOR_TEXT RGB(0, 0, 0)             
#define COLOR_ACCENT RGB(240, 240, 240)     

void DrawUI(HDC hdc, RECT *rect);

void DrawRoundedRect(HDC hdc, RECT *rect, int radius, COLORREF color);

void DrawCenteredText(HDC hdc, const char *text, RECT *rect, COLORREF color, int fontSize);

#endif 


