#ifndef APP_H
#define APP_H

#include <windows.h>

typedef struct {
    HINSTANCE hInstance;
    HWND hwndMain;
    BOOL isRunning;
} AppState;

BOOL AppInit(HINSTANCE hInstance);

int AppRun(void);

void AppShutdown(void);

AppState* GetAppState(void);

#endif 