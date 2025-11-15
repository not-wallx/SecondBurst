#include <windows.h>
#include "../include/app.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, 
                   LPSTR lpCmdLine, int nCmdShow)
{

    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;

    if (!AppInit(hInstance)) {
        MessageBox(NULL, "Failed to initialize application", "Error", 
                   MB_OK | MB_ICONERROR);
        return 1;
    }

    int exitCode = AppRun();

    AppShutdown();
    
    return exitCode;
}