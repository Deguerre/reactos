/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/include/internal/vf.h
 * PURPOSE:         Internal header for the verifier
 * PROGRAMMERS:     Andrew J. Bromage (Deguerre@users.noreply.github.com)
 */


//
// Verifier stuff should be together
//
#ifdef _MSC_VER
#pragma section("VERIFY", read,execute)
#pragma section("VERIFYCONST", read)
#pragma section("VERIFYDATA", read,write)
#pragma section("VERIFYBSS", read,write)
#endif



#ifdef __cplusplus
extern "C" {
#endif

//
// Data collected globally.
//
// See also: MM_DRIVER_VERIFIER_DATA
//
typedef struct _VERIFIER_DATA_GLOBAL
{
    ULONG Level;
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
    ULONG UnTrackedPool;
} VERIFIER_DATA_GLOBAL, *PVERIFIER_DATA_GLOBAL;


//
// Data collected per driver
//
// See also: MM_DRIVER_VERIFIER_DATA
//
typedef struct _VERIFIER_DRIVER_ENTRY
{
    LIST_ENTRY Links;
    UNICODE_STRING DriverName;

#define VF_DRIVER_VERIFYING    0x0001
    ULONG Flags;
    ULONG Loads;
    ULONG Unloads;
    ULONG CurrentPagedPoolAllocations;
    ULONG CurrentNonPagedPoolAllocations;
    ULONG PeakPagedPoolAllocations;
    ULONG PeakNonPagedPoolAllocations;
    SIZE_T PagedPoolUsageInBytes;
    SIZE_T NonPagedPoolUsageInBytes;
    SIZE_T PeakPagedPoolUsageInBytes;
    SIZE_T PeakNonPagedPoolUsageInBytes;
} VERIFIER_DRIVER_ENTRY, *PVERIFIER_DRIVER_ENTRY;


//
// The type for an override
//
typedef struct _VERIFIER_HOOK
{
    PCHAR OriginalRoutineName;
    PVOID OriginalRoutine;
    PVOID NewRoutine;
} VERIFIER_HOOK, *PVERIFIER_HOOK;


//
// Rule classes not exposed in ntddk.h
//
#define DRIVER_VERIFIER_DEADLOCK_DETECTION                      0x00000020
#define DRIVER_VERIFIER_ENHANCED_IO_CHECKING                    0x00000040
#define DRIVER_VERIFIER_DMA_VERIFICATION                        0x00000080
#define DRIVER_VERIFIER_SECURITY_CHECKS                         0x00000100
#define DRIVER_VERIFIER_FORCE_PENDING_IO_REQUESTS               0x00000200
#define DRIVER_VERIFIER_RIP_LOGGING                             0x00000400
#define DRIVER_VERIFIER_MISCELLANEOUS_CHECKS                    0x00000800
#define DRIVER_VERIFIER_INVARIANT_MDL_CHECKING_STACK            0x00002000  // WIN8
#define DRIVER_VERIFIER_INVARIANT_MDL_CHECKING_DRIVER           0x00004000  // WIN8
#define DRIVER_VERIFIER_POWER_FRAMEWORK_DELAY_FUZZING           0x00008000  // WIN8, deprecated Win10 build 19042
// #define DRIVER_VERIFIER_PORT_INTERFACE_CHECKING              0x00010000  // WIN10
#define DRIVER_VERIFIER_DDI_COMPLIANCE_CHECKING                 0x00020000  // WIN8
#define DRIVER_VERIFIER_INJECT_ALLOCATION_FAILURES_SYSTEMATIC   0x00040000  // WIN8, deprecated Win10 build 19042
#define DRIVER_VERIFIER_DDI_COMPLIANCE_CHECKING_ADDITIONAL      0x00080000  // WIN8, deprecated Win10 build 19042
#define DRIVER_VERIFIER_NDIS_WIFI_VERIFICATION                  0x00200000  // WINBLUE
#define DRIVER_VERIFIER_KERNEL_SYNCHRONIZATION_DELAY_FUZZING    0x00800000  // WINBLUE, deprecated Win10 build 19042
#define DRIVER_VERIFIER_KERNEL_VM_SWITCH_VERIFICATION           0x01000000  // WINBLUE
// #define DRIVER_VERIFIER_CODE_INTEGRITY_CHECKS                0x0000000002000000  // WIN10
// #define DRIVER_VERIFIER_DRIVER_ISOLATION_CHECKS              0x0000000200000000  // WIN10
// #define DRIVER_VERIFIER_WDF_VERIFICATION                     0x0000000400000000  // WIN10
// #define DRIVER_VERIFIER_ADDITIONAL_IRQL_CHECKING             0x0000001000000000  // WIN10
// #define DRIVER_VERIFIER_ENABLE_DIF                           0x0000002000000000  // WIN10


//
// Rules which are "standard".
//
#define DRIVER_VERIFIER_STANDARD_RULES (\
    DRIVER_VERIFIER_SPECIAL_POOLING | \
    DRIVER_VERIFIER_FORCE_IRQL_CHECKING | \
    DRIVER_VERIFIER_TRACK_POOL_ALLOCATIONS | \
    DRIVER_VERIFIER_IO_CHECKING | \
    DRIVER_VERIFIER_DEADLOCK_DETECTION | \
    DRIVER_VERIFIER_DMA_VERIFICATION | \
    DRIVER_VERIFIER_SECURITY_CHECKS | \
    DRIVER_VERIFIER_MISCELLANEOUS_CHECKS)

//
// Default ruleset if the specified level is (ULONG)-1.
//
#define DRIVER_VERIFIER_DEFAULT_RULES (\
    DRIVER_VERIFIER_SPECIAL_POOLING | \
    DRIVER_VERIFIER_FORCE_IRQL_CHECKING | \
    DRIVER_VERIFIER_TRACK_POOL_ALLOCATIONS)

//
// Rules which can be modified without rebooting.
//
#define DRIVER_VERIFIER_DYNAMIC_RULES (\
    DRIVER_VERIFIER_SPECIAL_POOLING | \
    DRIVER_VERIFIER_FORCE_IRQL_CHECKING | \
    DRIVER_VERIFIER_INJECT_ALLOCATION_FAILURES)

//
// Rules which are deprecated as of Windows 10.
// (No rules were deprecated before that.)
//
#define DRIVER_VERIFIER_DEPRECATED_RULES (\
    DRIVER_VERIFIER_POWER_FRAMEWORK_DELAY_FUZZING | \
    DRIVER_VERIFIER_INJECT_ALLOCATION_FAILURES_SYSTEMATIC | \
    DRIVER_VERIFIER_DDI_COMPLIANCE_CHECKING_ADDITIONAL | \
    DRIVER_VERIFIER_KERNEL_SYNCHRONIZATION_DELAY_FUZZING)

//
// Rules which ReactOS understands.
//
#define DRIVER_VERIFIER_ACCEPTED_RULES (\
    DRIVER_VERIFIER_SPECIAL_POOLING | \
    DRIVER_VERIFIER_FORCE_IRQL_CHECKING | \
    DRIVER_VERIFIER_INJECT_ALLOCATION_FAILURES | \
    DRIVER_VERIFIER_TRACK_POOL_ALLOCATIONS | \
    DRIVER_VERIFIER_IO_CHECKING | \
    DRIVER_VERIFIER_DEADLOCK_DETECTION | \
    DRIVER_VERIFIER_ENHANCED_IO_CHECKING | \
    DRIVER_VERIFIER_DMA_VERIFICATION | \
    DRIVER_VERIFIER_SECURITY_CHECKS | \
    DRIVER_VERIFIER_FORCE_PENDING_IO_REQUESTS | \
    DRIVER_VERIFIER_RIP_LOGGING | \
    DRIVER_VERIFIER_MISCELLANEOUS_CHECKS)


extern VERIFIER_DATA_GLOBAL VfpGlobalData;


#define VF_VERIFY_DRIVER_BUFFER_LENGTH 512

//
// These globals are in Mm for historical reasons.
//
extern ULONG MmVerifyDriverBufferType;
extern ULONG MmVerifyDriverLevel;
extern WCHAR MmVerifyDriverBuffer[];
extern ULONG MmVerifyDriverBufferLength;
extern LIST_ENTRY VfThunkedDriverList;
extern ULONG MiActiveVerifierThunks;
extern LOGICAL MmDontVerifyRandomDrivers;
extern LOGICAL MiVerifyAllDrivers;


//
// Initialization function
//
CODE_SEG("INIT")
VOID
VfInitialize(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
);


//
// Test to see if a driver is verifying.
//
BOOLEAN
VfpIsDriverVerifying (
    IN PDRIVER_OBJECT DriverObject
);



NTSTATUS
VfGetSystemInformation(
    IN PSYSTEM_VERIFIER_INFORMATION Information,
    IN ULONG Size,
    OUT PULONG ReqSize
);



NTSTATUS
VfSetSystemInformation(
    IN PVOID Information,
    IN ULONG Size
);



NTSTATUS
VfAddDriverEntry(
    IN PUNICODE_STRING DriverName
);



NTSTATUS
VfRemoveDriverEntry(
    IN PUNICODE_STRING DriverName
);


NTSTATUS
NTAPI
MmAddVerifierThunks(
    IN PVOID ThunkBuffer,
    IN ULONG ThunkBufferSize
);


LOGICAL
NTAPI
MmIsDriverVerifying(IN PDRIVER_OBJECT DriverObject);


NTSTATUS
NTAPI
MmIsVerifierEnabled(OUT PULONG VerifierFlags);


PLDR_DATA_TABLE_ENTRY
NTAPI
MiLookupDataTableEntry(IN PVOID Address);


#ifdef __cplusplus
}
#endif

//
// Inlined functions
//
#include "vf_x.h"
