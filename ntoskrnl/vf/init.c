/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/vf/init.c
 * PURPOSE:         Driver verifier initialization
 * PROGRAMMERS:     Andrew J. Bromage (Deguerre@users.noreply.github.com)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#undef NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/


VERIFIER_DATA_GLOBAL VfpGlobalData;

MM_DRIVER_VERIFIER_DATA MmVerifierData;

// A list of VERIFIER_DRIVER_ENTRY structures, which represents the list of drivers
// for which verification has been requesed. If KfVerifyAllDrivers is true, this
// overrides the request list.
//
// This is protected for writes by VfRequestedDriverListLock. Reads are safe because
// this list is only ever added to.
LIST_ENTRY VfRequestedDriverList;
KSPIN_LOCK VfRequestedDriverListLock;

// A list of DRIVER_SPECIFIED_VERIFIER_THUNKS structures, which represents the list of
// drivers which are actively thunked.
//
// This is protected by MmSystemLoadLock.
LIST_ENTRY VfThunkedDriverList;

ULONG MiActiveVerifierThunks;
WCHAR MmVerifyDriverBuffer[VF_VERIFY_DRIVER_BUFFER_LENGTH];
ULONG MmVerifyDriverBufferLength = sizeof(MmVerifyDriverBuffer);
ULONG MmVerifyDriverBufferType = REG_NONE;
ULONG MmVerifyDriverLevel = -1;
PVOID MmTriageActionTaken;
LOGICAL MmDontVerifyRandomDrivers = TRUE;
LOGICAL MiVerifyAllDrivers = FALSE;
LOGICAL KfVerifyKernel = FALSE;
LOGICAL KfVerifyHal = FALSE;
PVOID KernelVerifier = NULL;

#ifdef _MSC_VER
#pragma const_seg(push,"VERIFYCONST")
#pragma data_seg(push,"VERIFYDATA")
#pragma bss_seg(push,"VERIFYBSS")
#endif


#ifdef _MSC_VER
#pragma const_seg(pop)
#pragma data_seg(pop)
#pragma bss_seg(pop)
#endif


/* FUNCTIONS *******************************************************************/


NTSTATUS
VfGetSystemInformation(
    IN PSYSTEM_VERIFIER_INFORMATION Information,
    IN ULONG Size,
    OUT PULONG ReqSize
)
{
    NTSTATUS Status;
    PKTHREAD CurrentThread;
    ULONG ActualSize;
    PSYSTEM_VERIFIER_INFORMATION LastInformation;

    PAGED_CODE();

    ActualSize = 0;

    CurrentThread = KeGetCurrentThread();
    KeEnterCriticalRegionThread(CurrentThread);

    KeWaitForSingleObject(&MmSystemLoadLock,
                          WrVirtualMemory,
                          KernelMode,
                          FALSE,
                          NULL);

    Status = STATUS_SUCCESS;

    _SEH2_TRY
    {
        PLIST_ENTRY Entry;
        UNICODE_STRING DriverName;
        for (Entry = VfRequestedDriverList.Flink; Entry != &VfRequestedDriverList; Entry = Entry->Flink)
        {
            PVERIFIER_DRIVER_ENTRY VerifierEntry = CONTAINING_RECORD(Entry, VERIFIER_DRIVER_ENTRY, Links);
            ULONG NextEntryOffset = sizeof(SYSTEM_VERIFIER_INFORMATION);

            if (!(VerifierEntry->Flags & VF_DRIVER_VERIFYING))
            {
                // Not verifying this driver, so don't include it in the sysinfo.
                continue;
            }

            ActualSize += sizeof(SYSTEM_VERIFIER_INFORMATION);
            if (ActualSize > Size)
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            DriverName.Length = VerifierEntry->DriverName.Length;
            DriverName.MaximumLength = VerifierEntry->DriverName.Length + sizeof(WCHAR);
            DriverName.Buffer = (PWSTR)((PCHAR)Information + NextEntryOffset);

            ActualSize += ROUND_UP(DriverName.MaximumLength, sizeof(PVOID));
            if (ActualSize > Size)
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            NextEntryOffset += ROUND_UP(DriverName.MaximumLength, sizeof(PVOID));
            RtlCopyMemory(DriverName.Buffer, VerifierEntry->DriverName.Buffer, VerifierEntry->DriverName.Length);
            DriverName.Buffer[VerifierEntry->DriverName.Length / sizeof(WCHAR)] = UNICODE_NULL;

            Information->Level = VfpGlobalData.Level;
            Information->DriverName = DriverName;
            Information->RaiseIrqls = VfpGlobalData.RaiseIrqls;
            Information->AcquireSpinLocks = VfpGlobalData.AcquireSpinLocks;
            Information->SynchronizeExecutions = VfpGlobalData.SynchronizeExecutions;
            Information->AllocationsAttempted = VfpGlobalData.AllocationsAttempted;
            Information->AllocationsSucceeded = VfpGlobalData.AllocationsSucceeded;
            Information->AllocationsSucceededSpecialPool = VfpGlobalData.AllocationsSucceededSpecialPool;
            Information->AllocationsWithNoTag = VfpGlobalData.AllocationsWithNoTag;
            Information->TrimRequests = VfpGlobalData.TrimRequests;
            Information->Trims = VfpGlobalData.Trims;
            Information->AllocationsFailed = VfpGlobalData.AllocationsFailed;
            Information->AllocationsFailedDeliberately = VfpGlobalData.AllocationsFailedDeliberately;
            Information->UnTrackedPool = VfpGlobalData.UnTrackedPool;

            Information->Loads = VerifierEntry->Loads;
            Information->Unloads = VerifierEntry->Unloads;
            Information->CurrentPagedPoolAllocations = VerifierEntry->CurrentPagedPoolAllocations;
            Information->CurrentNonPagedPoolAllocations = VerifierEntry->CurrentNonPagedPoolAllocations;
            Information->PeakPagedPoolAllocations = VerifierEntry->PeakPagedPoolAllocations;
            Information->PeakNonPagedPoolAllocations = VerifierEntry->PeakNonPagedPoolAllocations;
            Information->PagedPoolUsageInBytes = VerifierEntry->PagedPoolUsageInBytes;
            Information->NonPagedPoolUsageInBytes = VerifierEntry->NonPagedPoolUsageInBytes;
            Information->PeakPagedPoolUsageInBytes = VerifierEntry->PeakPagedPoolUsageInBytes;
            Information->PeakNonPagedPoolUsageInBytes = VerifierEntry->PeakNonPagedPoolUsageInBytes;

            Information->NextEntryOffset = NextEntryOffset;
            LastInformation = Information;
            Information = (PSYSTEM_VERIFIER_INFORMATION)((PCHAR)Information + NextEntryOffset);
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    KeReleaseMutant(&MmSystemLoadLock, MUTANT_INCREMENT, FALSE, FALSE);

    KeLeaveCriticalRegionThread(CurrentThread);

    if (Status != STATUS_INFO_LENGTH_MISMATCH && LastInformation)
    {
        LastInformation->NextEntryOffset = 0;
        *ReqSize = ActualSize;
    }

    return Status;
}


static
VOID
VfpValidateRules(PULONG Rules)
{
    // Disable any rules not yet accepted
    *Rules &= ~DRIVER_VERIFIER_ACCEPTED_RULES;

    // Additional I/O checks require basic I/O checks.
    if ((*Rules & (DRIVER_VERIFIER_ENHANCED_IO_CHECKING | DRIVER_VERIFIER_FORCE_PENDING_IO_REQUESTS)))
    {
        *Rules |= DRIVER_VERIFIER_IO_CHECKING;
    }

    //
    // TODO:
    //
    // Invariant MDL checking also implies I/O checking.
    // Driver isolation checking also implies DIF.
    //
}


NTSTATUS
VfSetSystemInformation(
    IN PVOID Information,
    IN ULONG Size
)
{
    NTSTATUS Status;
    PKTHREAD CurrentThread;

    if (Size != sizeof(ULONG) && Size != sizeof(ULONGLONG))
    {
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    Status = STATUS_SUCCESS;

    CurrentThread = KeGetCurrentThread();
    KeEnterCriticalRegionThread(CurrentThread);

    KeWaitForSingleObject(&MmSystemLoadLock,
                          WrVirtualMemory,
                          KernelMode,
                          FALSE,
                          NULL);

    _SEH2_TRY
    {
        ULONG RequestedRules = (Size == sizeof(ULONG)) ? *(PULONG)Information : (ULONG)*(PULONGLONG)Information;

        // We can only change dynamic rules at run-time. Force all the others
        // to be unchanged.
        ULONG NewRulesEnable = RequestedRules & DRIVER_VERIFIER_DYNAMIC_RULES;
        ULONG NewRulesDisable = (~RequestedRules) & DRIVER_VERIFIER_DYNAMIC_RULES;
        ULONG NewRules = (VfpGlobalData.Level & ~NewRulesDisable) | NewRulesEnable;

       DbgPrint("VfSetSystemInformation: Rule change 0x%08lx, 0x%08lx -> 0x%08lx\n",
            RequestedRules, VfpGlobalData.Level, NewRules);

        VfpGlobalData.Level = NewRules;

        if (Size == sizeof(ULONG))
        {
            *(PULONG)Information = (ULONG)NewRules;
        }
        else
        {
            *(PULONGLONG)Information = NewRules;
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    KeReleaseMutant(&MmSystemLoadLock, MUTANT_INCREMENT, FALSE, FALSE);

    KeLeaveCriticalRegionThread(CurrentThread);

    return Status;
}



CODE_SEG("VERIFY")
VOID
VfInsertRequestedDriverList(PVERIFIER_DRIVER_ENTRY Entry)
{
    KIRQL PrevIrql;
    ExAcquireSpinLock(&VfRequestedDriverListLock, &PrevIrql);
    InsertTailList(&VfRequestedDriverList, &Entry->Links);
    ExReleaseSpinLock(&VfRequestedDriverListLock, PrevIrql);
}


CODE_SEG("VERIFY")
VOID
VfInitializeDriverEntry(
    IN PVERIFIER_DRIVER_ENTRY DriverEntry
)
{
    DriverEntry->Flags = 0;
    DriverEntry->Loads = 0;
    DriverEntry->Unloads = 0;
    DriverEntry->CurrentPagedPoolAllocations = 0;
    DriverEntry->CurrentNonPagedPoolAllocations = 0;
    DriverEntry->PeakPagedPoolAllocations = 0;
    DriverEntry->PeakNonPagedPoolAllocations = 0;
    DriverEntry->PagedPoolUsageInBytes = 0;
    DriverEntry->NonPagedPoolUsageInBytes = 0;
    DriverEntry->PeakPagedPoolUsageInBytes = 0;
    DriverEntry->PeakNonPagedPoolUsageInBytes = 0;
    DriverEntry->PeakNonPagedPoolUsageInBytes = 0;
}



extern ULONG InitSafeBootMode;

CODE_SEG("INIT")
VOID
VfInitialize(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
)
{
    PVERIFIER_DRIVER_ENTRY DllEntry;
    PVERIFIER_DRIVER_ENTRY KernelEntry;
    PVERIFIER_DRIVER_ENTRY HalEntry;
    UNICODE_STRING KernelName;
    UNICODE_STRING HalName;

    DbgPrint("VfInitialize\n");

    InitializeListHead(&VfThunkedDriverList);
    KeInitializeSpinLock(&VfRequestedDriverListLock);
    InitializeListHead(&VfRequestedDriverList);

    // Turn of verification in safe boot mode
    if (InitSafeBootMode)
    {
        DbgPrint("VfInitialize: Safe boot mode\n");
        MiVerifyAllDrivers = 0;
        MmVerifyDriverLevel = 0;
        MmDontVerifyRandomDrivers = TRUE;
    }

    // No drivers spoecified
    if (MmVerifyDriverBufferLength == (ULONG)-1)
    {
        if (MmDontVerifyRandomDrivers)
        {
            return;
        }

        // FIXME
        DbgPrint("VfInitialize: Random driver verification NYI\n");
        return;
    }

    // Decide which rule set we are using.
    if (MmVerifyDriverLevel == (ULONG)-1)
    {
        MmVerifierData.Level = DRIVER_VERIFIER_DEFAULT_RULES & DRIVER_VERIFIER_ACCEPTED_RULES;
    }
    else
    {
        MmVerifierData.Level = MmVerifyDriverLevel & DRIVER_VERIFIER_ACCEPTED_RULES;
    }
    VfpValidateRules(&MmVerifierData.Level);

    DllEntry = NULL;
    KernelEntry = NULL;
    HalEntry = NULL;

    RtlInitUnicodeString(&KernelName, (PWCHAR)L"ntoskrnl.exe");
    RtlInitUnicodeString(&HalName, (PWCHAR)L"hal.dll");

    if (MmDontVerifyRandomDrivers)
    {
        PWCHAR BufStart = MmVerifyDriverBuffer;
        PWCHAR BufEnd = MmVerifyDriverBuffer + MmVerifyDriverBufferLength / sizeof(WCHAR) - 1;

        if (*BufStart == L'*')
        {
            MiVerifyAllDrivers = TRUE;
        }
        else
        {
            while (BufStart < BufEnd)
            {
                PWCHAR BufPos;
                ULONG Length;
                for (BufPos = BufStart, Length = 0; BufPos < BufEnd; ++BufPos, ++Length)
                {
                    if (*BufPos == L' ')
                    {
                        break;
                    }
                }

                DllEntry = (PVERIFIER_DRIVER_ENTRY)ExAllocatePoolWithTag(
                                NonPagedPool,
                                sizeof(VERIFIER_DRIVER_ENTRY) + Length * sizeof(WCHAR), 
                                TAG_VF_DRIVER_ENTRY);

                VfInitializeDriverEntry(DllEntry);

                DllEntry->DriverName.Buffer = (PWSTR)((PCHAR)DllEntry + sizeof(*DllEntry));
                DllEntry->DriverName.Length = Length * sizeof(WCHAR);
                DllEntry->DriverName.MaximumLength = Length;
                RtlCopyMemory(DllEntry->DriverName.Buffer, BufStart, Length * sizeof(WCHAR));

                if (RtlEqualUnicodeString(&DllEntry->DriverName, &KernelName, TRUE))
                {
                    KfVerifyKernel = TRUE;
                    KernelEntry = DllEntry;
                }
                else if (RtlEqualUnicodeString(&DllEntry->DriverName, &HalName, TRUE))
                {
                    KfVerifyHal = TRUE;
                    HalEntry = DllEntry;
                }

                DllEntry->Flags |= VF_DRIVER_VERIFYING;
                ++DllEntry->Loads;
                VfInsertRequestedDriverList(DllEntry);

                BufStart = BufPos + 1;
            }
        }
    }
}


