/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/include/internal/vf.h
 * PURPOSE:         Internal header for verifier deadlock detection
 * PROGRAMMERS:     Andrew J. Bromage (Deguerre@users.noreply.github.com)
 */


#ifdef __cplusplus
extern "C" {
#endif

void
VfpInitializeDeadlockDetection();

void
FASTCALL
VffInitializeDeadlockableResource();

void
FASTCALL
VffAcquireDeadlockableResource();

void
FASTCALL
VffReleaseDeadlockableResource();

#ifdef __cplusplus
}
#endif

