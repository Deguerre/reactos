/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/vf/driver.c
 * PURPOSE:         Driver Verifier Device Driver Interface
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#undef NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/


/*
 * @implemented
 */
NTSTATUS
NTAPI
MmAddVerifierThunks(
    IN PVOID ThunkBuffer,
    IN ULONG ThunkBufferSize
)
{
    PKTHREAD CurrentThread;
    PDRIVER_VERIFIER_THUNK_PAIRS ThunkTable;
    ULONG ThunkCount;
    PDRIVER_SPECIFIED_VERIFIER_THUNKS ThunkedDriver;
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    PVOID ModuleBase, ModuleEnd;
    ULONG i;
    NTSTATUS Status = STATUS_SUCCESS;
    PLIST_ENTRY Entry;

    PAGED_CODE();

    //
    // Make sure the driver verifier is initialized
    //
    if (!VfIsVerifierOn())
    {
        return STATUS_NOT_SUPPORTED;
    }

    //
    // Get the thunk pairs and count them
    //
    ThunkCount = ThunkBufferSize / sizeof(DRIVER_VERIFIER_THUNK_PAIRS);
    if (!ThunkCount)
    {
        return STATUS_INVALID_PARAMETER_1;
    }

    //
    // Now allocate our own thunk table
    //
    ThunkedDriver = ExAllocatePoolWithTag(PagedPool,
                                         sizeof(*ThunkedDriver) +
                                         ThunkCount *
                                         sizeof(DRIVER_VERIFIER_THUNK_PAIRS),
                                         TAG_VF_THUNKS);
    if (!ThunkedDriver)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Now copy the driver-fed part
    //
    ThunkTable = (PDRIVER_VERIFIER_THUNK_PAIRS)(ThunkedDriver + 1);
    RtlCopyMemory(ThunkTable,
                  ThunkBuffer,
                  ThunkCount * sizeof(DRIVER_VERIFIER_THUNK_PAIRS));

    //
    // Acquire the system load lock
    //
    CurrentThread = KeGetCurrentThread();

    KeEnterCriticalRegionThread(CurrentThread);
    KeWaitForSingleObject(&MmSystemLoadLock,
                          WrVirtualMemory,
                          KernelMode,
                          FALSE,
                          NULL);

    //
    // Get the loader entry
    //
    LdrEntry = MiLookupDataTableEntry(ThunkTable->PristineRoutine);
    if (!LdrEntry)
    {
        //
        // Fail
        //
        Status = STATUS_INVALID_PARAMETER_2;
        goto Cleanup;
    }

    //
    // Get driver base and end
    //
    ModuleBase = LdrEntry->DllBase;
    ModuleEnd = (PVOID)((ULONG_PTR)LdrEntry->DllBase + LdrEntry->SizeOfImage);

    //
    // Don't allow hooking ntoskrnl.exe or hal.dll, which should always be the
    // first two modules in load order.
    //
    for (i = 0, Entry = PsLoadedModuleList.Flink; Entry != &PsLoadedModuleList; Entry = Entry->Flink)
    {
        PLDR_DATA_TABLE_ENTRY ModuleLdrEntry;
        ModuleLdrEntry = CONTAINING_RECORD(Entry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
        if (LdrEntry == ModuleLdrEntry)
        {
            Status = STATUS_INVALID_PARAMETER_2;
            goto Cleanup;
        }
        if (++i >= 2) break;
    }

    //
    // Loop all the thunks
    //
    for (i = 0; i < ThunkCount; ++i)
    {
        //
        // Make sure both routines are in the same driver.
        //
        if (((ULONG_PTR)ThunkTable->PristineRoutine < (ULONG_PTR)ModuleBase) ||
            ((ULONG_PTR)ThunkTable->PristineRoutine >= (ULONG_PTR)ModuleEnd) ||
            ((ULONG_PTR)ThunkTable->NewRoutine < (ULONG_PTR)ModuleBase) ||
            ((ULONG_PTR)ThunkTable->NewRoutine >= (ULONG_PTR)ModuleEnd))
        {
            //
            // Nope, fail
            //
            Status = STATUS_INVALID_PARAMETER_2;
            goto Cleanup;
        }
    }

    //
    // Otherwise, add this entry
    //
    ThunkedDriver->DataTableEntry = LdrEntry;
    ThunkedDriver->NumberOfThunks = ThunkCount;
    MiActiveVerifierThunks++;
    InsertTailList(&VfThunkedDriverList, &ThunkedDriver->ListEntry);
    ThunkedDriver = NULL;

Cleanup:
    //
    // Release the lock
    //
    KeReleaseMutant(&MmSystemLoadLock, MUTANT_INCREMENT, FALSE, FALSE);
    KeLeaveCriticalRegionThread(CurrentThread);

    //
    // Free the table if we failed and return status
    //
    if (ThunkedDriver)
    {
        ExFreePoolWithTag(ThunkedDriver, TAG_VF_THUNKS);
    }

    return Status;
}


/*
 * @implemented
 */
LOGICAL
NTAPI
MmIsDriverVerifying(IN PDRIVER_OBJECT DriverObject)
{
    PLDR_DATA_TABLE_ENTRY LdrEntry;

    //
    // Get the loader entry
    //
    LdrEntry = (PLDR_DATA_TABLE_ENTRY)DriverObject->DriverSection;
    if (!LdrEntry)
    {
        return FALSE;
    }

    //
    // Check if we're verifying or not
    //
    return (LdrEntry->Flags & LDRP_IMAGE_VERIFYING) ? TRUE: FALSE;
}


/*
 * @implemented
 */
NTSTATUS
NTAPI
MmIsVerifierEnabled(OUT PULONG VerifierFlags)
{
    //
    // Check if we've actually added anything to the list
    //
    if (VfThunkedDriverList.Flink)
    {
        //
        // We have, read the verifier level
        //
        *VerifierFlags = MmVerifierData.Level;
        return STATUS_SUCCESS;
    }

    //
    // Otherwise, we're disabled
    //
    *VerifierFlags = 0;
    return STATUS_NOT_SUPPORTED;
}



BOOLEAN
NTAPI
VfIsVerificationEnabled(IN VF_OBJECT_TYPE VfObjectType,
                        IN PVOID Object OPTIONAL)
{
    if (!VfIsVerifierOn())
    {
        return FALSE;
    }

    switch (VfObjectType)
    {
        case VFOBJTYPE_DRIVER:
            return MmIsDriverVerifying((PDRIVER_OBJECT)Object);

        case VFOBJTYPE_DEVICE:
            return FALSE;

        case VFOBJTYPE_SYSTEM_BIOS:
            return FALSE;

        default:
            ExRaiseStatus(STATUS_INVALID_PARAMETER_1);
    }

    return FALSE;
}

/*
 * @unimplemented
 */
VOID
__cdecl
VfFailDeviceNode(IN PDEVICE_OBJECT PhysicalDeviceObject,
                 IN ULONG BugCheckMajorCode,
                 IN ULONG BugCheckMinorCode,
                 IN VF_FAILURE_CLASS FailureClass,
                 IN OUT PULONG AssertionControl,
                 IN PSTR DebuggerMessageText,
                 IN PSTR ParameterFormatString,
                 ...)
{
    DbgPrint("VfFailDeviceNode: Called\n");
}

/*
 * @unimplemented
 */
VOID
__cdecl
VfFailSystemBIOS(IN ULONG BugCheckMajorCode,
                 IN ULONG BugCheckMinorCode,
                 IN VF_FAILURE_CLASS FailureClass,
                 IN OUT PULONG AssertionControl,
                 IN PSTR DebuggerMessageText,
                 IN PSTR ParameterFormatString,
                 ...)
{
    DbgPrint("VfFailSystemBIOS: Called\n");
}

VOID
__cdecl
VfFailDriver(IN ULONG BugCheckMajorCode,
             IN ULONG BugCheckMinorCode,
             IN VF_FAILURE_CLASS FailureClass,
             IN OUT PULONG AssertionControl,
             IN PSTR DebuggerMessageText,
             IN PSTR ParameterFormatString,
             ...)
{
    DbgPrint("VfFailDriver: Called\n");
}



NTSTATUS
VfAddDriverEntry(
    IN PUNICODE_STRING DriverName
)
{
    // NTSTATUS Status;
    // PKTHREAD CurrentThread;
    // PLIST_ENTRY NextEntry;
    PVERIFIER_DRIVER_ENTRY DriverEntry;
    // PLDR_DATA_TABLE_ENTRY DataTableEntry;

    ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);

    if (!VfIsVerifierOn())
    {
        return STATUS_NOT_SUPPORTED;
    }

    DbgPrint("VfAddDriverEntry: %wZ\n", DriverName);

    DriverEntry = (PVERIFIER_DRIVER_ENTRY)ExAllocatePoolWithTag(
                NonPagedPool,
                sizeof (VERIFIER_DRIVER_ENTRY) + DriverName->MaximumLength,
                TAG_VF_DRIVER_ENTRY);

    if (DriverEntry == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    DriverEntry->DriverName.Buffer = (PWSTR)((PCHAR)DriverEntry + sizeof(VERIFIER_DRIVER_ENTRY));
    DriverEntry->DriverName.Length = DriverName->Length;
    DriverEntry->DriverName.MaximumLength = DriverName->MaximumLength;

    RtlCopyMemory(DriverEntry->DriverName.Buffer, DriverName->Buffer, DriverName->Length);

    // FIXME

    return STATUS_NOT_IMPLEMENTED;
}



NTSTATUS
VfRemoveDriverEntry(
    IN PUNICODE_STRING DriverName
)
{
    ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);

    if (!VfIsVerifierOn())
    {
        return STATUS_NOT_SUPPORTED;
    }

    DbgPrint("VfRemoveDriverEntry: %wZ\n", DriverName);

    return STATUS_NOT_IMPLEMENTED;
}


/* EOF */
