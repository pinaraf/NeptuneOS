#pragma once

#include <ntos.h>
#include <ntimage.h>

#define PspAllocatePool(Var, Type)					\
    ExAllocatePoolEx(Var, Type, sizeof(Type), NTOS_PS_TAG, {})
#define PspAllocateArray(Var, Type, Size)				\
    ExAllocatePoolEx(Var, Type, sizeof(Type) * (Size), NTOS_PS_TAG, {})

/* Unsafe: no validation. Must be called after image file is fully mapped
 * into server address space and validated to be a PE image. */
static inline PIMAGE_NT_HEADERS PspImageNtHeader(IN PVOID FileBuffer)
{
    PIMAGE_DOS_HEADER DosHeader = (PIMAGE_DOS_HEADER) FileBuffer;
    return (PIMAGE_NT_HEADERS)((MWORD) FileBuffer + DosHeader->e_lfanew);
}

#define PROCESS_HEAP_DEFAULT_RESERVE	(64 * PAGE_SIZE)
#define PROCESS_HEAP_DEFAULT_COMMIT	(PAGE_SIZE)

/* arch/context.c */
VOID PspInitializeThreadContext(IN PTHREAD Thread,
				IN PTHREAD_CONTEXT Context);

/* create.c */
NTSTATUS PspThreadObjectCreateProc(POBJECT Object);
NTSTATUS PspProcessObjectCreateProc(POBJECT Object);

/* init.c */
extern LIST_ENTRY PspProcessList;
extern PSECTION PspSystemDllSection;
extern PSUBSECTION PspSystemDllTlsSubsection;
extern PMMVAD PspUserSharedDataVad;
