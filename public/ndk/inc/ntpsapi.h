/* This file is stolen shamelessly from reactos/sdk/lib/crt/wine/winternl.h */

#pragma once

#include <ldrtypes.h>

/* This is placed at the same address for both i386 and amd64 */
#define USER_SHARED_DATA                        0x7FFE0000

/*
 * Global Flags
 */
#define FLG_STOP_ON_EXCEPTION                   0x00000001
#define FLG_SHOW_LDR_SNAPS                      0x00000002
#define FLG_DEBUG_INITIAL_COMMAND               0x00000004
#define FLG_STOP_ON_HUNG_GUI                    0x00000008
#define FLG_HEAP_ENABLE_TAIL_CHECK              0x00000010
#define FLG_HEAP_ENABLE_FREE_CHECK              0x00000020
#define FLG_HEAP_VALIDATE_PARAMETERS            0x00000040
#define FLG_HEAP_VALIDATE_ALL                   0x00000080
#define FLG_APPLICATION_VERIFIER                0x00000100
#define FLG_POOL_ENABLE_TAGGING                 0x00000400
#define FLG_HEAP_ENABLE_TAGGING                 0x00000800
#define FLG_USER_STACK_TRACE_DB                 0x00001000
#define FLG_KERNEL_STACK_TRACE_DB               0x00002000
#define FLG_MAINTAIN_OBJECT_TYPELIST            0x00004000
#define FLG_HEAP_ENABLE_TAG_BY_DLL              0x00008000
#define FLG_DISABLE_STACK_EXTENSION             0x00010000
#define FLG_ENABLE_CSRDEBUG                     0x00020000
#define FLG_ENABLE_KDEBUG_SYMBOL_LOAD           0x00040000
#define FLG_DISABLE_PAGE_KERNEL_STACKS          0x00080000
#define FLG_ENABLE_SYSTEM_CRIT_BREAKS           0x00100000
#define FLG_HEAP_DISABLE_COALESCING             0x00200000
#define FLG_ENABLE_CLOSE_EXCEPTIONS             0x00400000
#define FLG_ENABLE_EXCEPTION_LOGGING            0x00800000
#define FLG_ENABLE_HANDLE_TYPE_TAGGING          0x01000000
#define FLG_HEAP_PAGE_ALLOCS                    0x02000000
#define FLG_DEBUG_INITIAL_COMMAND_EX            0x04000000
#define FLG_VALID_BITS                          0x07FFFFFF

/*
 * Thread Access Rights
 */
#define THREAD_TERMINATE                 0x0001
#define THREAD_SUSPEND_RESUME            0x0002
#define THREAD_ALERT                     0x0004
#define THREAD_GET_CONTEXT               0x0008
#define THREAD_SET_CONTEXT               0x0010
#define THREAD_SET_INFORMATION           0x0020
#define THREAD_SET_LIMITED_INFORMATION   0x0400
#define THREAD_QUERY_LIMITED_INFORMATION 0x0800
#define PROCESS_DUP_HANDLE               0x0040

#define PROCESS_ALL_ACCESS  (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0xFFFF)
#define THREAD_ALL_ACCESS   (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0xFFFF)

/*
 * Thread priority
 */
#define LOW_PRIORITY                      0
#define LOW_REALTIME_PRIORITY             16
#define HIGH_PRIORITY                     31
#define MAXIMUM_PRIORITY                  32

/*
 * Quota limits
 */
#define QUOTA_LIMITS_HARDWS_MIN_ENABLE  0x00000001
#define QUOTA_LIMITS_HARDWS_MIN_DISABLE 0x00000002
#define QUOTA_LIMITS_HARDWS_MAX_ENABLE  0x00000004
#define QUOTA_LIMITS_HARDWS_MAX_DISABLE 0x00000008
#define QUOTA_LIMITS_USE_DEFAULT_LIMITS 0x00000010

/*
 * Process Handle Tracing Values
 */
#define PROCESS_HANDLE_TRACE_TYPE_OPEN          1
#define PROCESS_HANDLE_TRACE_TYPE_CLOSE         2
#define PROCESS_HANDLE_TRACE_TYPE_BADREF        3
#define PROCESS_HANDLE_TRACING_MAX_STACKS       16

typedef struct _QUOTA_LIMITS {
    SIZE_T PagedPoolLimit;
    SIZE_T NonPagedPoolLimit;
    SIZE_T MinimumWorkingSetSize;
    SIZE_T MaximumWorkingSetSize;
    SIZE_T PagefileLimit;
    LARGE_INTEGER TimeLimit;
} QUOTA_LIMITS, *PQUOTA_LIMITS;

typedef union _RATE_QUOTA_LIMIT {
    ULONG RateData;
    struct {
	ULONG RatePercent : 7;
	ULONG Reserved0 : 25;
    };
} RATE_QUOTA_LIMIT, *PRATE_QUOTA_LIMIT;

typedef struct _QUOTA_LIMITS_EX {
    SIZE_T PagedPoolLimit;
    SIZE_T NonPagedPoolLimit;
    SIZE_T MinimumWorkingSetSize;
    SIZE_T MaximumWorkingSetSize;
    SIZE_T PagefileLimit;
    LARGE_INTEGER TimeLimit;
    SIZE_T WorkingSetLimit;
    SIZE_T Reserved2;
    SIZE_T Reserved3;
    SIZE_T Reserved4;
    ULONG Flags;
    RATE_QUOTA_LIMIT CpuRateLimit;
} QUOTA_LIMITS_EX, *PQUOTA_LIMITS_EX;

typedef struct _IO_COUNTERS {
    ULONGLONG ReadOperationCount;
    ULONGLONG WriteOperationCount;
    ULONGLONG OtherOperationCount;
    ULONGLONG ReadTransferCount;
    ULONGLONG WriteTransferCount;
    ULONGLONG OtherTransferCount;
} IO_COUNTERS, *PIO_COUNTERS;

typedef struct _VM_COUNTERS {
    SIZE_T PeakVirtualSize;
    SIZE_T VirtualSize;
    ULONG PageFaultCount;
    SIZE_T PeakWorkingSetSize;
    SIZE_T WorkingSetSize;
    SIZE_T QuotaPeakPagedPoolUsage;
    SIZE_T QuotaPagedPoolUsage;
    SIZE_T QuotaPeakNonPagedPoolUsage;
    SIZE_T QuotaNonPagedPoolUsage;
    SIZE_T PagefileUsage;
    SIZE_T PeakPagefileUsage;
} VM_COUNTERS, *PVM_COUNTERS;

typedef struct _VM_COUNTERS_EX {
    SIZE_T PeakVirtualSize;
    SIZE_T VirtualSize;
    ULONG PageFaultCount;
    SIZE_T PeakWorkingSetSize;
    SIZE_T WorkingSetSize;
    SIZE_T QuotaPeakPagedPoolUsage;
    SIZE_T QuotaPagedPoolUsage;
    SIZE_T QuotaPeakNonPagedPoolUsage;
    SIZE_T QuotaNonPagedPoolUsage;
    SIZE_T PagefileUsage;
    SIZE_T PeakPagefileUsage;
    SIZE_T PrivateUsage;
} VM_COUNTERS_EX, *PVM_COUNTERS_EX;

#define MAX_HW_COUNTERS 16
#define THREAD_PROFILING_FLAG_DISPATCH  0x00000001

typedef enum _HARDWARE_COUNTER_TYPE {
    PMCCounter,
    MaxHardwareCounterType
} HARDWARE_COUNTER_TYPE, *PHARDWARE_COUNTER_TYPE;

typedef struct _HARDWARE_COUNTER {
    HARDWARE_COUNTER_TYPE Type;
    ULONG Reserved;
    ULONG64 Index;
} HARDWARE_COUNTER, *PHARDWARE_COUNTER;

#define PROCESS_EXCEPTION_PORT_ALL_STATE_BITS     0x00000003UL
#define PROCESS_EXCEPTION_PORT_ALL_STATE_FLAGS    ((ULONG_PTR)((1UL << PROCESS_EXCEPTION_PORT_ALL_STATE_BITS) - 1))

typedef struct _PROCESS_EXCEPTION_PORT {
    HANDLE ExceptionPortHandle;
    ULONG StateFlags;
} PROCESS_EXCEPTION_PORT, *PPROCESS_EXCEPTION_PORT;

typedef enum _PROCESSINFOCLASS {
    ProcessBasicInformation,
    ProcessQuotaLimits,
    ProcessIoCounters,
    ProcessVmCounters,
    ProcessTimes,
    ProcessBasePriority,
    ProcessRaisePriority,
    ProcessDebugPort,
    ProcessExceptionPort,
    ProcessAccessToken,
    ProcessLdtInformation,
    ProcessLdtSize,
    ProcessDefaultHardErrorMode,
    ProcessIoPortHandlers,
    ProcessPooledUsageAndLimits,
    ProcessWorkingSetWatch,
    ProcessUserModeIOPL,
    ProcessEnableAlignmentFaultFixup,
    ProcessPriorityClass,
    ProcessWx86Information,
    ProcessHandleCount,
    ProcessAffinityMask,
    ProcessPriorityBoost,
    ProcessDeviceMap,
    ProcessSessionInformation,
    ProcessForegroundInformation,
    ProcessWow64Information,
    ProcessImageFileName,
    ProcessLUIDDeviceMapsEnabled,
    ProcessBreakOnTermination,
    ProcessDebugObjectHandle,
    ProcessDebugFlags,
    ProcessHandleTracing,
    ProcessIoPriority,
    ProcessExecuteFlags,
    ProcessTlsInformation,
    ProcessCookie,
    ProcessImageInformation,
    ProcessCycleTime,
    ProcessPagePriority,
    ProcessInstrumentationCallback,
    ProcessThreadStackAllocation,
    ProcessWorkingSetWatchEx,
    ProcessImageFileNameWin32,
    ProcessImageFileMapping,
    ProcessAffinityUpdateMode,
    ProcessMemoryAllocationMode,
    ProcessGroupInformation,
    ProcessTokenVirtualizationEnabled,
    ProcessConsoleHostProcess,
    ProcessWindowInformation,
    MaxProcessInfoClass
} PROCESSINFOCLASS, PROCESS_INFORMATION_CLASS;

typedef enum _THREADINFOCLASS {
    ThreadBasicInformation,
    ThreadTimes,
    ThreadPriority,
    ThreadBasePriority,
    ThreadAffinityMask,
    ThreadImpersonationToken,
    ThreadDescriptorTableEntry,
    ThreadEnableAlignmentFaultFixup,
    ThreadEventPair_Reusable,
    ThreadQuerySetWin32StartAddress,
    ThreadZeroTlsCell,
    ThreadPerformanceCount,
    ThreadAmILastThread,
    ThreadIdealProcessor,
    ThreadPriorityBoost,
    ThreadSetTlsArrayAddress,
    ThreadIsIoPending,
    ThreadHideFromDebugger,
    ThreadBreakOnTermination,
    ThreadSwitchLegacyState,
    ThreadIsTerminated,
    ThreadLastSystemCall,
    ThreadIoPriority,
    ThreadCycleTime,
    ThreadPagePriority,
    ThreadActualBasePriority,
    ThreadTebInformation,
    ThreadCSwitchMon,
    ThreadCSwitchPmu,
    ThreadWow64Context,
    ThreadGroupInformation,
    ThreadUmsInformation,
    ThreadCounterProfiling,
    ThreadIdealProcessorEx,
    MaxThreadInfoClass
} THREADINFOCLASS, THREAD_INFORMATION_CLASS;

typedef struct _GDI_TEB_BATCH {
    ULONG Offset;
    HANDLE HDC;
    ULONG Buffer[0x136];
} GDI_TEB_BATCH;

typedef struct _ACTIVATION_CONTEXT_STACK {
    ULONG Flags;
    ULONG NextCookieSequenceNumber;
    RTL_ACTIVATION_CONTEXT_STACK_FRAME* ActiveFrame;
    LIST_ENTRY FrameListCache;
} ACTIVATION_CONTEXT_STACK, * PACTIVATION_CONTEXT_STACK;

/*
 * Initial TEB
 */
typedef struct _INITIAL_TEB {
    PVOID PreviousStackBase;
    PVOID PreviousStackLimit;
    PVOID StackBase;		/* Top of the initial stack */
    PVOID StackLimit;		/* Bottom of the initial committed stack */
    PVOID AllocatedStackBase;	/* Bottom of the initial stack reserve */
} INITIAL_TEB, *PINITIAL_TEB;

/*
 * TEB Active Frame Structures
 */
typedef struct _TEB_ACTIVE_FRAME_CONTEXT {
    ULONG Flags;
    PCSTR FrameName;
} TEB_ACTIVE_FRAME_CONTEXT, *PTEB_ACTIVE_FRAME_CONTEXT;

typedef struct _TEB_ACTIVE_FRAME {
    ULONG Flags;
    struct _TEB_ACTIVE_FRAME *Previous;
    PTEB_ACTIVE_FRAME_CONTEXT Context;
} TEB_ACTIVE_FRAME, *PTEB_ACTIVE_FRAME;

typedef VOID (NTAPI *PPS_POST_PROCESS_INIT_ROUTINE)(VOID);
typedef VOID (NTAPI *PFLS_CALLBACK_FUNCTION)(PVOID);

/*
 * Fiber local storage data
 */
#define RTL_FLS_MAXIMUM_AVAILABLE 128
typedef struct _RTL_FLS_DATA {
    LIST_ENTRY ListEntry;
    PVOID Data[RTL_FLS_MAXIMUM_AVAILABLE];
} RTL_FLS_DATA, *PRTL_FLS_DATA;

/* For ELF targets we need the packed attribute since clang puts extra spaces otherwise */
#include <pshpack4.h>
typedef struct _PEB {                                                 /* win32/win64 */
    BOOLEAN                          InheritedAddressSpace;             /* 000/000 */
    BOOLEAN                          ReadImageFileExecOptions;          /* 001/001 */
    BOOLEAN                          BeingDebugged;                     /* 002/002 */
    BOOLEAN                          SpareBool;                         /* 003/003 */
    HANDLE                           Mutant;                            /* 004/008 */
    HMODULE                          ImageBaseAddress;                  /* 008/010 */
    PPEB_LDR_DATA                    LdrData;                           /* 00c/018 */
    RTL_USER_PROCESS_PARAMETERS     *ProcessParameters;                 /* 010/020 */
    PVOID                            SubSystemData;                     /* 014/028 */
    HANDLE                           ProcessHeap;                       /* 018/030 */
    struct _RTL_CRITICAL_SECTION    *FastPebLock;                       /* 01c/038 */
    PVOID /*PPEBLOCKROUTINE*/        FastPebLockRoutine;                /* 020/040 */
    PVOID /*PPEBLOCKROUTINE*/        FastPebUnlockRoutine;              /* 024/048 */
    ULONG                            EnvironmentUpdateCount;            /* 028/050 */
    PVOID                            KernelCallbackTable;               /* 02c/058 */
    ULONG                            Reserved[2];                       /* 030/060 */
    PVOID /*PPEB_FREE_BLOCK*/        FreeList;                          /* 038/068 */
    ULONG                            TlsExpansionCounter;               /* 03c/070 */
    PRTL_BITMAP                      TlsBitmap;                         /* 040/078 */
    ULONG                            TlsBitmapBits[2];                  /* 044/080 */
    PVOID                            ReadOnlySharedMemoryBase;          /* 04c/088 */
    PVOID                            ReadOnlySharedMemoryHeap;          /* 050/090 */
    PVOID                           *ReadOnlyStaticServerData;          /* 054/098 */
    PVOID                            AnsiCodePageData;                  /* 058/0a0 */
    PVOID                            OemCodePageData;                   /* 05c/0a8 */
    PVOID                            UnicodeCaseTableData;              /* 060/0b0 */
    ULONG                            NumberOfProcessors;                /* 064/0b8 */
    ULONG                            NtGlobalFlag;                      /* 068/0bc */
    LARGE_INTEGER                    CriticalSectionTimeout;            /* 070/0c0 */
    SIZE_T                           HeapSegmentReserve;                /* 078/0c8 */
    SIZE_T                           HeapSegmentCommit;                 /* 07c/0d0 */
    SIZE_T                           HeapDeCommitTotalFreeThreshold;    /* 080/0d8 */
    SIZE_T                           HeapDeCommitFreeBlockThreshold;    /* 084/0e0 */
    ULONG                            NumberOfHeaps;                     /* 088/0e8 */
    ULONG                            MaximumNumberOfHeaps;              /* 08c/0ec */
    PVOID                           *ProcessHeaps;                      /* 090/0f0 */
    PVOID                            GdiSharedHandleTable;              /* 094/0f8 */
    PVOID                            ProcessStarterHelper;              /* 098/100 */
    PVOID                            GdiDCAttributeList;                /* 09c/108 */
    PVOID                            LoaderLock;                        /* 0a0/110 */
    ULONG                            OSMajorVersion;                    /* 0a4/118 */
    ULONG                            OSMinorVersion;                    /* 0a8/11c */
    ULONG                            OSBuildNumber;                     /* 0ac/120 */
    ULONG                            OSPlatformId;                      /* 0b0/124 */
    ULONG                            ImageSubSystem;                    /* 0b4/128 */
    ULONG                            ImageSubSystemMajorVersion;        /* 0b8/12c */
    ULONG                            ImageSubSystemMinorVersion;        /* 0bc/130 */
    ULONG                            ImageProcessAffinityMask;          /* 0c0/134 */
    HANDLE                           GdiHandleBuffer[28];               /* 0c4/138 */
    ULONG                            unknown[6];                        /* 134/218 */
    PPS_POST_PROCESS_INIT_ROUTINE    PostProcessInitRoutine;            /* 14c/230 */
    PRTL_BITMAP                      TlsExpansionBitmap;                /* 150/238 */
    ULONG                            TlsExpansionBitmapBits[32];        /* 154/240 */
    ULONG                            SessionId;                         /* 1d4/2c0 */
    ULARGE_INTEGER                   AppCompatFlags;                    /* 1d8/2c8 */
    ULARGE_INTEGER                   AppCompatFlagsUser;                /* 1e0/2d0 */
    PVOID                            ShimData;                          /* 1e8/2d8 */
    PVOID                            AppCompatInfo;                     /* 1ec/2e0 */
    UNICODE_STRING                   CSDVersion;                        /* 1f0/2e8 */
    PVOID                            ActivationContextData;             /* 1f8/2f8 */
    PVOID                            ProcessAssemblyStorageMap;         /* 1fc/300 */
    PVOID                            SystemDefaultActivationData;       /* 200/308 */
    PVOID                            SystemAssemblyStorageMap;          /* 204/310 */
    SIZE_T                           MinimumStackCommit;                /* 208/318 */
    PFLS_CALLBACK_FUNCTION          *FlsCallback;                       /* 20c/320 */
    LIST_ENTRY                       FlsListHead;                       /* 210/328 */
    PRTL_BITMAP                      FlsBitmap;                         /* 218/338 */
    ULONG                            FlsBitmapBits[4];                  /* 21c/340 */
    ULONG                            FlsHighIndex;                      /* 22c/350 */
} PEB, *PPEB;
#include <poppack.h>

typedef struct _EXCEPTION_REGISTRATION_RECORD {
    struct _EXCEPTION_REGISTRATION_RECORD *Next;
    PEXCEPTION_ROUTINE Handler;
} EXCEPTION_REGISTRATION_RECORD, *PEXCEPTION_REGISTRATION_RECORD;

typedef struct _NT_TIB {
    struct _EXCEPTION_REGISTRATION_RECORD *ExceptionList;
    PVOID StackBase;
    PVOID StackLimit;
    PVOID SubSystemTib;
    union {
	PVOID FiberData;
	ULONG Version;
    };
    PVOID ArbitraryUserPointer;
    struct _NT_TIB *Self;
} NT_TIB, *PNT_TIB;

#include <pshpack4.h>
typedef struct _TEB {                                             /* win32/win64 */
    NT_TIB                       NtTib;                             /* 000/0000 */
    PVOID                        EnvironmentPointer;                /* 01c/0038 */
    CLIENT_ID                    ClientId;                          /* 020/0040 */
    PVOID                        ActiveRpcHandle;                   /* 028/0050 */
    PVOID                        ThreadLocalStoragePointer;         /* 02c/0058 */
    PPEB                         ProcessEnvironmentBlock;           /* 030/0060 */
    ULONG                        LastErrorValue;                    /* 034/0068 */
    ULONG                        CountOfOwnedCriticalSections;      /* 038/006c */
    PVOID                        CsrClientThread;                   /* 03c/0070 */
    PVOID                        Win32ThreadInfo;                   /* 040/0078 */
    ULONG                        Win32ClientInfo[31];               /* 044/0080 used for user32 private data in Wine */
    PVOID                        WOW32Reserved;                     /* 0c0/0100 */
    ULONG                        CurrentLocale;                     /* 0c4/0108 */
    ULONG                        FpSoftwareStatusRegister;          /* 0c8/010c */
    PVOID                        SystemReserved1[54];               /* 0cc/0110 used for kernel32 private data in Wine */
    LONG                         ExceptionCode;                     /* 1a4/02c0 */
    ACTIVATION_CONTEXT_STACK     ActivationContextStackPointer;     /* 1a8/02c8 */
    BYTE                         SpareBytes1[24];                   /* 1bc/02e8 */
    PVOID                        SystemReserved2[10];               /* 1d4/0300 used for ntdll platform-specific private data in Wine */
    GDI_TEB_BATCH                GdiTebBatch;                       /* 1fc/0350 used for ntdll private data in Wine */
    HANDLE                       gdiRgn;                            /* 6dc/0838 */
    HANDLE                       gdiPen;                            /* 6e0/0840 */
    HANDLE                       gdiBrush;                          /* 6e4/0848 */
    CLIENT_ID                    RealClientId;                      /* 6e8/0850 */
    HANDLE                       GdiCachedProcessHandle;            /* 6f0/0860 */
    ULONG                        GdiClientPID;                      /* 6f4/0868 */
    ULONG                        GdiClientTID;                      /* 6f8/086c */
    PVOID                        GdiThreadLocalInfo;                /* 6fc/0870 */
    ULONG                        UserReserved[5];                   /* 700/0878 */
    PVOID                        glDispatchTable[280];              /* 714/0890 */
    PVOID                        glReserved1[26];                   /* b74/1150 */
    PVOID                        glReserved2;                       /* bdc/1220 */
    PVOID                        glSectionInfo;                     /* be0/1228 */
    PVOID                        glSection;                         /* be4/1230 */
    PVOID                        glTable;                           /* be8/1238 */
    PVOID                        glCurrentRC;                       /* bec/1240 */
    PVOID                        glContext;                         /* bf0/1248 */
    ULONG                        LastStatusValue;                   /* bf4/1250 */
    UNICODE_STRING               StaticUnicodeString;               /* bf8/1258 used by advapi32 */
    WCHAR                        StaticUnicodeBuffer[261];          /* c00/1268 used by advapi32 */
    PVOID                        DeallocationStack;                 /* e0c/1478 */
    PVOID                        TlsSlots[64];                      /* e10/1480 */
    LIST_ENTRY                   TlsLinks;                          /* f10/1680 */
    PVOID                        Vdm;                               /* f18/1690 */
    PVOID                        ReservedForNtRpc;                  /* f1c/1698 */
    PVOID                        DbgSsReserved[2];                  /* f20/16a0 */
    ULONG                        HardErrorMode;                     /* f28/16b0 */
    PVOID                        Instrumentation[16];               /* f2c/16b8 */
    PVOID                        WinSockData;                       /* f6c/1738 */
    ULONG                        GdiBatchCount;                     /* f70/1740 */
    union {
	struct {
	    BOOLEAN              InDbgPrint;                        /* f74/1744 */
	    BOOLEAN              FreeStackOnTermination;            /* f75/1745 */
	    BOOLEAN              HasFiberData;                      /* f76/1746 */
	    UCHAR                IdealProcessor;                    /* f77/1747 */
	};
	ULONG                    IdealProcessorValue;               /* f74/1744 */
    };
    ULONG                        GuaranteedStackBytes;              /* f78/1748 */
    PVOID                        ReservedForPerf;                   /* f7c/1750 */
    PVOID                        ReservedForOle;                    /* f80/1758 */
    ULONG                        WaitingOnLoaderLock;               /* f84/1760 */
    PVOID                        Reserved5[3];                      /* f88/1768 */
    PVOID                       *TlsExpansionSlots;                 /* f94/1780 */
#ifdef _WIN64
    PVOID                        DeallocationBStore;                /*    /1788 */
    PVOID                        BStoreLimit;                       /*    /1790 */
#endif
    ULONG                        ImpersonationLocale;               /* f98/1798 */
    ULONG                        IsImpersonating;                   /* f9c/179c */
    PVOID                        NlsCache;                          /* fa0/17a0 */
    PVOID                        ShimData;                          /* fa4/17a8 */
    ULONG                        HeapVirtualAffinity;               /* fa8/17b0 */
    PVOID                        CurrentTransactionHandle;          /* fac/17b8 */
    TEB_ACTIVE_FRAME            *ActiveFrame;                       /* fb0/17c0 */
    PRTL_FLS_DATA                FlsData;                           /* fb4/17c8 */
} TEB, *PTEB;
#include <poppack.h>

/*
 * Process Information Structures for NtQueryProcessInformation
 */
typedef struct _PROCESS_BASIC_INFORMATION {
    NTSTATUS ExitStatus;
    PPEB PebBaseAddress;
    ULONG_PTR AffinityMask;
    KPRIORITY BasePriority;
    ULONG_PTR UniqueProcessId;
    ULONG_PTR InheritedFromUniqueProcessId;
} PROCESS_BASIC_INFORMATION, *PPROCESS_BASIC_INFORMATION;

typedef struct _PROCESS_ACCESS_TOKEN {
    HANDLE Token;
    HANDLE Thread;
} PROCESS_ACCESS_TOKEN, *PPROCESS_ACCESS_TOKEN;

typedef struct _PROCESS_DEVICEMAP_INFORMATION {
    union {
	struct {
	    HANDLE DirectoryHandle;
	} Set;
	struct {
	    ULONG DriveMap;
	    UCHAR DriveType[32];
	} Query;
    };
} PROCESS_DEVICEMAP_INFORMATION, *PPROCESS_DEVICEMAP_INFORMATION;

typedef struct _KERNEL_USER_TIMES {
    LARGE_INTEGER CreateTime;
    LARGE_INTEGER ExitTime;
    LARGE_INTEGER KernelTime;
    LARGE_INTEGER UserTime;
} KERNEL_USER_TIMES, *PKERNEL_USER_TIMES;

typedef struct _POOLED_USAGE_AND_LIMITS {
    SIZE_T PeakPagedPoolUsage;
    SIZE_T PagedPoolUsage;
    SIZE_T PagedPoolLimit;
    SIZE_T PeakNonPagedPoolUsage;
    SIZE_T NonPagedPoolUsage;
    SIZE_T NonPagedPoolLimit;
    SIZE_T PeakPagefileUsage;
    SIZE_T PagefileUsage;
    SIZE_T PagefileLimit;
} POOLED_USAGE_AND_LIMITS, *PPOOLED_USAGE_AND_LIMITS;

typedef struct _PROCESS_WS_WATCH_INFORMATION {
    PVOID FaultingPc;
    PVOID FaultingVa;
} PROCESS_WS_WATCH_INFORMATION, *PPROCESS_WS_WATCH_INFORMATION;

typedef struct _PROCESS_SESSION_INFORMATION {
    ULONG SessionId;
} PROCESS_SESSION_INFORMATION, *PPROCESS_SESSION_INFORMATION;

typedef struct _PROCESS_HANDLE_TRACING_ENTRY {
    HANDLE Handle;
    CLIENT_ID ClientId;
    ULONG Type;
    PVOID Stacks[PROCESS_HANDLE_TRACING_MAX_STACKS];
} PROCESS_HANDLE_TRACING_ENTRY, *PPROCESS_HANDLE_TRACING_ENTRY;

typedef struct _PROCESS_HANDLE_TRACING_QUERY {
    HANDLE Handle;
    ULONG TotalTraces;
    PROCESS_HANDLE_TRACING_ENTRY HandleTrace[ANYSIZE_ARRAY];
} PROCESS_HANDLE_TRACING_QUERY, *PPROCESS_HANDLE_TRACING_QUERY;

typedef struct _PROCESS_LDT_INFORMATION {
    ULONG Start;
    ULONG Length;
    PVOID LdtEntries;
} PROCESS_LDT_INFORMATION, *PPROCESS_LDT_INFORMATION;

typedef struct _PROCESS_LDT_SIZE {
    ULONG Length;
} PROCESS_LDT_SIZE, *PPROCESS_LDT_SIZE;

typedef struct _PROCESS_PRIORITY_CLASS {
    BOOLEAN Foreground;
    UCHAR PriorityClass;
} PROCESS_PRIORITY_CLASS, *PPROCESS_PRIORITY_CLASS;

// Compatibility with windows, see ReactOS CORE-16757, CORE-17106, CORE-17247
C_ASSERT(sizeof(PROCESS_PRIORITY_CLASS) == 2);

typedef struct _PROCESS_FOREGROUND_BACKGROUND {
    BOOLEAN Foreground;
} PROCESS_FOREGROUND_BACKGROUND, *PPROCESS_FOREGROUND_BACKGROUND;

/*
 * Thread Information Structures for NtQueryProcessInformation
 */
typedef struct _THREAD_BASIC_INFORMATION {
    NTSTATUS ExitStatus;
    PVOID TebBaseAddress;
    CLIENT_ID ClientId;
    KAFFINITY AffinityMask;
    KPRIORITY Priority;
    KPRIORITY BasePriority;
} THREAD_BASIC_INFORMATION, *PTHREAD_BASIC_INFORMATION;

/*
 * Native Calls. These are only available for client threads.
 * The NTOS root task has different function signatures for these.
 */
#ifndef _NTOSKRNL_

NTAPI NTSYSAPI NTSTATUS NtTerminateProcess(IN HANDLE ProcessHandle,
					   IN NTSTATUS ExitStatus);

NTAPI NTSYSAPI NTSTATUS NtTerminateThread(IN HANDLE ThreadHandle,
					  IN NTSTATUS ExitStatus);

NTAPI NTSYSAPI NTSTATUS NtResumeThread(IN HANDLE ThreadHandle,
				       OUT OPTIONAL PULONG SuspendCount);

#endif
