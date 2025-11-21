/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/mm/ARM3/sysldr.c
 * PURPOSE:         Contains the Kernel Loader (SYSLDR) for loading PE files.
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  ReactOS Portable Systems Group
 */


/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS ******************************************************************/

PLDR_DATA_TABLE_ENTRY
NTAPI
MiLookupDataTableEntry(IN PVOID Address)
{
    PLDR_DATA_TABLE_ENTRY LdrEntry, FoundEntry = NULL;
    PLIST_ENTRY NextEntry;
    PAGED_CODE();

    /* Loop entries */
    NextEntry = PsLoadedModuleList.Flink;
    do
    {
        /* Get the loader entry */
        LdrEntry =  CONTAINING_RECORD(NextEntry,
                                      LDR_DATA_TABLE_ENTRY,
                                      InLoadOrderLinks);

        /* Check if the address matches */
        if ((Address >= LdrEntry->DllBase) &&
            (Address < (PVOID)((ULONG_PTR)LdrEntry->DllBase +
                               LdrEntry->SizeOfImage)))
        {
            /* Found a match */
            FoundEntry = LdrEntry;
            break;
        }

        /* Move on */
        NextEntry = NextEntry->Flink;
    } while(NextEntry != &PsLoadedModuleList);

    /* Return the entry */
    return FoundEntry;
}

