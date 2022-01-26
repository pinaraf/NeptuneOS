#include "iop.h"

NTSTATUS IopDeviceObjectCreateProc(IN POBJECT Object,
				   IN PVOID CreaCtx)
{
    PIO_DEVICE_OBJECT Device = (PIO_DEVICE_OBJECT) Object;
    PDEVICE_OBJ_CREATE_CONTEXT Ctx = (PDEVICE_OBJ_CREATE_CONTEXT) CreaCtx;
    PIO_DRIVER_OBJECT DriverObject = Ctx->DriverObject;
    PCSTR DeviceName = Ctx->DeviceName;
    IO_DEVICE_INFO DeviceInfo = Ctx->DeviceInfo;
    BOOLEAN Exclusive = Ctx->Exclusive;
    InitializeListHead(&Device->DeviceLink);

    /* If the client has supplied a non-empty DeviceName, record it */
    if ((DeviceName != NULL) && (DeviceName[0] != '\0')) {
	/* DeviceName must be a full path */
	if (DeviceName[0] != OBJ_NAME_PATH_SEPARATOR) {
	    return STATUS_INVALID_PARAMETER_1;
	}
	PCSTR DeviceNameCopy = RtlDuplicateString(DeviceName, NTOS_IO_TAG);
	if (DeviceNameCopy == NULL) {
	    return STATUS_NO_MEMORY;
	}
	/* If the client has supplied a device name, insert the device
	 * object into the global object namespace, with the given path. */
	RET_ERR_EX(ObInsertObjectByPath(DeviceName, Device),
		   ExFreePool(DeviceNameCopy));
	Device->DeviceName = DeviceNameCopy;
    }

    Device->DriverObject = DriverObject;
    InsertTailList(&DriverObject->DeviceList, &Device->DeviceLink);
    Device->DeviceInfo = DeviceInfo;
    Device->Exclusive = Exclusive;

    return STATUS_SUCCESS;
}

NTSTATUS IopDeviceObjectOpenProc(IN ASYNC_STATE State,
				 IN PTHREAD Thread,
				 IN POBJECT Object,
				 IN PCSTR SubPath,
				 IN POB_PARSE_CONTEXT ParseContext,
				 OUT POBJECT *pOpenedInstance,
				 OUT PCSTR *pRemainingPath)
{
    assert(Thread != NULL);
    assert(Object != NULL);
    assert(SubPath != NULL);
    assert(ParseContext != NULL);
    assert(pOpenedInstance != NULL);

    /* We haven't implemented file system devices yet */
    *pRemainingPath = SubPath;
    if (*SubPath != '\0') {
	UNIMPLEMENTED;
    }

    /* These must come before ASYNC_BEGIN since they are referenced
     * throughout the whole function (in particular, after the AWAIT call) */
    PIO_DEVICE_OBJECT Device = (PIO_DEVICE_OBJECT)Object;
    PIO_DRIVER_OBJECT Driver = Device->DriverObject;
    PIO_OPEN_CONTEXT OpenContext = (PIO_OPEN_CONTEXT)ParseContext;
    POPEN_PACKET OpenPacket = &OpenContext->OpenPacket;
    assert(Driver != NULL);

    ASYNC_BEGIN(State, Locals, {
	    PPENDING_IRP PendingIrp;
	});

    /* Reject the open if the parse context is not IO_OPEN_CONTEXT */
    if (ParseContext->Type != PARSE_CONTEXT_DEVICE_OPEN) {
	return STATUS_OBJECT_TYPE_MISMATCH;
    }

    PIO_FILE_OBJECT FileObject = NULL;
    RET_ERR(IopCreateFileObject(Device->DeviceName, Device, NULL, 0, &FileObject));
    assert(FileObject != NULL);

    /* Check if this is Synch I/O and set the sync flag accordingly */
    if (OpenPacket->CreateOptions & (FILE_SYNCHRONOUS_IO_ALERT | FILE_SYNCHRONOUS_IO_NONALERT)) {
	FileObject->Flags |= FO_SYNCHRONOUS_IO;
	/* Check if it's also alertable and set the alertable flag accordingly */
	if (OpenPacket->CreateOptions & FILE_SYNCHRONOUS_IO_ALERT) {
	    FileObject->Flags |= FO_ALERTABLE_IO;
	}
    }

    PIO_PACKET IoPacket = NULL;
    ULONG FileNameLength = strlen(FileObject->FileName);
    ULONG IoPacketSize = sizeof(IO_PACKET) + FileNameLength + 1;
    RET_ERR_EX(IopAllocateIoPacket(IoPacketTypeRequest, IoPacketSize, &IoPacket),
	       ObDereferenceObject(FileObject));
    assert(IoPacket != NULL);

    /* For IO packets involving the creation of file objects, we need to pass FILE_OBJECT_CREATE_PARAMETERS
     * so the client can record the file object information there */
    memcpy(IoPacket+1, FileObject->FileName, FileNameLength + 1);
    FILE_OBJECT_CREATE_PARAMETERS FileObjectParameters = {
	.ReadAccess = FileObject->ReadAccess,
	.WriteAccess = FileObject->WriteAccess,
	.DeleteAccess = FileObject->DeleteAccess,
	.SharedRead = FileObject->SharedRead,
	.SharedWrite = FileObject->SharedWrite,
	.SharedDelete = FileObject->SharedDelete,
	.Flags = FileObject->Flags,
	.FileNameOffset = sizeof(IO_PACKET)
    };

    if (OpenPacket->CreateFileType == CreateFileTypeNone) {
	IoPacket->Request.MajorFunction = IRP_MJ_CREATE;
	IoPacket->Request.Parameters.Create.Options = OpenPacket->CreateOptions;
	IoPacket->Request.Parameters.Create.FileAttributes = OpenPacket->FileAttributes;
	IoPacket->Request.Parameters.Create.ShareAccess = OpenPacket->ShareAccess;
	IoPacket->Request.Parameters.Create.FileObjectParameters = FileObjectParameters;
    } else if (OpenPacket->CreateFileType == CreateFileTypeNamedPipe) {
	IoPacket->Request.MajorFunction = IRP_MJ_CREATE_NAMED_PIPE;
	IoPacket->Request.Parameters.CreatePipe.Options = OpenPacket->CreateOptions;
	IoPacket->Request.Parameters.CreatePipe.ShareAccess = OpenPacket->ShareAccess;
	IoPacket->Request.Parameters.CreatePipe.Parameters = *(OpenPacket->NamedPipeCreateParameters);
	IoPacket->Request.Parameters.CreatePipe.FileObjectParameters = FileObjectParameters;
    } else {
	assert(OpenPacket->CreateFileType == CreateFileTypeMailslot);
	IoPacket->Request.MajorFunction = IRP_MJ_CREATE_MAILSLOT;
	IoPacket->Request.Parameters.CreateMailslot.Options = OpenPacket->CreateOptions;
	IoPacket->Request.Parameters.CreateMailslot.ShareAccess = OpenPacket->ShareAccess;
	IoPacket->Request.Parameters.CreateMailslot.Parameters = *(OpenPacket->MailslotCreateParameters);
	IoPacket->Request.Parameters.CreateMailslot.FileObjectParameters = FileObjectParameters;
    }

    IoPacket->Request.Device.Object = Device;
    IoPacket->Request.File.Object = FileObject;
    RET_ERR_EX(IopAllocatePendingIrp(IoPacket, &Locals.PendingIrp),
	       {
		   ExFreePool(IoPacket);
		   ObDereferenceObject(FileObject);
	       });
    IopQueueIoPacket(Locals.PendingIrp, Driver, Thread);

    /* For create/open we always wait till the driver has completed the request. */
    AWAIT(KeWaitForSingleObject, State, Locals, Thread,
	  &Locals.PendingIrp->IoCompletionEvent.Header, FALSE);

    /* This is the starting point when the function is resumed.
     * Note: The local variable IoPacket is undefined here. IoPacket in the previous async block
     * is stored as Thread's PendingIoPacket */
    PIO_FILE_OBJECT FileObject = Locals.PendingIrp->IoPacket->Request.File.Object;
    assert(FileObject != NULL);

    IO_STATUS_BLOCK IoStatus = Locals.PendingIrp->IoResponseStatus;
    OpenContext->Information = IoStatus.Information;
    NTSTATUS Status = IoStatus.Status;

    if (NT_SUCCESS(Status)) {
	*pOpenedInstance = FileObject;
    } else {
	/* The IO request has returned a error status. Clean up the file object. */
	ObDereferenceObject(FileObject);
    }

    /* This will free the pending IRP and detach the pending irp from the thread.
     * At this point the IRP has already been detached from the driver object,
     * so we do not need to remove it from the driver IRP queue here. */
    IopCleanupPendingIrp(Locals.PendingIrp);
    ASYNC_END(Status);
}

/*
 * Note: DeviceName must be a full path.
 */
NTSTATUS IopCreateDevice(IN ASYNC_STATE State,
			 IN PTHREAD Thread,
                         IN OPTIONAL PCSTR DeviceName,
                         IN PIO_DEVICE_INFO DeviceInfo,
                         IN BOOLEAN Exclusive,
                         OUT GLOBAL_HANDLE *DeviceHandle)
{
    assert(Thread != NULL);
    assert(Thread->Process->DriverObject != NULL);
    assert(DeviceHandle != NULL);

    PIO_DEVICE_OBJECT DeviceObject = NULL;
    DEVICE_OBJ_CREATE_CONTEXT CreaCtx = {
	.DriverObject = Thread->Process->DriverObject,
	.DeviceName = DeviceName,
	.DeviceInfo = *DeviceInfo,
	.Exclusive = Exclusive
    };
    RET_ERR(ObCreateObject(OBJECT_TYPE_DEVICE, (POBJECT *) &DeviceObject, &CreaCtx));
    assert(DeviceObject != NULL);

    *DeviceHandle = OBJECT_TO_GLOBAL_HANDLE(DeviceObject);

    return STATUS_SUCCESS;
}

NTSTATUS IopIoAttachDeviceToDeviceStack(IN ASYNC_STATE AsyncState,
                                        IN PTHREAD Thread,
                                        IN GLOBAL_HANDLE SourceDeviceHandle,
                                        IN GLOBAL_HANDLE TargetDeviceHandle,
                                        OUT GLOBAL_HANDLE *PreviousTopDeviceHandle,
                                        OUT IO_DEVICE_INFO *PreviousTopDeviceInfo)
{
    assert(Thread->Process != NULL);
    PIO_DRIVER_OBJECT DriverObject = Thread->Process->DriverObject;
    assert(DriverObject != NULL);
    PIO_DEVICE_OBJECT SrcDev = IopGetDeviceObject(SourceDeviceHandle, DriverObject);
    PIO_DEVICE_OBJECT TgtDev = IopGetDeviceObject(TargetDeviceHandle, NULL);
    if (SrcDev == NULL || TgtDev == NULL) {
	/* This shouldn't happen since we check the validity of device handles before */
	assert(FALSE);
	return STATUS_INVALID_PARAMETER;
    }
    PIO_DEVICE_OBJECT PrevTop = TgtDev;
    while (PrevTop->AttachedDevice != NULL) {
	PrevTop = PrevTop->AttachedDevice;
    }
    /* Detech the source device from the previous device stack (if any) */
    if (SrcDev->AttachedTo != NULL) {
	assert(SrcDev->AttachedTo->AttachedDevice == SrcDev);
	SrcDev->AttachedTo->AttachedDevice = NULL;
    }
    SrcDev->AttachedTo = PrevTop;
    PrevTop->AttachedDevice = SrcDev;
    *PreviousTopDeviceHandle = POINTER_TO_GLOBAL_HANDLE(PrevTop);
    *PreviousTopDeviceInfo = PrevTop->DeviceInfo;
    return STATUS_SUCCESS;
}

NTSTATUS IopGetAttachedDevice(IN ASYNC_STATE AsyncState,
                              IN PTHREAD Thread,
                              IN GLOBAL_HANDLE DeviceHandle,
                              OUT GLOBAL_HANDLE *TopDeviceHandle,
                              OUT IO_DEVICE_INFO *TopDeviceInfo)
{
    assert(Thread->Process != NULL);
    PIO_DRIVER_OBJECT DriverObject = Thread->Process->DriverObject;
    assert(DriverObject != NULL);
    PIO_DEVICE_OBJECT Device = IopGetDeviceObject(DeviceHandle, DriverObject);
    while (Device->AttachedDevice != NULL) {
	Device = Device->AttachedDevice;
    }
    *TopDeviceHandle = POINTER_TO_GLOBAL_HANDLE(Device);
    *TopDeviceInfo = Device->DeviceInfo;
    return STATUS_SUCCESS;
}

NTSTATUS IoSetDeviceInterfaceState(IN ASYNC_STATE AsyncState,
                                   IN PTHREAD Thread,
                                   IN PCSTR SymbolicLinkName,
                                   IN BOOLEAN Enable)
{
    UNIMPLEMENTED;
}

NTSTATUS NtDeviceIoControlFile(IN ASYNC_STATE State,
			       IN PTHREAD Thread,
                               IN HANDLE FileHandle,
                               IN HANDLE Event,
                               IN PIO_APC_ROUTINE ApcRoutine,
                               IN PVOID ApcContext,
                               OUT IO_STATUS_BLOCK *IoStatusBlock,
                               IN ULONG Ioctl,
                               IN PVOID InputBuffer,
                               IN ULONG InputBufferLength,
                               IN PVOID OutputBuffer,
                               IN ULONG OutputBufferLength)
{
    assert(Thread != NULL);
    assert(Thread->Process != NULL);

    ASYNC_BEGIN(State, Locals, {
	    PPENDING_IRP PendingIrp;
	});

    if (FileHandle == NULL) {
	return STATUS_INVALID_HANDLE;
    }
    PIO_FILE_OBJECT FileObject = NULL;
    RET_ERR(ObReferenceObjectByHandle(Thread->Process, FileHandle,
				      OBJECT_TYPE_FILE, (POBJECT *)&FileObject));
    assert(FileObject != NULL);
    assert(FileObject->DeviceObject != NULL);
    assert(FileObject->DeviceObject->DriverObject != NULL);
    PIO_DRIVER_OBJECT DriverObject = FileObject->DeviceObject->DriverObject;

    PIO_PACKET IoPacket = NULL;
    RET_ERR_EX(IopAllocateIoPacket(IoPacketTypeRequest, sizeof(IO_PACKET), &IoPacket),
	       ObDereferenceObject(FileObject));
    assert(IoPacket != NULL);

    IoPacket->Request.MajorFunction = IRP_MJ_DEVICE_CONTROL;
    IoPacket->Request.MinorFunction = 0;
    IoPacket->Request.Control = 0;
    IoPacket->Request.Flags = 0;
    IoPacket->Request.Device.Object = FileObject->DeviceObject;
    IoPacket->Request.File.Object = FileObject;

    MWORD DriverInputBuffer = 0;
    if ((InputBuffer != NULL) && (InputBufferLength != 0)) {
	RET_ERR_EX(IopMapUserBuffer(Thread->Process, DriverObject,
				    (MWORD) InputBuffer, InputBufferLength,
				    &DriverInputBuffer, TRUE),
		   {
		       ObDereferenceObject(FileObject);
		       ExFreePool(IoPacket);
		   });
    }

    MWORD DriverOutputBuffer = 0;
    if ((OutputBuffer != NULL) && (OutputBufferLength != 0)) {
	BOOLEAN MapReadOnly = METHOD_FROM_CTL_CODE(Ioctl) == METHOD_IN_DIRECT;
	RET_ERR_EX(IopMapUserBuffer(Thread->Process, DriverObject,
				    (MWORD) OutputBuffer, OutputBufferLength,
				    &DriverOutputBuffer, MapReadOnly),
		   {
		       ObDereferenceObject(FileObject);
		       ExFreePool(IoPacket);
		       if (DriverInputBuffer != 0) {
			   IopUnmapUserBuffer(DriverObject, DriverInputBuffer);
		       }
		   });
    }

    IoPacket->Request.Parameters.DeviceIoControl.InputBuffer = (PVOID)DriverInputBuffer;
    IoPacket->Request.Parameters.DeviceIoControl.OutputBuffer = (PVOID)DriverOutputBuffer;
    IoPacket->Request.Parameters.DeviceIoControl.InputBufferLength = InputBufferLength;
    IoPacket->Request.Parameters.DeviceIoControl.OutputBufferLength = OutputBufferLength;
    IoPacket->Request.Parameters.DeviceIoControl.IoControlCode = Ioctl;

    RET_ERR_EX(IopAllocatePendingIrp(IoPacket, &Locals.PendingIrp),
	       {
		   ObDereferenceObject(FileObject);
		   ExFreePool(IoPacket);
		   if (DriverInputBuffer != 0) {
		       IopUnmapUserBuffer(DriverObject, DriverInputBuffer);
		   }
		   if (DriverOutputBuffer != 0) {
		       IopUnmapUserBuffer(DriverObject, DriverOutputBuffer);
		   }
	       });
    IopQueueIoPacket(Locals.PendingIrp, DriverObject, Thread);

    /* Only wait for the IO completion if file is opened with
     * the synchronize flag. Otherwise, return pending status */
    if (!IopFileIsSynchronous(FileObject)) {
	/* TODO: Event, APC and IO completion port... */
	return STATUS_PENDING;
    }
    AWAIT_IF(IopFileIsSynchronous(FileObject), KeWaitForSingleObject, State,
	     Locals, Thread, &Locals.PendingIrp->IoCompletionEvent.Header, FALSE);

    /* This is the starting point when the function is resumed. */
    PIO_FILE_OBJECT FileObject = Locals.PendingIrp->IoPacket->Request.File.Object;
    assert(FileObject != NULL);

    NTSTATUS Status = Locals.PendingIrp->IoResponseStatus.Status;

    if (!NT_SUCCESS(Status)) {
	/* The IO request has returned a error status. Clean up the file object. */
	ObDereferenceObject(FileObject);
    }

    /* This will free the pending IRP and detach the pending irp from the thread.
     * At this point the IRP has already been detached from the driver object,
     * so we do not need to remove it from the driver IRP queue here. */
    IopCleanupPendingIrp(Locals.PendingIrp);
    ASYNC_END(Status);
}
