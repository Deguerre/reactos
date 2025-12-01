/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/include/internal/vf_x.h
 * PURPOSE:         Internal inlined functions for the verifier
 * PROGRAMMERS:     Andrew J. Bromage (Deguerre@users.noreply.github.com)
 */


FORCEINLINE
BOOLEAN
NTAPI
VfIsVerifierOn()
{
    return (VfThunkedDriverList.Flink != NULL);
}


// This hash function is designed for very small hash tables, such
// as are found in the verifier (around 32-64 buckets).
FORCEINLINE
ULONG
VfpHashFunction(ULONG Seed, ULONG Value)
{
    ULONG ValueHi = Value >> 16;
    ULONG ValueLo = Value & 0xFFFF;
    ULONG Hash = (0xd991 * ValueHi + 0x6745 * ValueLo + 0xd4e1 * Seed) >> 16;
    return Hash;
}


