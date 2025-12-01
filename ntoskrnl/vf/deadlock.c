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


/* CONSTANTS *****************************************************************/


#define VFP_DEADLOCK_TAG							  'kcLD'
#define VFP_DEADLOCK_HASH_TABLE_SIZE    32
#define VFP_THREAD_SEED                 0x9e3779b9
#define VFP_RESOURCE_SEED               0x7f4a7c15


/* STRUCTURES *****************************************************************/

struct _VFP_DEADLOCK_THREAD;
typedef struct _VFP_DEADLOCK_THREAD VFP_DEADLOCK_THREAD, *PVFP_DEADLOCK_THREAD;

struct _VFP_DEADLOCK_RESOURCE;
typedef struct _VFP_DEADLOCK_RESOURCE VFP_DEADLOCK_RESOURCE, *PVFP_DEADLOCK_RESOURCE;

struct _VFP_DEADLOCK_ACQUISITION;
typedef struct _VFP_DEADLOCK_ACQUISITION VFP_DEADLOCK_ACQUISITION, *PVFP_DEADLOCK_ACQUISITION;


struct _VFP_DEADLOCK_THREAD {
    union {
        LIST_ENTRY HashChain;
        LIST_ENTRY FreeList;
    };
    PKTHREAD Thread;
    PVFP_DEADLOCK_ACQUISITION CurrentAcquisition;
};

struct _VFP_DEADLOCK_RESOURCE {
    union {
        LIST_ENTRY HashChain;
        LIST_ENTRY FreeList;
    };
    LIST_ENTRY Acquisitions;
    PVOID Resource;
    PVFP_DEADLOCK_THREAD Owner;
};

struct _VFP_DEADLOCK_ACQUISITION {
    union {
        LIST_ENTRY ChildList;
        LIST_ENTRY FreeList;
    };
    LIST_ENTRY SiblingList;
    LIST_ENTRY ResourceList;
    PVFP_DEADLOCK_ACQUISITION Parent;
    PVFP_DEADLOCK_RESOURCE RootResource;
    PVFP_DEADLOCK_THREAD Thread;
};



/* GLOBALS *****************************************************************/

// FIXME This is currently a glorified namespace, but there's no reason this
// couldn't be dynamically allocated if the verifier is running.
typedef struct _VFP_DEADLOCK_GLOBALS {
    LIST_ENTRY ThreadFreeList;
    LIST_ENTRY ResourceFreeList;
    LIST_ENTRY AcquisitionFreeList;
    ULONG FreeThreads;
    ULONG FreeResources;
    ULONG FreeAcquisitions;
    PLIST_ENTRY ThreadTable;
    PLIST_ENTRY ResourceTable;
    SIZE_T AllocationSize;
} VFP_DEADLOCK_GLOBALS, *PVFP_DEADLOCK_GLOBALS;

VFP_DEADLOCK_GLOBALS VfpDeadlockGlobals;



/* FUNCTIONS *****************************************************************/


static
void
FASTCALL
VfpFreeDeadlockThread(PVFP_DEADLOCK_THREAD Thread)
{
    InsertHeadList(&VfpDeadlockGlobals.ThreadFreeList, &Thread->FreeList);
    ++VfpDeadlockGlobals.FreeThreads;
}


static
void
FASTCALL
VfpFreeDeadlockResource(PVFP_DEADLOCK_RESOURCE Resource)
{
    InsertHeadList(&VfpDeadlockGlobals.ResourceFreeList, &Resource->FreeList);
    ++VfpDeadlockGlobals.FreeResources;
}


static
void
FASTCALL
VfpFreeDeadlockAcquisition(PVFP_DEADLOCK_ACQUISITION Acquisition)
{
    InsertHeadList(&VfpDeadlockGlobals.AcquisitionFreeList, &Acquisition->FreeList);
    ++VfpDeadlockGlobals.FreeAcquisitions;
}


static
ULONG
VfpDeadlockHash(
    IN ULONG Seed,
    IN PVOID Address
)
{
    ULONG Hash = VfpHashFunction(Seed, ((ULONG_PTR)Address >> PAGE_SHIFT));
    return Hash % VFP_DEADLOCK_HASH_TABLE_SIZE;
}




void
FASTCALL
VffInitializeDeadlockableResource()
{
}


void
FASTCALL
VffAcquireDeadlockableResource()
{
}


void
FASTCALL
VffReleaseDeadlockableResource()
{
}



static void
VfiInitializeDeadlockGlobals(PVFP_DEADLOCK_GLOBALS Globals)
{
    Globals->ThreadTable = NULL;
    Globals->ResourceTable = NULL;
    InitializeListHead(&Globals->ThreadFreeList);
    InitializeListHead(&Globals->ResourceFreeList);
    InitializeListHead(&Globals->AcquisitionFreeList);
    Globals->AllocationSize = 0;
}



void
FASTCALL
VfpInitializeDeadlockDetection()
{
    VfiInitializeDeadlockGlobals(&VfpDeadlockGlobals);
}


/* EOF */
