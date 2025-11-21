#include "vrfsys.h"
#include "utils.h"
#include "resource.h"

#undef WIN32_NO_STATUS
#include <winnt.h>
#include <ntverp.h>
#include <winternl.h>

#ifdef __REACTOS__
#define NTOS_MODE_USER
// #include <ndk/extypes.h>

// FIXME A consequence of the fix to include the correct winternl.h is that
// not all status codes are included.
#define STATUS_SUCCESS                  (NTSTATUS)0x00000000
#define NT_SUCCESS(x) ( (NTSTATUS)(x)>=0 )
#endif

// FIXME: This stuff should be cribbed from extypes.h, but there are problems
// including that at the moment.
#include <ntstatus.h>

#define SystemVerifierInformation ((SYSTEM_INFORMATION_CLASS)51)

typedef struct _SYSTEM_VERIFIER_INFORMATION
{
    ULONG NextEntryOffset;
    ULONG Level;
    UNICODE_STRING DriverName;
    ULONG RaiseIrqls;
    ULONG AcquireSpinLocks;
    ULONG SynchronizeExecutions;
    ULONG AllocationsAttempted;
    ULONG AllocationsSucceeded;
    ULONG AllocationsSucceededSpecialPool;
    ULONG AllocationsWithNoTag;
    ULONG TrimRequests;
    ULONG Trims;
    ULONG AllocationsFailed;
    ULONG AllocationsFailedDeliberately;
    ULONG Loads;
    ULONG Unloads;
    ULONG UnTrackedPool;
    ULONG CurrentPagedPoolAllocations;
    ULONG CurrentNonPagedPoolAllocations;
    ULONG PeakPagedPoolAllocations;
    ULONG PeakNonPagedPoolAllocations;
    SIZE_T PagedPoolUsageInBytes;
    SIZE_T NonPagedPoolUsageInBytes;
    SIZE_T PeakPagedPoolUsageInBytes;
    SIZE_T PeakNonPagedPoolUsageInBytes;
} SYSTEM_VERIFIER_INFORMATION, *PSYSTEM_VERIFIER_INFORMATION;

// FIXME: Expose these in a better way
#define DRIVER_VERIFIER_DEADLOCK_DETECTION                      0x00000020
#define DRIVER_VERIFIER_ENHANCED_IO_CHECKING                    0x00000040
#define DRIVER_VERIFIER_DMA_VERIFICATION                        0x00000080
#define DRIVER_VERIFIER_SECURITY_CHECKS                         0x00000100
#define DRIVER_VERIFIER_FORCE_PENDING_IO_REQUESTS               0x00000200
#define DRIVER_VERIFIER_RIP_LOGGING                             0x00000400
#define DRIVER_VERIFIER_MISCELLANEOUS_CHECKS                    0x00000800
#define DRIVER_VERIFIER_INVARIANT_MDL_CHECKING_STACK            0x00002000
#define DRIVER_VERIFIER_INVARIANT_MDL_CHECKING_DRIVER           0x00004000
#define DRIVER_VERIFIER_POWER_FRAMEWORK_DELAY_FUZZING           0x00008000
#define DRIVER_VERIFIER_PORT_INTERFACE_CHECKING                 0x00010000
#define DRIVER_VERIFIER_DDI_COMPLIANCE_CHECKING                 0x00020000
#define DRIVER_VERIFIER_INJECT_ALLOCATION_FAILURES_SYSTEMATIC   0x00040000
#define DRIVER_VERIFIER_DDI_COMPLIANCE_CHECKING_ADDITIONAL      0x00080000
#define DRIVER_VERIFIER_NDIS_WIFI_VERIFICATION                  0x00200000
#define DRIVER_VERIFIER_KERNEL_SYNCHRONIZATION_DELAY_FUZZING    0x00800000
#define DRIVER_VERIFIER_KERNEL_VM_SWITCH_VERIFICATION           0x01000000


static const TCHAR MmRegKeyName[] = _T("System\\CurrentControlSet\\Control\\Session Manager\\Memory Management");

const TCHAR VfDriverLevelRegName[] = _T("VerifyDriverLevel");

const TCHAR VfDriversRegName[] = _T("VerifyDrivers");


BOOL VrfyReset()
{
    HKEY hMmKey = NULL;
    LONG lRes;
    BOOL bResult = FALSE;

    lRes = RegOpenKeyEx(HKEY_LOCAL_MACHINE, MmRegKeyName, 0, KEY_SET_VALUE, &hMmKey);

    if (lRes == ERROR_ACCESS_DENIED)
    {
        VrfyErrorPrint(IDS_ACCESS_DENIED);
        goto done;
    }
    else if (lRes != ERROR_SUCCESS)
    {
        VrfyErrorPrintf(IDS_OPEN_KEY_FAILED, MmRegKeyName, (DWORD)lRes);
        goto done;
    }

    lRes = RegDeleteValue(hMmKey, VfDriverLevelRegName);
    if (lRes != ERROR_SUCCESS && lRes != ERROR_FILE_NOT_FOUND)
    {
        VrfyErrorPrintf(IDS_DELETE_VALUE_FAILED, VfDriverLevelRegName, (DWORD)lRes);
        goto done;
    }

    lRes = RegDeleteValue(hMmKey, VfDriversRegName);
    if (lRes != ERROR_SUCCESS && lRes != ERROR_FILE_NOT_FOUND)
    {
        VrfyErrorPrintf(IDS_DELETE_VALUE_FAILED, VfDriverLevelRegName, (DWORD)lRes);
        goto done;
    }

    bResult = TRUE;

done:
    if (hMmKey)
    {
        RegCloseKey(hMmKey);
    }

    return bResult;
}



void VrfyQueryToFile(FILE* out)
{
    ULONG lBufferSize = 1024, lFoundLength;
    PVOID Buffer = NULL;
    PSYSTEM_VERIFIER_INFORMATION Information;
    NTSTATUS Status = STATUS_SUCCESS;

    for (;;)
    {
        Buffer = malloc(lBufferSize);
        if (!Buffer)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        Status = NtQuerySystemInformation(SystemVerifierInformation, Buffer, lBufferSize, &lFoundLength);
        if (Status != STATUS_INFO_LENGTH_MISMATCH)
        {
            break;
        }

        free(Buffer);
        Buffer = NULL;
        lBufferSize += 1024;
    }

    _ftprintf(out, _T("NtQuerySystemInformation Status = 0x%08ull, lBufferSize = %d, lFoundLength = %d\n"), Status, lBufferSize, lFoundLength);

    if (!NT_SUCCESS(Status))
    {
        VrfyErrorPrintf(IDS_QUERY_SYSTEM_INFO_FAILED, Status);
        goto done;
    }

    if (!lFoundLength)
    {
        goto done;
    }

    for (ULONG i = 0; i < lFoundLength; i += 8)
    {
        ULONG LineLength = lFoundLength - i;
        ULONG j;

        if (LineLength > 8) LineLength = 8;
        for (j = 0; j < LineLength; ++j)
        {
            _ftprintf(out, _T(" %02x"), (UCHAR)*((PCHAR)Buffer + i + j));
        }
        for (; j < 8; ++j)
        {
            _ftprintf(out, _T("   "));
        }
        _ftprintf(out, _T(" "));
        for (j = 0; j < LineLength; ++j)
        {
            CHAR c = *((PCHAR)Buffer + i + j);
            _ftprintf(out, _T("%c"), isprint(c) ? c : (TCHAR)'.');
        }
        _ftprintf(out, _T("\n"));
    }

    _ftprintf(out, _T("\n"));

    Information = (PSYSTEM_VERIFIER_INFORMATION)Buffer;

    _ftprintf(out, _T("Verify level: 0x%08ul\n"), Information->Level);
    _ftprintf(out, _T("Deadlock detection:                    %s\n"), (Information->Level & DRIVER_VERIFIER_DEADLOCK_DETECTION) ? _T("true") : _T("false"));
    _ftprintf(out, _T("Enhanced I/O checking:                 %s\n"), (Information->Level & DRIVER_VERIFIER_ENHANCED_IO_CHECKING) ? _T("true") : _T("false"));
    _ftprintf(out, _T("DMA verification:                      %s\n"), (Information->Level & DRIVER_VERIFIER_DMA_VERIFICATION) ? _T("true") : _T("false"));
    _ftprintf(out, _T("Security checks:                       %s\n"), (Information->Level & DRIVER_VERIFIER_SECURITY_CHECKS) ? _T("true") : _T("false"));
    _ftprintf(out, _T("Force pending I/O requests:            %s\n"), (Information->Level & DRIVER_VERIFIER_FORCE_PENDING_IO_REQUESTS) ? _T("true") : _T("false"));
    _ftprintf(out, _T("RIP logging:                           %s\n"), (Information->Level & DRIVER_VERIFIER_RIP_LOGGING) ? _T("true") : _T("false"));
    _ftprintf(out, _T("Miscellaneous checks:                  %s\n"), (Information->Level & DRIVER_VERIFIER_MISCELLANEOUS_CHECKS) ? _T("true") : _T("false"));
    _ftprintf(out, _T("Invariant MDL checking stack:          %s\n"), (Information->Level & DRIVER_VERIFIER_INVARIANT_MDL_CHECKING_STACK) ? _T("true") : _T("false"));
    _ftprintf(out, _T("Invariant MDL checking driver:         %s\n"), (Information->Level & DRIVER_VERIFIER_INVARIANT_MDL_CHECKING_DRIVER) ? _T("true") : _T("false"));
    _ftprintf(out, _T("Power framework delay fuzzing:         %s\n"), (Information->Level & DRIVER_VERIFIER_POWER_FRAMEWORK_DELAY_FUZZING) ? _T("true") : _T("false"));
    _ftprintf(out, _T("Port interface checking:               %s\n"), (Information->Level & DRIVER_VERIFIER_PORT_INTERFACE_CHECKING) ? _T("true") : _T("false"));
    _ftprintf(out, _T("DDI compliance checking:               %s\n"), (Information->Level & DRIVER_VERIFIER_DDI_COMPLIANCE_CHECKING) ? _T("true") : _T("false"));
    _ftprintf(out, _T("Inject allocation failures systematic: %s\n"), (Information->Level & DRIVER_VERIFIER_INJECT_ALLOCATION_FAILURES_SYSTEMATIC) ? _T("true") : _T("false"));
    _ftprintf(out, _T("Additional DDI compliance checking:    %s\n"), (Information->Level & DRIVER_VERIFIER_DDI_COMPLIANCE_CHECKING_ADDITIONAL) ? _T("true") : _T("false"));
    _ftprintf(out, _T("NDIS WiFi verification:                %s\n"), (Information->Level & DRIVER_VERIFIER_NDIS_WIFI_VERIFICATION) ? _T("true") : _T("false"));
    _ftprintf(out, _T("Kernel synchronization delay fuzzing:  %s\n"), (Information->Level & DRIVER_VERIFIER_KERNEL_SYNCHRONIZATION_DELAY_FUZZING) ? _T("true") : _T("false"));
    _ftprintf(out, _T("Kernel VM switch verification:         %s\n"), (Information->Level & DRIVER_VERIFIER_KERNEL_VM_SWITCH_VERIFICATION) ? _T("true") : _T("false"));
    _ftprintf(out, _T("RaiseIrqls                      %u\n"), Information->RaiseIrqls);
    _ftprintf(out, _T("AcquireSpinLocks                %u\n"), Information->AcquireSpinLocks);
    _ftprintf(out, _T("SynchronizeExecutions           %u\n"), Information->SynchronizeExecutions);
    _ftprintf(out, _T("AllocationsAttempted            %u\n"), Information->AllocationsAttempted);
    _ftprintf(out, _T("AllocationsSucceeded            %u\n"), Information->AllocationsSucceeded);
    _ftprintf(out, _T("AllocationsSucceededSpecialPool %u\n"), Information->AllocationsSucceededSpecialPool);
    _ftprintf(out, _T("AllocationsWithNoTag            %u\n"), Information->AllocationsWithNoTag);
    _ftprintf(out, _T("Trims                           %u\n"), Information->Trims);
    _ftprintf(out, _T("AllocationsFailed               %u\n"), Information->AllocationsFailed);
    _ftprintf(out, _T("AllocationsFailedDeliberately   %u\n"), Information->AllocationsFailedDeliberately);
    _ftprintf(out, _T("UnTrackedPool                   %u\n"), Information->UnTrackedPool);

    for (;;)
    {
        _ftprintf(out, _T("Driver \"%.*lS\"\n"), Information->DriverName.Length, Information->DriverName.Buffer);

        _ftprintf(out, _T("Loads                          %u\n"), Information->Loads);
        _ftprintf(out, _T("Unloads                        %u\n"), Information->Unloads);
        _ftprintf(out, _T("CurrentPagedPoolAllocations    %u\n"), Information->CurrentPagedPoolAllocations);
        _ftprintf(out, _T("CurrentNonPagedPoolAllocations %u\n"), Information->CurrentNonPagedPoolAllocations);
        _ftprintf(out, _T("PeakPagedPoolAllocations       %u\n"), Information->PeakPagedPoolAllocations);
        _ftprintf(out, _T("PeakNonPagedPoolAllocations    %u\n"), Information->PeakNonPagedPoolAllocations);
        _ftprintf(out, _T("PagedPoolUsageInBytes          %Iu\n"), Information->PagedPoolUsageInBytes);
        _ftprintf(out, _T("NonPagedPoolUsageInBytes       %Iu\n"), Information->NonPagedPoolUsageInBytes);
        _ftprintf(out, _T("PeakPagedPoolUsageInBytes      %Iu\n"), Information->PeakPagedPoolUsageInBytes);
        _ftprintf(out, _T("PeakNonPagedPoolUsageInBytes   %Iu\n"), Information->PeakNonPagedPoolUsageInBytes);

        if (!Information->NextEntryOffset)
        {
            break;
        }
        Information = (PSYSTEM_VERIFIER_INFORMATION)((PCHAR)Information + Information->NextEntryOffset);
    }

done:
    if (Buffer)
    {
        free(Buffer);
    }
}
