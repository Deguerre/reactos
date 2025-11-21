
#include "framework.h"
#include "verifier.h"
#include "cmdline.h"
#include "utils.h"
#include "resource.h"

// Global Variables
HWND g_hMainWnd = NULL;
HMODULE g_hResourceModule = NULL;
BOOLEAN g_bCommandLine = FALSE;
HINSTANCE g_hInst = NULL;


int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPTSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    g_hInst = hInstance;
    g_hResourceModule = GetModuleHandle(NULL);

    if (__argc > 1)
    {
        if (!VrfyConsoleMode())
        {
            VrfyErrorPrint(IDS_CANT_ATTACH_CONSOLE);
            return 1;
        }

        g_hMainWnd = NULL;
        g_bCommandLine = TRUE;
        return VrfyCommandLine(__argc, __targv);
    }

    // Run as GUI
    VrfyErrorPrint(IDS_GUI_NYI);

    return 0;
}

