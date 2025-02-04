#include <ntdll.h>

/*
* @implemented
*/
NTAPI ULONG RtlGetNtGlobalFlags(VOID)
{
    PPEB pPeb = NtCurrentPeb();
    return pPeb->NtGlobalFlag;
}

BOOLEAN NTAPI RtlpCheckForActiveDebugger(VOID)
{
    /* Return the flag in the PEB */
    return NtCurrentPeb()->BeingDebugged;
}

NTAPI VOID RtlpGetStackLimits(OUT PULONG_PTR LowLimit,
			      OUT PULONG_PTR HighLimit)
{
    *LowLimit = (ULONG_PTR)NtCurrentTeb()->NtTib.StackLimit;
    *HighLimit = (ULONG_PTR)NtCurrentTeb()->NtTib.StackBase;
}

/*
 * @implemented
 */
NTAPI PVOID RtlPcToFileHeader(IN PVOID PcValue,
			      OUT PVOID *BaseOfImage)
{
    PLIST_ENTRY ModuleListHead;
    PLIST_ENTRY Entry;
    PLDR_DATA_TABLE_ENTRY Module;
    PVOID ImageBase = NULL;

    RtlEnterCriticalSection (NtCurrentPeb()->LoaderLock);
    ModuleListHead = &NtCurrentPeb()->LdrData->InLoadOrderModuleList;
    Entry = ModuleListHead->Flink;
    while (Entry != ModuleListHead) {
        Module = CONTAINING_RECORD(Entry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

        if ((ULONG_PTR)PcValue >= (ULONG_PTR)Module->DllBase &&
                (ULONG_PTR)PcValue < (ULONG_PTR)Module->DllBase + Module->SizeOfImage)
        {
            ImageBase = Module->DllBase;
            break;
        }
        Entry = Entry->Flink;
    }
    RtlLeaveCriticalSection(NtCurrentPeb()->LoaderLock);

    *BaseOfImage = ImageBase;
    return ImageBase;
}
