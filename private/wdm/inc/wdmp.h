#pragma once

#include <nt.h>
#include <wdm.h>
#include <assert.h>
#include <debug.h>

extern PCSTR IopDbgTraceModuleName;
#define RTLP_DBGTRACE_MODULE_NAME	IopDbgTraceModuleName

#include <util.h>
#include <wdmsvc.h>
#include <wdm_wdmsvc_gen.h>

#define TAG_DRIVER_EXTENSION	'EVRD'
#define TAG_REINIT		'iRoI'
#define TAG_SYS_BUF		'BSYS'

#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b))

/* Prevent the compiler from inlining the function */
#define NO_INLINE	__attribute__((noinline))

/* Inform the compiler that the function does not return */
#define NORETURN	__attribute__((__noreturn__))

/* Shared kernel data that is accessible from user space */
#define SharedUserData ((KUSER_SHARED_DATA *CONST) KUSER_SHARED_DATA_CLIENT_ADDR)

#define IopAllocatePoolEx(Ptr, Type, Size, OnError)		\
    Type *Ptr = (Type *) RtlAllocateHeap(RtlGetProcessHeap(),	\
					 HEAP_ZERO_MEMORY,	\
					 Size);			\
    if (Ptr == NULL) {						\
	OnError;						\
	return STATUS_NO_MEMORY;				\
    }

#define IopAllocatePool(Ptr, Type, Size)	\
    IopAllocatePoolEx(Ptr, Type, Size, {})

#define IopAllocateObjectEx(Ptr, Type, OnError)		\
    IopAllocatePoolEx(Ptr, Type, sizeof(Type), OnError)

#define IopAllocateObject(Ptr, Type)		\
    IopAllocateObjectEx(Ptr, Type, {})

#define IopFreePool(Ptr)			\
    RtlFreeHeap(RtlGetProcessHeap(), 0, Ptr)

/*
 * Private IO types
 */
#define IOP_TYPE_WORKITEM		0xa0
#define IOP_TYPE_ADD_DEVICE_REQUEST	0xad

/*
 * A PNP AddDevice request from the server.
 */
typedef struct _ADD_DEVICE_REQUEST {
    SHORT Type;
    USHORT Size;
    NTSTATUS Status;
    PDEVICE_OBJECT PhyDevObj;
    PDRIVER_ADD_DEVICE AddDevice;
    LIST_ENTRY Link;
    PVOID CoroutineStackTop;
    ULONG_PTR OriginalRequestor;
    HANDLE Identifier;
} ADD_DEVICE_REQUEST, *PADD_DEVICE_REQUEST;

/*
 * Driver Re-Initialization Entry
 */
typedef struct _DRIVER_REINIT_ITEM {
    LIST_ENTRY ItemEntry;
    PDRIVER_OBJECT DriverObject;
    PDRIVER_REINITIALIZE ReinitRoutine;
    PVOID Context;
    ULONG Count;
} DRIVER_REINIT_ITEM, *PDRIVER_REINIT_ITEM;

/*
 * X86 IO port object
 */
typedef struct _X86_IOPORT {
    MWORD Cap;
    LIST_ENTRY Link;
    USHORT PortNum;
} X86_IOPORT, *PX86_IOPORT;

/*
 * IO work item object.
 */
typedef struct _IO_WORKITEM {
    SHORT Type;
    USHORT Size;
    PDEVICE_OBJECT DeviceObject;
    SLIST_ENTRY DECLSPEC_ALIGN(MEMORY_ALLOCATION_ALIGNMENT) QueueEntry;
    LIST_ENTRY SuspendedListEntry;
    PVOID CoroutineStackTop;
    union {
	PIO_WORKITEM_ROUTINE WorkerRoutine;
	PIO_WORKITEM_ROUTINE_EX WorkerRoutineEx;
    };
    PVOID Context;
    BOOLEAN ExtendedRoutine; /* TRUE if the union above is WorkerRoutineEx */
} IO_WORKITEM;

/*
 * Lightweight mutex, used to synchronize data shared between two driver threads.
 *
 * This is used to implement what the Windows driver model calls the "interrupt
 * spinlock", which protects data structures accessed by both the dispatch routines
 * and the interrupt service routines. We don't want to use spinlocks here since our
 * drivers runs in a userspace process (and may get scheduled out).
 */
typedef struct POINTER_ALIGNMENT _KMUTEX {
    MWORD Notification;		/* Notification capability */
    LONG Counter;		/* 0 -- Mutex available.
				 * >= 1 -- Lock is held. Number indicates
				 * number of contenting threads. */
} KMUTEX, *PKMUTEX;

static inline VOID KeInitializeMutex(IN PKMUTEX Mutex,
				     IN MWORD Cap)
{
    assert(Mutex != NULL);
    Mutex->Notification = Cap;
    Mutex->Counter = 0;
}

/*
 * Acquire the lock. If the lock is free, simply acquire the lock and return.
 * If the lock has already been acquired by another thread, wait on the notification
 * object.
 */
static inline VOID KeAcquireMutex(IN PKMUTEX Mutex)
{
    assert(Mutex != NULL);
    assert(Mutex->Notification != 0);
    if (InterlockedIncrement(&Mutex->Counter) != 1) {
	seL4_Wait(Mutex->Notification, NULL);
    }
}

/*
 * Release the mutex that is previously acquired. Note that you must only call
 * this function after you have acquired the mutex (KeTryAcquireMutex returns TRUE).
 * On debug build we assert if this has not been enforced.
 */
static inline VOID KeReleaseMutex(IN PKMUTEX Mutex)
{
    assert(Mutex != NULL);
    assert(Mutex->Notification != 0);
    LONG Counter = InterlockedDecrement(&Mutex->Counter);
    assert(Counter >= 0);
    if (Counter >= 1) {
	seL4_Signal(Mutex->Notification);
    }
}

/*
 * Interrupt object.
 */
typedef struct DECLSPEC_ALIGN(MEMORY_ALLOCATION_ALIGNMENT) _KINTERRUPT {
    ULONG Vector;
    KIRQL Irql;
    KIRQL SynchronizeIrql;
    KINTERRUPT_MODE InterruptMode;
    KMUTEX Mutex;
    PKSERVICE_ROUTINE ServiceRoutine;
    PVOID ServiceContext;
    MWORD WdmServiceCap;
    MWORD ThreadCap;
    PVOID ThreadIpcBuffer;
    MWORD IrqHandlerCap;
    MWORD NotificationCap;
} KINTERRUPT;

/* device.c */
extern LIST_ENTRY IopDeviceList;
PDEVICE_OBJECT IopGetDeviceObject(IN GLOBAL_HANDLE Handle);
GLOBAL_HANDLE IopGetDeviceHandle(IN PDEVICE_OBJECT Device);
PDEVICE_OBJECT IopGetDeviceObjectOrCreate(IN GLOBAL_HANDLE DeviceHandle,
					  IN IO_DEVICE_INFO DevInfo);

/* irp.c */
extern PIO_PACKET IopIncomingIoPacketBuffer;
extern PIO_PACKET IopOutgoingIoPacketBuffer;
extern LIST_ENTRY IopIrpQueue;
extern LIST_ENTRY IopFileObjectList;
extern LIST_ENTRY IopReplyIrpList;
extern LIST_ENTRY IopPendingIrpList;
extern LIST_ENTRY IopCleanupIrpList;
extern LIST_ENTRY IopAddDeviceRequestList;
extern LIST_ENTRY IopSuspendedAddDeviceRequestList;
extern LIST_ENTRY IopCompletedAddDeviceRequestList;
extern PVOID IopCurrentObject;
VOID IopProcessIoPackets(OUT ULONG *pNumResponses,
			 IN ULONG NumRequests);
VOID IoDbgDumpIrp(IN PIRP Irp);

/* isr.c */
extern LIST_ENTRY IopDpcQueue;
extern KMUTEX IopDpcMutex;
VOID IopProcessDpcQueue();

/* ioport.c */
extern LIST_ENTRY IopX86PortList;

/* main.c */
extern DRIVER_OBJECT IopDriverObject;
extern __thread seL4_CPtr KiWdmServiceCap;

/* timer.c */
extern LIST_ENTRY IopTimerList;
extern ULONG KiStallScaleFactor;

/* workitem.c */
extern SLIST_HEADER IopWorkItemQueue;
extern LIST_ENTRY IopSuspendedWorkItemList;
VOID IopProcessWorkItemQueue();
BOOLEAN IopWorkItemIsInSuspendedList(IN PIO_WORKITEM Item);
VOID IopDbgDumpWorkItem(IN PIO_WORKITEM WorkItem);

static inline BOOLEAN IopDeviceObjectIsLocal(IN PDEVICE_OBJECT DeviceObject)
{
    return DeviceObject->DriverObject == &IopDriverObject;
}
