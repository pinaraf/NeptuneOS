/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS Run-Time Library
 * PURPOSE:           AMD64 stubs
 * FILE:              lib/rtl/amd64/stubs.c
 * PROGRAMMERS:       Stefan Ginsberg (stefan.ginsberg@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include "../rtlp.h"

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
NTAPI VOID RtlInitializeContext(IN HANDLE ProcessHandle,
				OUT PCONTEXT ThreadContext,
				IN PVOID ThreadStartParam OPTIONAL,
				IN PTHREAD_START_ROUTINE ThreadStartAddress,
				IN PINITIAL_TEB StackBase)
{
    /* Initialize everything to 0 */
    RtlZeroMemory(ThreadContext, sizeof(*ThreadContext));

    /* Initialize StartAddress and Stack */
    ThreadContext->Rip = (ULONG64)ThreadStartAddress;

    /* Enable Interrupts */
    ThreadContext->EFlags = EFLAGS_INTERRUPT_MASK;

    /* Set start parameter */
    ThreadContext->Rcx = (ULONG64)ThreadStartParam;

    /* Initialize user mode segments */
    ThreadContext->SegGs = (ULONG_PTR)NtCurrentTeb();

    /* Only the basic Context is initialized */
    ThreadContext->ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER | CONTEXT_SEGMENTS;

    return;
}
