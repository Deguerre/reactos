#include "framework.h"
#include "cmdline.h"
#include "utils.h"
#include "resource.h"
#include "vrfsys.h"

#define MAX_SWITCH_LENGTH 32

static BOOL
ShowHelp(int argc, TCHAR* targv[])
{
	TCHAR szHelpSwitch[MAX_SWITCH_LENGTH];
	VrfyLoadString(IDS_CMDLINE_HELP, szHelpSwitch, MAX_SWITCH_LENGTH);
	if (argc == 2 && !_tcsicmp(szHelpSwitch, targv[1]))
	{
		VrfyPrint(IDS_HELP_CMDLINE_1);
		VrfyPrint(IDS_HELP_CMDLINE_2);
		return TRUE;
	}
	return FALSE;
}


static BOOL
DoReset(int argc, TCHAR* targv[])
{
	TCHAR szResetSwitch[MAX_SWITCH_LENGTH];
	VrfyLoadString(IDS_CMDLINE_RESET, szResetSwitch, MAX_SWITCH_LENGTH);
	if (argc == 2 && !_tcsicmp(szResetSwitch, targv[1]))
	{
		return TRUE;
	}
	return FALSE;
}


static BOOL
DoQuery(int argc, TCHAR* targv[])
{
	TCHAR szQuerySwitch[MAX_SWITCH_LENGTH];
	VrfyLoadString(IDS_CMDLINE_QUERY, szQuerySwitch, MAX_SWITCH_LENGTH);
	if (argc == 2 && !_tcsicmp(szQuerySwitch, targv[1]))
	{
		return TRUE;
	}
	return FALSE;
}


int
VrfyCommandLine(int argc, TCHAR* targv[])
{
	if (ShowHelp(argc, targv))
	{
		return 0;
	}

	if (DoReset(argc, targv))
	{
		return VrfyReset() ? 0 : 1;
	}

	if (DoQuery(argc, targv))
	{
		VrfyQueryToFile(stdout);
		return 0;
	}

	return 0;
}
