#pragma once

#include <nt.h>
#include <ntos.h>
#include <wdmsvc.h>

#define NTOS_IO_TAG	(EX_POOL_TAG('n','t','i','o'))

#define IopAllocatePoolEx(Var, Type, OnError)				\
    ExAllocatePoolEx(Var, Type, sizeof(Type), NTOS_IO_TAG, OnError)
#define IopAllocatePool(Var, Type)	IopAllocatePoolEx(Var, Type, {})
#define IopAllocateArrayEx(Var, Type, Size, OnError)			\
    ExAllocatePoolEx(Var, Type, sizeof(Type) * (Size), NTOS_IO_TAG, OnError)
#define IopAllocateArray(Var, Type, Size)	\
    IopAllocateArrayEx(Var, Type, Size, {})

/*
 * Device Node for the PNP manager
 */
typedef struct _DEVICE_NODE {
    PIO_DEVICE_OBJECT PhyDevObj;
    PCSTR DeviceId;
    PCSTR InstanceId;
    struct _DEVICE_NODE *Parent;
    LIST_ENTRY ChildrenList;
    LIST_ENTRY SiblingLink;
    PCSTR DriverServiceName;
    ULONG UpperFilterCount;
    ULONG LowerFilterCount;
    PCSTR *UpperFilterDriverNames;
    PCSTR *LowerFilterDriverNames;
    PIO_DRIVER_OBJECT FunctionDriverObject;
    PIO_DRIVER_OBJECT *UpperFilterDrivers;
    PIO_DRIVER_OBJECT *LowerFilterDrivers;
    PCM_RESOURCE_LIST Resources;
    BOOLEAN DriverLoaded;
    BOOLEAN DeviceStarted;
} DEVICE_NODE, *PDEVICE_NODE;

/*
 * The PENDING_IRP represents a pending IRP that is queued by either a THREAD object
 * or a DRIVER object. There are several possible scenarios when it comes to IRP
 * flow. These are
 *
 * 1) If a THREAD object queues an IRP to a driver object, we add PENDING_IRP to the
 * THREAD object's pending IRP list and queue the IO_PACKET to the driver object's
 * IoPacketQueue. When the driver completes the IRP, the thread is notified by
 * signalling PENDING_IRP.IoCompletionEvent.
 *
 * 2) If a DRIVER object Drv0 queues a new IRP to another driver object Drv1, we add
 * the PENDING_IRP struct to Drv0's ForwardedIrpList and queue the IO_PACKET to Drv1's
 * IrpQueue. When Drv1 completes the IRP, Drv0 is notified via a IoCompleted message.
 *
 * 3) If a driver object Drv1 forwards an existing IRP to another driver object Drv2
 * without requesting server notification (NotifyCompletion == FALSE), we detach the
 * IO_PACKET from Drv1's PendingIoPacketList and queues it to Drv2's IoPacketQueue.
 * Whichever object has queued the IRP will be notified once the IRP is completed by
 * Drv2. Drv1 is never notified.
 *
 * 4) However, if the client has requested that the server notify it of the completion
 * of an IRP that it has forwarded (by setting NotifyCompletion to TRUE in the client
 * message), in addition to moving the IO_PACKET from Drv1 to Drv2, we also create a
 * new PENDING_IRP struct that points to the existing PENDING_IRP so we can later reply
 * to the higher-level driver (Drv1) to notify the completion of the IRP that was
 * forwarded. Note that although on client side the IRP IoCompletion routine may return
 * StopCompletion (STATUS_MORE_PROCESSING_REQUIRED) to halt the IRP completion (usually
 * the driver adds the IRP back to a driver-defined queue and completes the IRP at a
 * later time), this is transparent to the server as the client side dll (wdm.dll)
 * will not reply to server until the IRP has been completed at that level.
 *
 * See the following diagrams for details.
 *
 * NOTE: do not confuse this with the driver object's PendingIoPacketList. That list
 * is used to keep track of the IO packets that have been sent over to the driver.
 *
 * Flow of IO packets:
 *
 * Case 1: THREAD object queues an IRP on a DRIVER object
 *
 *  |-------------|       |-----------------------|       |-------------------------|       |------------------|       |---------|
 *  |  IO_PACKET  |       | DRIVER.IoPacketQueue  |       | DRV.PendingIoPacketList |       | Driver completes |       | Server  |
 *  |-------------| ----> |-----------------------| ----> |-------------------------| ----> | IRP, replies     | ----> | signals |
 *  | PENDING_IRP |       | THREAD.PendingIrpList |       | THREAD.PendingIrpList   |   |   | to server        |       | THREAD  |
 *  |-------------|       |-----------------------|       |-------------------------|   |   |------------------|       |---------|
 *                                                                                      |or
 *                                                                                      |-----> Case 3,4
 *
 * Case 2: Driver0 queues a new IRP on Driver1 via IoCallDriver
 *
 *  |-------------|       |-----------------------|       |-----------------------|       |----------------|       |---------|
 *  |  IO_PACKET  |       |   Drv1.IoPacketQueue  |       | Drv1.PendingIoPacketL |       | Drv1 completes |       | Server  |
 *  |-------------| ----> |-----------------------| ----> |-----------------------| ----> | IRP, replies   | ----> | replies |
 *  | PENDING_IRP |       | Drv0.ForwardedIrpList |       | Drv0.ForwardedIrpList |   |   | to server      |       | to Drv0 |
 *  |-------------|       |-----------------------|       |-----------------------|   |   |----------------|       |---------|
 *                                                                                    |or
 *                                                                                    |-----> Case 3,4
 *
 * Case 3: Driver1 forwards an existing IRP to Driver2 via IoCallDriver, NotifyCompletion == FALSE
 *
 *  |--------------------------|       |--------------------------|       |----------------|       |----------------|
 *  | Drv1.PendingIoPacketList |       | Drv2.PendingIoPacketList |       | Drv2 completes |       | Server signals |
 *  |--------------------------| ----> |--------------------------| ----> | IRP, replies   | ----> | thread/replies |
 *  |       PENDING_IRP        |       |     DOES NOT CHANGE      |   |   | to server      |       | to driver      |
 *  |--------------------------|       |--------------------------|   |   |----------------|       |----------------|
 *                                                                    |or
 *                                                                    |-----> Case 3,4
 *
 * Case 4: Driver2 forwards an existing IRP to Driver3 via IoCallDriver, NotifyCompletion == TRUE
 *
 *  |--------------------------|       |--------------------------|       |----------------|
 *  | Drv2.PendingIoPacketList |       | Drv3.PendingIoPacketList |       | Drv3 completes |
 *  |--------------------------| ----> |--------------------------| ----> | IRP, replies   |
 *  |       PENDING_IRP        |       |     DOES NOT CHANGE      |   |   | to server      |
 *  |--------------------------|       |--------------------------|   |   |----------------|
 *               |                                                  or|           |
 *              \|/                                     Case 3,4 <----|          \|/
 *  |-----------------------------|       |-----------------------|       |----------------|       |----------------|
 *  | New PENDING_IRP that links  | ----> | Drv2.ForwardedIrpList | ----> | Server replies |       | Server signals |
 *  | to the existing PENDING_IRP |       |-----------------------|       | to Drv2, waits | ----> | thread/replies |
 *  |-----------------------------|                                       | for Drv2 reply |       | to driver      |
 *                                                                        |----------------|       |----------------|
 */
typedef struct _PENDING_IRP {
    PIO_PACKET IoPacket; /* IO packet that the thread is waiting for a response for.
			  * The pending IO packet must be of type IoPacketTypeRequest. */
    POBJECT Requestor; /* Points to the THREAD or DRIVER object at this level */
    LIST_ENTRY Link; /* List entry for THREAD.PendingIrpList or DRIVER.ForwardedIrpList */
    struct _PENDING_IRP *ForwardedTo; /* Points to the driver object that this IRP has been forwarded to. */
    struct _PENDING_IRP *ForwardedFrom; /* Back pointer for ForwardedTo. */
    MWORD InputBuffer; /* Pointer in the address space of the THREAD or DRIVER at this level */
    MWORD OutputBuffer;	/* Pointer in the address space of the THREAD or DRIVER at this level */
    /* ---- The following four members are only valid if Requestor is a THREAD object ---- */
    KEVENT IoCompletionEvent; /* Signaled when the IO request has been completed. */
    IO_STATUS_BLOCK IoResponseStatus; /* Response status to the pending IO packet. */
    ULONG IoResponseDataSize; /* Size of the response data to the pending IO packet. */
    PVOID IoResponseData; /* Response data to the pending IO packet. NULL if not supplied
			   * or if server-side allocation failed. */
    /* ---- The following member is only valid if Requestor is a DRIVER object ---- */
    PIO_DEVICE_OBJECT PreviousDeviceObject; /* Device object of this IRP before it is forwarded
					     * (this is called ForwardedFrom in irp.c). The device
					     * object in the IRP is always from the driver currently
					     * handling it (ie. the device it has been forwarded to).
					     * This is only valid if Requestor is a DRIVER object. */
} PENDING_IRP, *PPENDING_IRP;

static inline NTSTATUS IopAllocateIoPacket(IN IO_PACKET_TYPE Type,
					   IN ULONG Size,
					   OUT PIO_PACKET *pIoPacket)
{
    assert(pIoPacket != NULL);
    assert(Size >= sizeof(IO_PACKET));
    ExAllocatePoolEx(IoPacket, IO_PACKET, Size, NTOS_IO_TAG, {});
    IoPacket->Type = Type;
    IoPacket->Size = Size;
    *pIoPacket = IoPacket;
    return STATUS_SUCCESS;
}

static inline NTSTATUS IopAllocatePendingIrp(IN PIO_PACKET IoPacket,
					     IN POBJECT Requestor,
					     OUT PPENDING_IRP *pPendingIrp)
{
    assert(IoPacket != NULL);
    assert(Requestor != NULL);
    assert(pPendingIrp != NULL);
    IopAllocatePool(PendingIrp, PENDING_IRP);
    PendingIrp->IoPacket = IoPacket;
    PendingIrp->Requestor = Requestor;
    PendingIrp->PreviousDeviceObject = NULL;
    assert(ObObjectIsType(Requestor, OBJECT_TYPE_DRIVER) ||
	   ObObjectIsType(Requestor, OBJECT_TYPE_THREAD));
    *pPendingIrp = PendingIrp;
    return STATUS_SUCCESS;
}

static inline VOID IopFreeIoResponseData(IN PPENDING_IRP PendingIrp)
{
    if (PendingIrp->IoResponseData != NULL) {
	ExFreePool(PendingIrp->IoResponseData);
	PendingIrp->IoResponseData = NULL;
    }
}

/*
 * Detach the given pending IRP from the thread that it has been queued on.
 * Free the IO response data, and delete both the IO packet struct and the
 * PENDING_IRP struct itself.
 */
static inline VOID IopCleanupPendingIrp(IN PPENDING_IRP PendingIrp)
{
    RemoveEntryList(&PendingIrp->Link);
    IopFreeIoResponseData(PendingIrp);
    if (PendingIrp->IoPacket != NULL) {
	ExFreePool(PendingIrp->IoPacket);
    }
    ExFreePool(PendingIrp);
}

static inline VOID IopCleanupPendingIrpList(IN PTHREAD Thread)
{
    LoopOverList(PendingIrp, &Thread->PendingIrpList, PENDING_IRP, Link) {
	IopCleanupPendingIrp(PendingIrp);
    }
}

/*
 * Returns the IO_PACKET queued in the driver object's PendingIoPacketList,
 * given the handle pair (OriginalRequestor, IrpIdentifier) that uniquely
 * identifies the IO_PACKET (currently).
 */
static inline PIO_PACKET IopLocateIrpInDriverPendingList(IN PIO_DRIVER_OBJECT DriverObject,
							 IN GLOBAL_HANDLE OriginalRequestor,
							 IN HANDLE IrpIdentifier)
{
    PIO_PACKET CompletedIrp = NULL;
    LoopOverList(PendingIoPacket, &DriverObject->PendingIoPacketList, IO_PACKET, IoPacketLink) {
	/* The IO packets in the pending IO packet list must be of type IoPacketTypeRequest.
	 * We never put IoPacketTypeClientMessage etc into the pending IO packet list */
	assert(PendingIoPacket->Type == IoPacketTypeRequest);
	if ((PendingIoPacket->Request.OriginalRequestor == OriginalRequestor) &&
	    (PendingIoPacket->Request.Identifier == IrpIdentifier)) {
	    CompletedIrp = PendingIoPacket;
	}
    }
    return CompletedIrp;
}

/*
 * Returns the PENDING_IRP struct of the given IoPacket. Note that for driver
 * objects this searches the ForwardedIrpList not the PendingIoPacketList.
 * The latter is used for a different purpose (see io.h).
 */
static inline PPENDING_IRP IopLocateIrpInOriginalRequestor(IN GLOBAL_HANDLE OriginalRequestor,
							   IN PIO_PACKET IoPacket)
{
    POBJECT RequestorObject = GLOBAL_HANDLE_TO_OBJECT(OriginalRequestor);
    if (ObObjectIsType(RequestorObject, OBJECT_TYPE_THREAD)) {
	PTHREAD Thread = (PTHREAD)RequestorObject;
	LoopOverList(PendingIrp, &Thread->PendingIrpList, PENDING_IRP, Link) {
	    if (PendingIrp->IoPacket == IoPacket) {
		assert(PendingIrp->Requestor == RequestorObject);
		return PendingIrp;
	    }
	}
    } else if (ObObjectIsType(RequestorObject, OBJECT_TYPE_DRIVER)) {
	PIO_DRIVER_OBJECT Driver = (PIO_DRIVER_OBJECT)RequestorObject;
	LoopOverList(PendingIrp, &Driver->ForwardedIrpList, PENDING_IRP, Link) {
	    if (PendingIrp->IoPacket == IoPacket) {
		assert(PendingIrp->Requestor == RequestorObject);
		return PendingIrp;
	    }
	}
    } else {
	assert(FALSE);
    }
    return NULL;
}

static inline VOID IopDumpDriverPendingIoPacketList(IN PIO_DRIVER_OBJECT DriverObject,
						    IN GLOBAL_HANDLE OriginalRequestor,
						    IN HANDLE IrpIdentifier)
{
    DbgTrace("Received response packet from driver %s with invalid IRP identifier %p:%p."
	     " Dumping all IO packets in the driver's pending IO packet list.\n",
	     DriverObject->DriverImagePath, (PVOID)OriginalRequestor, IrpIdentifier);
    LoopOverList(PendingIoPacket, &DriverObject->PendingIoPacketList, IO_PACKET, IoPacketLink) {
	IoDbgDumpIoPacket(PendingIoPacket, FALSE);
    }
}

/*
 * DO NOT call this directly!
 */
static inline VOID IopQueueIoPacketEx(IN PPENDING_IRP PendingIrp,
				      IN PIO_DRIVER_OBJECT Driver,
				      IN PTHREAD Thread)
{
    /* Queue the IRP to the driver */
    InsertTailList(&Driver->IoPacketQueue, &PendingIrp->IoPacket->IoPacketLink);
    PendingIrp->IoPacket->Request.OriginalRequestor = OBJECT_TO_GLOBAL_HANDLE(Thread);
    /* Use the GLOBAL_HANDLE of the IoPacket as the Identifier */
    PendingIrp->IoPacket->Request.Identifier = (HANDLE)POINTER_TO_GLOBAL_HANDLE(PendingIrp->IoPacket);
    InsertTailList(&Thread->PendingIrpList, &PendingIrp->Link);
    KeInitializeEvent(&PendingIrp->IoCompletionEvent, NotificationEvent);
    KeSetEvent(&Driver->IoPacketQueuedEvent);
}

static inline VOID IopQueueIoPacket(IN PPENDING_IRP PendingIrp,
				    IN PTHREAD Thread)
{
    /* We can only queue IO request packets */
    assert(PendingIrp != NULL);
    assert(PendingIrp->IoPacket != NULL);
    assert(PendingIrp->IoPacket->Type == IoPacketTypeRequest);
    assert(PendingIrp->IoPacket->Request.MajorFunction != IRP_MJ_ADD_DEVICE);
    PIO_DRIVER_OBJECT Driver = PendingIrp->IoPacket->Request.Device.Object->DriverObject;
    assert(Driver != NULL);
    IopQueueIoPacketEx(PendingIrp, Driver, Thread);
}

/*
 * For AddDevice request we use the Device member of the IRP to store
 * the handle to the physical device object, which will (usually) have
 * a different driver object than the one we are sending it to.
 */
static inline VOID IopQueueAddDeviceRequest(IN PPENDING_IRP PendingIrp,
					    IN PIO_DRIVER_OBJECT Driver,
					    IN PTHREAD Thread)
{
    /* We can only queue AddDevice request packets */
    assert(PendingIrp != NULL);
    assert(PendingIrp->IoPacket != NULL);
    assert(PendingIrp->IoPacket->Type == IoPacketTypeRequest);
    assert(PendingIrp->IoPacket->Request.MajorFunction == IRP_MJ_ADD_DEVICE);
    assert(Driver != NULL);
    IopQueueIoPacketEx(PendingIrp, Driver, Thread);
}

static inline BOOLEAN IopFileIsSynchronous(IN PIO_FILE_OBJECT File)
{
    return !!(File->Flags & FO_SYNCHRONOUS_IO);
}

typedef enum _CREATE_FILE_TYPE {
    CreateFileTypeNone,
    CreateFileTypeNamedPipe,
    CreateFileTypeMailslot
} CREATE_FILE_TYPE;

/*
 * An open packet is used as a context for opening a Device object so
 * the device open routine can know what operation is being requested.
 */
typedef struct _OPEN_PACKET {
    CREATE_FILE_TYPE CreateFileType;
    ULONG CreateOptions;
    ULONG FileAttributes;
    ULONG ShareAccess;
    ULONG Disposition;
    union {
	PNAMED_PIPE_CREATE_PARAMETERS NamedPipeCreateParameters;
	PMAILSLOT_CREATE_PARAMETERS MailslotCreateParameters;
    };
} OPEN_PACKET, *POPEN_PACKET;

/*
 * Extension of the OB_PARSE_CONTEXT.
 */
typedef struct _IO_OPEN_CONTEXT {
    IN OB_PARSE_CONTEXT Header;
    IN OPEN_PACKET OpenPacket;
    OUT ULONG_PTR Information; /* IO_STATUS_BLOCK.Information returned by the driver call */
} IO_OPEN_CONTEXT, *PIO_OPEN_CONTEXT;

/*
 * Creation context for the driver object creation routine
 */
typedef struct _DRIVER_OBJ_CREATE_CONTEXT {
    PCSTR DriverImagePath;
    PCSTR DriverServicePath;
    PCSTR DriverName;
} DRIVER_OBJ_CREATE_CONTEXT, *PDRIVER_OBJ_CREATE_CONTEXT;

/*
 * Creation context for the device object creation routine
 */
typedef struct _DEVICE_OBJ_CREATE_CONTEXT {
    PIO_DRIVER_OBJECT DriverObject;
    PCSTR DeviceName;
    IO_DEVICE_INFO DeviceInfo;
    BOOLEAN Exclusive;
} DEVICE_OBJ_CREATE_CONTEXT, *PDEVICE_OBJ_CREATE_CONTEXT;

/*
 * Creation context for the file object creation routine
 */
typedef struct _FILE_OBJ_CREATE_CONTEXT {
    PCSTR FileName;
    PIO_DEVICE_OBJECT DeviceObject;
    PVOID BufferPtr;
    MWORD FileSize;
} FILE_OBJ_CREATE_CONTEXT, *PFILE_OBJ_CREATE_CONTEXT;

/*
 * Worker thread of a driver object
 */
typedef struct _WORKER_THREAD {
    PTHREAD Thread;
    NOTIFICATION Notification;	/* Client side capability */
} WORKER_THREAD, *PWORKER_THREAD;

/*
 * Interrupt service of a driver object
 */
typedef struct _INTERRUPT_SERVICE {
    ULONG Vector;
    LIST_ENTRY Link;
    IRQ_HANDLER IrqHandler;
    PTHREAD IsrThread;
    CAP_TREE_NODE IsrThreadClientCap; /* Client side cap of the ISR thread cap */
    NOTIFICATION Notification;	      /* Client side capability */
    NOTIFICATION InterruptMutex;      /* Client side capability */
} INTERRUPT_SERVICE, *PINTERRUPT_SERVICE;

/* init.c */
extern LIST_ENTRY IopDriverList;

/*
 * Returns the device object of the given device handle, optionally
 * checking whether the device belongs to the driver object
 */
static inline PIO_DEVICE_OBJECT IopGetDeviceObject(IN GLOBAL_HANDLE DeviceHandle,
						   IN PIO_DRIVER_OBJECT DriverObject)
{
    if (DeviceHandle == 0) {
	return NULL;
    }
    /* If driver object is not NULL, only check its device for a match */
    if (DriverObject != NULL) {
	LoopOverList(DevObj, &DriverObject->DeviceList, IO_DEVICE_OBJECT, DeviceLink) {
	    if (DevObj == GLOBAL_HANDLE_TO_OBJECT(DeviceHandle)) {
		return DevObj;
	    }
	}
    } else {
	/* Traverse the list of all driver objects, and check if there is a match */
	LoopOverList(DrvObj, &IopDriverList, IO_DRIVER_OBJECT, DriverLink) {
	    LoopOverList(DevObj, &DrvObj->DeviceList, IO_DEVICE_OBJECT, DeviceLink) {
		if (DevObj == GLOBAL_HANDLE_TO_OBJECT(DeviceHandle)) {
		    return DevObj;
		}
	    }
	}
    }
    return NULL;
}

static inline PIO_DEVICE_OBJECT IopGetTopDevice(IN PIO_DEVICE_OBJECT Device)
{
    assert(Device != NULL);
    while (Device->AttachedDevice != NULL) {
	Device = Device->AttachedDevice;
    }
    return Device;
}

/* irp.c */
NTSTATUS IopThreadWaitForIoCompletion(IN ASYNC_STATE State,
				      IN PTHREAD Thread,
				      IN BOOLEAN Alertable,
				      IN WAIT_TYPE WaitType);
NTSTATUS IopMapIoBuffers(IN PPENDING_IRP PendingIrp,
			 IN BOOLEAN OutputIsReadOnly);

/* file.c */
extern LIST_ENTRY IopFileObjectList;
NTSTATUS IopFileObjectCreateProc(IN POBJECT Object,
				 IN PVOID CreaCtx);
NTSTATUS IopFileObjectOpenProc(IN ASYNC_STATE State,
			       IN PTHREAD Thread,
			       IN POBJECT Object,
			       IN PCSTR SubPath,
			       IN POB_PARSE_CONTEXT ParseContext,
			       OUT POBJECT *pOpenedInstance,
			       OUT PCSTR *pRemainingPath);
NTSTATUS IopCreateFileObject(IN PCSTR FileName,
			     IN PIO_DEVICE_OBJECT DeviceObject,
			     IN PVOID BufferPtr,
			     IN MWORD FileSize,
			     OUT PIO_FILE_OBJECT *pFile);

/*
 * Returns the file object of the given global handle
 */
static inline PIO_FILE_OBJECT IopGetFileObject(IN GLOBAL_HANDLE Handle)
{
    if (Handle == 0) {
	return NULL;
    }
    LoopOverList(FileObj, &IopFileObjectList, IO_FILE_OBJECT, Link) {
	if (FileObj == GLOBAL_HANDLE_TO_OBJECT(Handle)) {
	    return FileObj;
	}
    }
    return NULL;
}

/* device.c */
NTSTATUS IopDeviceObjectCreateProc(IN POBJECT Object,
				   IN PVOID CreaCtx);
NTSTATUS IopDeviceObjectOpenProc(IN ASYNC_STATE State,
				 IN PTHREAD Thread,
				 IN POBJECT Object,
				 IN PCSTR SubPath,
				 IN POB_PARSE_CONTEXT ParseContext,
				 OUT POBJECT *pOpenedInstance,
				 OUT PCSTR *pRemainingPath);

/* driver.c */
NTSTATUS IopDriverObjectCreateProc(POBJECT Object,
				   IN PVOID CreaCtx);
NTSTATUS IopLoadDriver(IN PCSTR DriverServicePath,
		       OUT OPTIONAL PIO_DRIVER_OBJECT *pDriverObject);
