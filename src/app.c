#include "../include/app.h"
#include "../include/window.h"
#include <windows.h>

static AppState g_AppState = {0};

BOOL AppInit(HINSTANCE hInstance)
{
    g_AppState.hInstance = hInstance;
    g_AppState.isRunning = FALSE;

    if (!InitWindowClass(hInstance)) {
        return FALSE;
    }

    g_AppState.hwndMain = CreateMainWindow(hInstance);
    if (g_AppState.hwndMain == NULL) {
        return FALSE;
    }

    ShowWindow(g_AppState.hwndMain, SW_SHOW);
    UpdateWindow(g_AppState.hwndMain);
    
    g_AppState.isRunning = TRUE;
    return TRUE;
}

int AppRun(void)
{
    MSG msg = {0};

    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return (int)msg.wParam;
}

void AppShutdown(void)
{
    g_AppState.isRunning = FALSE;

    if (g_AppState.hInstance) {
        UnregisterClass(WINDOW_CLASS_NAME, g_AppState.hInstance);
    }
}


AppState* GetAppState(void)
{
    return &g_AppState;
}