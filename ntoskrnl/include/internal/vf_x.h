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


