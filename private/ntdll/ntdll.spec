@ stdcall NtDisplayString(ptr)
@ stdcall NtLoadDriver(ptr)
@ stdcall NtDisplayStringA(ptr)
@ stdcall NtLoadDriverA(ptr)
@ stdcall RtlInitUnicodeString(ptr wstr)
@ stdcall RtlInitUnicodeStringEx(ptr wstr)
@ cdecl memcpy(ptr ptr long)
@ cdecl memmove(ptr ptr long)
@ stdcall RtlCompareMemory(ptr ptr long)
@ stdcall RtlCompareMemoryUlong(ptr long long)
@ stdcall RtlFillMemory(ptr long long)
@ stdcall -arch=i386 RtlFillMemoryUlong(ptr long long)
@ stdcall RtlMoveMemory(ptr ptr long)
@ stdcall RtlZeroMemory(ptr long)
@ stdcall -version=0x600+ RtlTestBit(ptr long)
@ stdcall RtlGetNtGlobalFlags()
@ stdcall RtlSetLastWin32ErrorAndNtStatusFromNtStatus(long)
@ stdcall RtlNtStatusToDosErrorNoTeb(long)
@ stdcall RtlInitializeSListHead(ptr)
@ stdcall RtlPcToFileHeader(ptr ptr)
@ cdecl -arch=x86_64 RtlRestoreContext(ptr ptr)
@ cdecl _vsnprintf(ptr long str ptr) vsnprintf
@ varargs _snprintf(ptr long str) snprintf
@ varargs DbgPrint(str)
@ stdcall LdrGetProcedureAddress(ptr ptr long ptr)
@ stdcall RtlInitAnsiString(ptr str)
@ stdcall RtlInitAnsiStringEx(ptr str)
@ stdcall LdrFindEntryForAddress(ptr ptr)
