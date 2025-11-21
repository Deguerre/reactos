/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/mm/ARM3/drvmgmt.c
 * PURPOSE:         ARM Memory Manager Driver Management
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#define MODULE_INVOLVED_IN_ARM3
#include <mm/ARM3/miarm.h>

/* GLOBALS *******************************************************************/

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @unimplemented
 */
VOID
NTAPI
MmUnlockPageableImageSection(IN PVOID ImageSectionHandle)
{
    static ULONG Warn; if (!Warn++) UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
VOID
NTAPI
MmLockPageableSectionByHandle(IN PVOID ImageSectionHandle)
{
    UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
PVOID
NTAPI
MmLockPageableDataSection(IN PVOID AddressWithinSection)
{
    //
    // We should just find the section and call MmLockPageableSectionByHandle
    //
    static ULONG Warn; if (!Warn++) UNIMPLEMENTED;
    return AddressWithinSection;
}

/*
 * @unimplemented
 */
ULONG
NTAPI
MmTrimAllSystemPageableMemory(IN ULONG PurgeTransitionList)
{
    UNIMPLEMENTED;
    return 0;
}

/* EOF */
