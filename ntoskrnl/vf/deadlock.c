/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/vf/deadlock.c
 * PURPOSE:         Driver Verifier deadlock detection
 * PROGRAMMERS:     Andrew J. Bromage
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#ifdef _MSC_VER
#pragma section("VERIFY", read,execute)
#pragma section("VERIFYCONST", read)
#pragma section("VERIFYDATA", read,write)
#pragma section("VERIFYBSS", read,write)
#endif


/* FUNCTIONS *****************************************************************/


void
FASTCALL
VfpInitializeDeadlockableResource()
{
}


void
FASTCALL
VfpAcquireDeadlockableResource()
{
}


void
FASTCALL
VfpReleaseDeadlockableResource()
{
}


/* EOF */
