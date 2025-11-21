#define _CRT_SECURE_NO_WARNINGS
#include "utils.h"
#include "verifier.h"
#include <locale.h>
#include <stdio.h>
#undef WIN32_NO_STATUS
#include <ntstatus.h>


#define MESSAGE_LENGTH  256


BOOL VrfyLoadString(UINT nResource, TCHAR *szBuffer, int uBufSize)
{
	if (uBufSize < 2)
	{
		return FALSE;
	}
	return LoadString(g_hResourceModule, nResource, szBuffer, uBufSize) > 0;
}

BOOL VrfyConsoleMode()
{
	if (!AttachConsole(ATTACH_PARENT_PROCESS) && !AllocConsole())
	{
		return FALSE;
	}

#ifdef __REACTOS__
	freopen("CONIN$", "r", stdin);
	freopen("CONOUT$", "w", stdout);
	freopen("CONERR$", "w", stderr);
#else
	{
		FILE* fDummy;
		freopen_s(&fDummy, "CONIN$", "r", stdin);
		freopen_s(&fDummy, "CONOUT$", "w", stdout);
		freopen_s(&fDummy, "CONERR$", "w", stderr);
	}
#endif
	_tsetlocale(LC_ALL, _T(".OCP"));

	return TRUE;
}

void VrfyPrint(UINT nResource)
{
	TCHAR szMsg[MESSAGE_LENGTH];
	VrfyLoadString(nResource, szMsg, MESSAGE_LENGTH);
	_putts(szMsg);
}


void VrfyPrintf(UINT nResource, ...)
{
	TCHAR szFmt[MESSAGE_LENGTH];
	TCHAR szMsg[MESSAGE_LENGTH];
	va_list args;

	VrfyLoadString(nResource, szMsg, MESSAGE_LENGTH);

	va_start(args, nResource);
	_vsntprintf(szMsg, _countof(szFmt), szFmt, args);
	_putts(szMsg);
	va_end(args);
}

void VrfyErrorPrint(UINT nResource)
{
	TCHAR szMsg[MESSAGE_LENGTH];
	VrfyLoadString(nResource, szMsg, MESSAGE_LENGTH);

	if (g_bCommandLine)
	{
		_putts(szMsg);
	}
	else
	{
		MessageBox(g_hMainWnd, szMsg, _T(""), MB_OK | MB_ICONSTOP);
	}
}

void VrfyErrorPrintf(UINT nResource, ...)
{
	TCHAR szFmt[MESSAGE_LENGTH];
	TCHAR szMsg[MESSAGE_LENGTH];
	va_list args;

	VrfyLoadString(nResource, szMsg, MESSAGE_LENGTH);

	va_start(args, nResource);
	_vsntprintf(szMsg, _countof(szMsg), szFmt, args);
	if (g_bCommandLine)
	{
		_putts(szMsg);
	}
	else
	{
		MessageBox(g_hMainWnd, szMsg, _T(""), MB_OK | MB_ICONSTOP);
	}
	va_end(args);
}

