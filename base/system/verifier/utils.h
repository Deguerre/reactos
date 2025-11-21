#pragma once

#include "framework.h"

#ifdef __cplusplus
extern "C" {
#endif

	extern BOOL VrfyLoadString(UINT nResource, TCHAR* szBuffer, int uBufSize);

	extern BOOL VrfyConsoleMode();

	extern void VrfyPrint(UINT nResource);
	extern void VrfyPrintf(UINT nResource, ...);
	extern void VrfyErrorPrint(UINT nResource);
	extern void VrfyErrorPrintf(UINT nResource, ...);

#ifdef __cplusplus
}
#endif
