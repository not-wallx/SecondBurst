#include "../include/ui.h"
#include <windows.h>

void DrawUI(HDC hdc, RECT *rect)
{
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBitmap = CreateCompatibleBitmap(hdc, rect->right, rect->bottom);
    HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, memBitmap);

    HBRUSH bgBrush = CreateSolidBrush(COLOR_BACKGROUND);
    FillRect(memDC, rect, bgBrush);
    DeleteObject(bgBrush);
    
    RECT headerRect = {20, 20, rect->right - 20, 80};
    DrawRoundedRect(memDC, &headerRect, 10, COLOR_PRIMARY);

    RECT titleRect = headerRect;
    DrawCenteredText(memDC, "SecondBurst", &titleRect, RGB(255, 255, 255), 24);

    RECT contentRect = {20, 100, rect->right - 20, rect->bottom - 40};
    DrawRoundedRect(memDC, &contentRect, 10, COLOR_ACCENT);

    RECT subtitleRect = {40, 120, rect->right - 40, 160};
    DrawCenteredText(memDC, "Windows Desktop Utility", &subtitleRect, COLOR_TEXT, 14);

    RECT descRect = {40, 170, rect->right - 40, 230};
    SetBkMode(memDC, TRANSPARENT);
    SetTextColor(memDC, COLOR_TEXT);
    
    HFONT hFont = CreateFont(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                             DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                             CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
    HFONT oldFont = (HFONT)SelectObject(memDC, hFont);
    
    DrawText(memDC, "Built with pure C and Win32 API.\n"
                    "No external libraries required.\n"
                    "Lightweight and efficient.",
             -1, &descRect, DT_CENTER | DT_WORDBREAK);
    
    SelectObject(memDC, oldFont);
    DeleteObject(hFont);

    BitBlt(hdc, 0, 0, rect->right, rect->bottom, memDC, 0, 0, SRCCOPY);

    SelectObject(memDC, oldBitmap);
    DeleteObject(memBitmap);
    DeleteDC(memDC);
}

void DrawRoundedRect(HDC hdc, RECT *rect, int radius, COLORREF color)
{
    HBRUSH brush = CreateSolidBrush(color);
    HPEN pen = CreatePen(PS_SOLID, 1, color);
    
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, brush);
    HPEN oldPen = (HPEN)SelectObject(hdc, pen);
    
    RoundRect(hdc, rect->left, rect->top, rect->right, rect->bottom, 
              radius, radius);
    
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(brush);
    DeleteObject(pen);
}

void DrawCenteredText(HDC hdc, const char *text, RECT *rect, COLORREF color, int fontSize)
{
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, color);
    
    HFONT hFont = CreateFont(fontSize, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                             DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                             CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
    HFONT oldFont = (HFONT)SelectObject(hdc, hFont);
    
    DrawText(hdc, text, -1, rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    
    SelectObject(hdc, oldFont);
    DeleteObject(hFont);
}