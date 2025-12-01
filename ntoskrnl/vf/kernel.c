/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/vf/kernel.c
 * PURPOSE:         Driver Verifier Kernel 
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#include "deadlock.h"

#undef NDEBUG
#include <debug.h>

/* FORWARD DECLARATIONS *****************************************************************/

extern VERIFIER_THUNK VfKernelThunks[];



/* FUNCTIONS *****************************************************************/


CODE_SEG("VERIFY")
void
VfpTrimPagableMemory()
{
    // TODO: Like it says on the tin, trim pagable memory.
    // See MmTrimAllSystemPageableMemory() in mm/ARM3/drvmgmt.c
}


CODE_SEG("VERIFY")
static
void
FASTCALL
VfCheckRaiseIrql(IN KIRQL NewIrql)
{
    KIRQL CurrentIrql;

    CurrentIrql = KeGetCurrentIrql();

    if (CurrentIrql > NewIrql) {
        // FIXME Check Arg1
        KeBugCheckEx(DRIVER_VERIFIER_DETECTED_VIOLATION,
                     0,
                     CurrentIrql,
                     NewIrql,
                     0);
    }
}


CODE_SEG("VERIFY")
static
void
FASTCALL
VfCheckLowerIrql(IN KIRQL NewIrql)
{
    KIRQL CurrentIrql;

    CurrentIrql = KeGetCurrentIrql();

    if (CurrentIrql < NewIrql) {
        // FIXME Check Arg1
        KeBugCheckEx(DRIVER_VERIFIER_DETECTED_VIOLATION,
                     0,
                     CurrentIrql,
                     NewIrql,
                     0);
    }
}


CODE_SEG("VERIFY")
BOOLEAN
NTAPI
VfEnableVerifierForDriver(IN PLDR_DATA_TABLE_ENTRY DataTableEntry)
{
    PULONG_PTR ImportData;
    ULONG ImportDataSize;
    ULONG i;
    BOOLEAN RoutineFound;

    ImportData = (PULONG_PTR)RtlImageDirectoryEntryToData(
            DataTableEntry->DllBase,
            TRUE,
            IMAGE_DIRECTORY_ENTRY_IAT,
            &ImportDataSize);

    if (!ImportData || (ImportDataSize % sizeof(PULONG_PTR)))
    {
        return FALSE;
    }

    RoutineFound = FALSE;
    ImportDataSize /= sizeof(PULONG_PTR);
    for (i = 0; i < ImportDataSize; ++i, ++ImportData)
    {
        if (!VfVerifyKernel)
        {
            
        }
    }

    return TRUE;
}


CODE_SEG("INIT")
VOID
VfInitializeKernel()
{
}


/* THUNKS *****************************************************************/

#ifdef _MSC_VER
#pragma code_seg(push,"VERIFY")
#pragma const_seg(push,"VERIFYCONST")
#pragma data_seg(push,"VERIFYDATA")
#pragma bss_seg(push,"VERIFYBSS")
#endif

// Undefine macros so that we get the real routines
#undef KeRaiseIrql
#undef KeLowerIrql
#undef KeAcquireSpinLock
#undef KeReleaseSpinLock
#undef KeAcquireSpinLockAtDpcLevel
#undef KeReleaseSpinLockFromDpcLevel

extern void NTAPI KeRaiseIrql(IN KIRQL NewIrql, OUT PKIRQL OldIrql);
extern void NTAPI KeLowerIrql(IN KIRQL NewIrql);
extern void NTAPI KeAcquireSpinLock(IN PKSPIN_LOCK SpinLock, OUT PKIRQL OldIrql);
extern void NTAPI KeReleaseSpinLock(IN PKSPIN_LOCK SpinLock, IN KIRQL NewIrql);
extern void NTAPI KeAcquireSpinLockAtDpcLevel(IN PKSPIN_LOCK SpinLock);
extern void NTAPI KeReleaseSpinLockFromDpcLevel(IN PKSPIN_LOCK SpinLock);


#if !defined(_M_IX86) && !defined(_M_AMD64)
#error "Driver verifier not ported to this platform yet"
#endif


//
// This macro contains a list of all thunked functions. Each non-last macro
// has four arguments:
//     - The name of the thunked function
//     - A typedef and enumeration-friendly name for the thunked function.
//     - The return type
//     - The calling convention
//     - The arguments
//
// KT_ALL means that the function is always present.
// KT_IX86 means that the function is x86-only.
// KT_AMD64 means that the function is amd64-only.
// KT_LAST() marks the end of the list.
//
#define VF_KERNEL_THUNKS \
    KT_IX86(KeRaiseIrql, KE_RAISE_IRQL, void, , (IN KIRQL, OUT PKIRQL)) \
    KT_IX86(KfRaiseIrql, KF_RAISE_IRQL, void, FASTCALL, (IN KIRQL)) \
    KT_ALL(KeRaiseIrqlToDpcLevel, KE_RAISE_IRQL_TO_DPC_LEVEL, KIRQL, , ()) \
    KT_ALL(KeLowerIrql, KE_LOWER_IRQL, void, , (IN KIRQL)) \
    KT_IX86(KfLowerIrql, KF_LOWER_IRQL, void, FASTCALL, (IN KIRQL)) \
    KT_ALL(KeInitializeSpinLock, KE_INITIALIZE_SPIN_LOCK, void, , (IN PKSPIN_LOCK)) \
    KT_IX86(KeAcquireSpinLock, KE_ACQUIRE_SPIN_LOCK, void, , (IN PKSPIN_LOCK, OUT PKIRQL)) \
    KT_IX86(KfAcquireSpinLock, KF_ACQUIRE_SPIN_LOCK, KIRQL, FASTCALL, (IN PKSPIN_LOCK)) \
    KT_ALL(KeReleaseSpinLock, KE_RELEASE_SPIN_LOCK, void, , (IN PKSPIN_LOCK, IN KIRQL)) \
    KT_IX86(KfReleaseSpinLock, KF_RELEASE_SPIN_LOCK, void, FASTCALL, (IN PKSPIN_LOCK, IN KIRQL)) \
    KT_ALL(KeAcquireSpinLockAtDpcLevel, KE_ACQUIRE_SPIN_LOCK_AT_DPC_LEVEL, void, , (IN PKSPIN_LOCK)) \
    KT_AMD64(KeAcquireSpinLockRaiseToDpc, KE_ACQUIRE_SPIN_LOCK_RAISE_TO_DPC, KIRQL, , (IN PKSPIN_LOCK)) \
    KT_AMD64(KeAcquireSpinLockRaiseToSynch, KE_ACQUIRE_SPIN_LOCK_RAISE_TO_SYNCH, KIRQL, , (IN PKSPIN_LOCK)) \
    KT_ALL(KeReleaseSpinLockFromDpcLevel, KE_RELEASE_SPIN_LOCK_FROM_DPC_LEVEL, void, , (IN PKSPIN_LOCK)) \
    KT_LAST()


//
// Enumeration for the thunked functions
//
#define KT_ALL(n,t,rt,cc,args) \
    VFE_##t,

#ifdef _M_AMD64
#define KT_AMD64(n,t,rt,cc,args) KT_ALL(n,t,rt,cc,args)
#else
#define KT_AMD64(n,t,rt,cc,args)
#endif

#ifdef _M_IX86
#define KT_IX86(n,t,rt,cc,args) KT_ALL(n,t,rt,cc,args)
#else
#define KT_IX86(n,t,rt,cc,args)
#endif

#define KT_LAST() \
    VFE_COUNT

typedef enum _VF_KERNEL_THUNK_NUM {
VF_KERNEL_THUNKS
} VF_KERNEL_THUNK_NUM;

#undef KT_ALL
#undef KT_IX86
#undef KT_AMD64
#undef KT_LAST

//
// Declare typedefs for the thunked functions
//
#define KT_ALL(n,t,rt,cc,args) \
    typedef rt (cc *P##t) args;

#ifdef _M_AMD64
#define KT_AMD64(n,t,rt,cc,args) KT_ALL(n,t,rt,cc,args)
#else
#define KT_AMD64(n,t,rt,cc,args)
#endif

#ifdef _M_IX86
#define KT_IX86(n,t,rt,cc,args) KT_ALL(n,t,rt,cc,args)
#else
#define KT_IX86(n,t,rt,cc,args)
#endif

#define KT_LAST()

VF_KERNEL_THUNKS

#undef KT_ALL
#undef KT_IX86
#undef KT_AMD64
#undef KT_LAST

//
// Forward declare the thunks
//
#define KT_ALL(n,t,rt,cc,args) \
    rt cc Vrfy##n args;

#ifdef _M_AMD64
#define KT_AMD64(n,t,rt,cc,args) KT_ALL(n,t,rt,cc,args)
#else
#define KT_AMD64(n,t,rt,cc,args)
#endif

#ifdef _M_IX86
#define KT_IX86(n,t,rt,cc,args) KT_ALL(n,t,rt,cc,args)
#else
#define KT_IX86(n,t,rt,cc,args)
#endif

#define KT_LAST()

VF_KERNEL_THUNKS

#undef KT_ALL
#undef KT_IX86
#undef KT_AMD64
#undef KT_LAST


//
// Declare the kernel thunk list
//
#define KT_ALL(n,t,rt,cc,args) \
    { #n, NULL, (PVOID)Vrfy##n },

#ifdef _M_AMD64
#define KT_AMD64(n,t,rt,cc,args) KT_ALL(n,t,rt,cc,args)
#else
#define KT_AMD64(n,t,rt,cc,args)
#endif

#ifdef _M_IX86
#define KT_IX86(n,t,rt,cc,args) KT_ALL(n,t,rt,cc,args)
#else
#define KT_IX86(n,t,rt,cc,args)
#endif

#define KT_LAST() \
    { NULL, NULL, NULL }

VERIFIER_THUNK VfKernelThunks[] = {
    VF_KERNEL_THUNKS
};

#undef KT_ALL
#undef KT_IX86
#undef KT_AMD64
#undef KT_LAST

#undef VF_KERNEL_THUNKS


#ifdef _M_IX86
PVOID VfiOriginalHalRoutines[VFE_COUNT];
#endif


void
VrfyKeInitializeSpinLock(
    IN PKSPIN_LOCK SpinLock
)
{
    KeInitializeSpinLock(SpinLock);
    VfiInitializeDeadlockableResource();
}


#ifdef _M_IX86
void
VrfyKeAcquireSpinLock(
    IN PKSPIN_LOCK SpinLock,
    OUT PKIRQL OldIrql
)
{
    KIRQL CurrentIrql;

    CurrentIrql = KeGetCurrentIrql();

    VfCheckRaiseIrql(DISPATCH_LEVEL);

    InterlockedIncrementUL(&VfpGlobalData.AcquireSpinLocks);

    if (VfpGlobalData.Level & DRIVER_VERIFIER_FORCE_IRQL_CHECKING)
    {
        if (CurrentIrql < DISPATCH_LEVEL)
        {
            VfpTrimPagableMemory();
        }
    }

    PKE_ACQUIRE_SPIN_LOCK OrigKeAcquireSpinLock;
    OrigKeAcquireSpinLock = (PKE_ACQUIRE_SPIN_LOCK)VfiOriginalHalRoutines[VFE_KE_ACQUIRE_SPIN_LOCK];

    if (OrigKeAcquireSpinLock)
    {
        (*OrigKeAcquireSpinLock)(SpinLock, OldIrql);
        VfiAcquireDeadlockableResource();
    }
    else
    {
        // FIXME Check this
        KeBugCheckEx(DRIVER_VERIFIER_DETECTED_VIOLATION,
                     0,
                     VFE_KE_ACQUIRE_SPIN_LOCK,
                     0,
                     0);
    }
}
#endif


void
VrfyKeReleaseSpinLock(
    IN PKSPIN_LOCK SpinLock,
    IN KIRQL NewIrql
)
{
    KIRQL CurrentIrql;

    CurrentIrql = KeGetCurrentIrql();

    if (CurrentIrql < DISPATCH_LEVEL)
    {
        // FIXME Check Arg1
        KeBugCheckEx(DRIVER_VERIFIER_DETECTED_VIOLATION,
                     0,
                     CurrentIrql,
                     (ULONG_PTR)SpinLock,
                     0);
    }

    VfCheckLowerIrql(NewIrql);

#ifdef _M_IX86
    PKE_RELEASE_SPIN_LOCK OrigKeReleaseSpinLock;
    OrigKeReleaseSpinLock = (PKE_RELEASE_SPIN_LOCK)VfiOriginalHalRoutines[VFE_KE_RELEASE_SPIN_LOCK];

    if (OrigKeReleaseSpinLock)
    {
        VfiReleaseDeadlockableResource();
        (*OrigKeReleaseSpinLock)(SpinLock, NewIrql);
        return;
    }
    else
    {
        // FIXME Check this
        KeBugCheckEx(DRIVER_VERIFIER_DETECTED_VIOLATION,
                     0,
                     VFE_KE_RELEASE_SPIN_LOCK,
                     0,
                     0);
    }
#else
    VfiReleaseDeadlockableResource();
    KeReleaseSpinLock(SpinLock, NewIrql);
#endif
}



#ifdef _M_IX86
KIRQL
FASTCALL
VrfyKfAcquireSpinLock(
    IN PKSPIN_LOCK SpinLock
)
{
    KIRQL CurrentIrql;
    KIRQL PrevIrql;

    CurrentIrql = KeGetCurrentIrql();

    VfCheckRaiseIrql(DISPATCH_LEVEL);

    InterlockedIncrementUL(&VfpGlobalData.AcquireSpinLocks);

    if (VfpGlobalData.Level & DRIVER_VERIFIER_FORCE_IRQL_CHECKING)
    {
        if (CurrentIrql < DISPATCH_LEVEL)
        {
            VfpTrimPagableMemory();
        }
    }

    PKF_ACQUIRE_SPIN_LOCK OrigKfAcquireSpinLock;
    OrigKfAcquireSpinLock = (PKF_ACQUIRE_SPIN_LOCK)VfiOriginalHalRoutines[VFE_KF_ACQUIRE_SPIN_LOCK];

    if (OrigKfAcquireSpinLock)
    {
        PrevIrql = (*OrigKfAcquireSpinLock)(SpinLock);
        VfiAcquireDeadlockableResource();
        return PrevIrql;
    }
    else
    {
        // FIXME Check this
        KeBugCheckEx(DRIVER_VERIFIER_DETECTED_VIOLATION,
                     0,
                     VFE_KF_ACQUIRE_SPIN_LOCK,
                     0,
                     0);
    }
}


void
FASTCALL
VrfyKfReleaseSpinLock(
    IN PKSPIN_LOCK SpinLock,
    IN KIRQL NewIrql
)
{
    KIRQL CurrentIrql;

    CurrentIrql = KeGetCurrentIrql();

    if (CurrentIrql < DISPATCH_LEVEL)
    {
        // FIXME Check Arg1
        KeBugCheckEx(DRIVER_VERIFIER_DETECTED_VIOLATION,
                     0,
                     CurrentIrql,
                     (ULONG_PTR)SpinLock,
                     0);
    }

    VfCheckLowerIrql(NewIrql);

    PKF_RELEASE_SPIN_LOCK OrigKfReleaseSpinLock;
    OrigKfReleaseSpinLock = (PKF_RELEASE_SPIN_LOCK)VfiOriginalHalRoutines[VFE_KF_RELEASE_SPIN_LOCK];

    if (OrigKfReleaseSpinLock)
    {
        VfiReleaseDeadlockableResource();
        (*OrigKfReleaseSpinLock)(SpinLock, NewIrql);
        return;
    }
    else
    {
        // FIXME Check this
        KeBugCheckEx(DRIVER_VERIFIER_DETECTED_VIOLATION,
                     0,
                     VFE_KF_RELEASE_SPIN_LOCK,
                     0,
                     0);
    }
}
#endif



KIRQL
VrfyKeRaiseIrqlToDpcLevel()
{
    KIRQL CurrentIrql;

    CurrentIrql = KeGetCurrentIrql();

    VfCheckRaiseIrql(DISPATCH_LEVEL);

    InterlockedIncrementUL(&VfpGlobalData.RaiseIrqls);

    if (MmVerifierData.Level & DRIVER_VERIFIER_FORCE_IRQL_CHECKING)
    {
        if (CurrentIrql < DISPATCH_LEVEL)
        {
            VfpTrimPagableMemory();
        }
    }

#if defined (_X86_)
    PKE_RAISE_IRQL_TO_DPC_LEVEL OrigKeRaiseIrqlToDpcLevel;
    OrigKeRaiseIrqlToDpcLevel = (PKE_RAISE_IRQL_TO_DPC_LEVEL)VfiOriginalHalRoutines[VFE_KE_RAISE_IRQL_TO_DPC_LEVEL];
    if (OrigKeRaiseIrqlToDpcLevel)
    {
        return (*OrigKeRaiseIrqlToDpcLevel)();
    }
    else
    {
        // FIXME Check this
        KeBugCheckEx(DRIVER_VERIFIER_DETECTED_VIOLATION,
                     0,
                     VFE_KE_RAISE_IRQL_TO_DPC_LEVEL,
                     0,
                     0);
    }
#endif

    return KeRaiseIrqlToDpcLevel();
}


#ifdef _M_IX86
void
FASTCALL
VrfyKfRaiseIrql(IN KIRQL NewIrql)
{
    PKF_RAISE_IRQL OrigKfRaiseIrql;

    VfCheckRaiseIrql(NewIrql);

    InterlockedIncrementUL(&VfpGlobalData.RaiseIrqls);

    OrigKfRaiseIrql = (PKF_RAISE_IRQL)VfiOriginalHalRoutines[VFE_KF_RAISE_IRQL];
    if (OrigKfRaiseIrql)
    {
        (*OrigKfRaiseIrql)(NewIrql);
        return;
    }
    else
    {
        // FIXME Check this
        KeBugCheckEx(DRIVER_VERIFIER_DETECTED_VIOLATION,
                     0,
                     VFE_KF_RAISE_IRQL,
                     0,
                     0);
    }
}
#endif


#ifdef _M_IX86
void
FASTCALL
VrfyKfLowerIrql(IN KIRQL NewIrql)
{
    PKF_LOWER_IRQL OrigKfLowerIrql;

    VfCheckLowerIrql(NewIrql);

    OrigKfLowerIrql = (PKF_LOWER_IRQL)VfiOriginalHalRoutines[VFE_KF_LOWER_IRQL];
    if (OrigKfLowerIrql)
    {
        (*OrigKfLowerIrql)(NewIrql);
        return;
    }
    else
    {
        // FIXME Check this
        KeBugCheckEx(DRIVER_VERIFIER_DETECTED_VIOLATION,
                     0,
                     VFE_KF_LOWER_IRQL,
                     0,
                     0);
    }
}
#endif


#ifdef _M_IX86
void
VrfyKeRaiseIrql(IN KIRQL NewIrql, OUT PKIRQL OldIrql)
{
    KIRQL CurrentIrql;

    CurrentIrql = KeGetCurrentIrql();

    VfCheckRaiseIrql(NewIrql);

    InterlockedIncrementUL(&VfpGlobalData.RaiseIrqls);

    if (VfpGlobalData.Level & DRIVER_VERIFIER_FORCE_IRQL_CHECKING)
    {
        if (CurrentIrql < DISPATCH_LEVEL)
        {
            VfpTrimPagableMemory();
        }
    }

    PKE_RAISE_IRQL OrigKeRaiseIrql;

    OrigKeRaiseIrql = (PKE_RAISE_IRQL)VfiOriginalHalRoutines[VFE_KE_RAISE_IRQL];
    if (OrigKeRaiseIrql)
    {
        (*OrigKeRaiseIrql)(NewIrql, OldIrql);
    }
    else
    {
        // FIXME Check this
        KeBugCheckEx(DRIVER_VERIFIER_DETECTED_VIOLATION,
                     0,
                     VFE_KE_RAISE_IRQL,
                     0,
                     0);
    }
}
#endif


void
VrfyKeLowerIrql(IN KIRQL NewIrql)
{
    VfCheckLowerIrql(NewIrql);

#if defined (_X86_)
    PKE_LOWER_IRQL OrigKeLowerIrql;

    OrigKeLowerIrql = (PKE_LOWER_IRQL)VfiOriginalHalRoutines[VFE_KE_LOWER_IRQL];
    if (OrigKeLowerIrql)
    {
        (*OrigKeLowerIrql)(NewIrql);
    }
    else
    {
        // FIXME Check this
        KeBugCheckEx(DRIVER_VERIFIER_DETECTED_VIOLATION,
                     0,
                     VFE_KE_LOWER_IRQL,
                     0,
                     0);
    }
#else
    KeLowerIrql(NewIrql);
#endif
}



void
VrfyKeAcquireSpinLockAtDpcLevel(IN PKSPIN_LOCK SpinLock)
{
    KIRQL CurrentIrql;

    CurrentIrql = KeGetCurrentIrql();

    if (CurrentIrql < DISPATCH_LEVEL) {
        // FIXME Check Arg1
        KeBugCheckEx(DRIVER_VERIFIER_DETECTED_VIOLATION,
                     0,
                     CurrentIrql,
                     (ULONG_PTR)SpinLock,
                     0);
    }

    InterlockedIncrementUL(&VfpGlobalData.AcquireSpinLocks);

    KeAcquireSpinLockAtDpcLevel(SpinLock);

    VfiAcquireDeadlockableResource();
}


void
VrfyKeReleaseSpinLockFromDpcLevel(IN PKSPIN_LOCK SpinLock)
{
    KIRQL CurrentIrql;

    CurrentIrql = KeGetCurrentIrql();

    if (CurrentIrql < DISPATCH_LEVEL) {
        // FIXME Check Arg1
        KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                      0,
                      CurrentIrql,
                      (ULONG_PTR)SpinLock,
                      0);
    }

    VfiReleaseDeadlockableResource();

    KeReleaseSpinLockFromDpcLevel(SpinLock);
}


#ifdef _M_AMD64
KIRQL
VrfyKeAcquireSpinLockRaiseToDpc(IN PKSPIN_LOCK SpinLock)
{
    KIRQL NewIrql;

    VfCheckRaiseIrql(DISPATCH_LEVEL);

    InterlockedIncrementUL(&VfpGlobalData.RaiseIrqls);
    InterlockedIncrementUL(&VfpGlobalData.AcquireSpinLocks);

    NewIrql = KeAcquireSpinLockRaiseToDpc(SpinLock);

    VfiAcquireDeadlockableResource();

    return NewIrql;
}


KIRQL
VrfyKeAcquireSpinLockRaiseToSynch(IN PKSPIN_LOCK SpinLock)
{
    KIRQL NewIrql;

    VfCheckRaiseIrql(SYNCH_LEVEL);

    InterlockedIncrementUL(&VfpGlobalData.RaiseIrqls);
    InterlockedIncrementUL(&VfpGlobalData.AcquireSpinLocks);

    NewIrql = KeAcquireSpinLockRaiseToDpc(SpinLock);

    VfiAcquireDeadlockableResource();

    return NewIrql;
}
#endif



#ifdef _MSC_VER
#pragma code_seg(pop)
#pragma const_seg(pop)
#pragma data_seg(pop)
#pragma bss_seg(pop)
#endif

/* EOF */
