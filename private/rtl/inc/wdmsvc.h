#pragma once

#include <nt.h>
#include <services.h>
#include <wdmsvc_gen.h>

compile_assert(TOO_MANY_WDM_SERVICES, NUMBER_OF_WDM_SERVICES < 0x1000UL);

#define DRIVER_IO_PACKET_BUFFER_RESERVE	(64 * 1024)
#define DRIVER_IO_PACKET_BUFFER_COMMIT	(8 * 1024)

#define PNP_ROOT_BUS_DRIVER	"\\Driver\\pnp"
#define PNP_ROOT_ENUMERATOR	"\\Device\\pnp"

/*
 * Global handle type.
 *
 * See ntos/inc/ob.h
 */
typedef MWORD GLOBAL_HANDLE, *PGLOBAL_HANDLE;

/* Make sure the struct packing matches on both ELF and PE targets */
#include <pshpack4.h>

/*
 * Device object information shared between server and client drivers
 */
typedef struct _IO_DEVICE_INFO {
    DEVICE_TYPE DeviceType;
    ULONG DeviceCharacteristics;
} IO_DEVICE_INFO, *PIO_DEVICE_INFO;

/*
 * Parameters for NtCreateMailslotFile/NtCreateNamedPipeFile
 */
typedef struct _MAILSLOT_CREATE_PARAMETERS {
    ULONG MailslotQuota;
    ULONG MaximumMessageSize;
    LARGE_INTEGER ReadTimeout;
    BOOLEAN TimeoutSpecified;
} MAILSLOT_CREATE_PARAMETERS, *PMAILSLOT_CREATE_PARAMETERS;

typedef struct _NAMED_PIPE_CREATE_PARAMETERS {
    ULONG NamedPipeType;
    ULONG ReadMode;
    ULONG CompletionMode;
    ULONG MaximumInstances;
    ULONG InboundQuota;
    ULONG OutboundQuota;
    LARGE_INTEGER DefaultTimeout;
    BOOLEAN TimeoutSpecified;
} NAMED_PIPE_CREATE_PARAMETERS, *PNAMED_PIPE_CREATE_PARAMETERS;

/*
 * Parameters for CREATE, CREATE_MAILSLOT, and CREATE_NAMED_PIPE
 */
typedef struct _FILE_OBJECT_CREATE_PARAMETERS {
    BOOLEAN ReadAccess;
    BOOLEAN WriteAccess;
    BOOLEAN DeleteAccess;
    BOOLEAN SharedRead;
    BOOLEAN SharedWrite;
    BOOLEAN SharedDelete;
    ULONG Flags;
    ULONG FileNameOffset; /* Offset to the NUL-terminated file name buffer,
			   * starting from the beginning of the IO packet. */
} FILE_OBJECT_CREATE_PARAMETERS, *PFILE_OBJECT_CREATE_PARAMETERS;

/*
 * IO packet type that is being sent between NTOS server and drivers
 */
typedef enum _IO_PACKET_TYPE {
    IoPacketTypeRequest, /* An IO request packet, from either an NT client or a driver */
    IoPacketTypeServerMessage, /* Message sent from the server */
    IoPacketTypeClientMessage  /* Message to the server from the client */
} IO_PACKET_TYPE;

/*
 * Declares a union type of server-side pointer and
 * client-side global handle
 */
#define DECLARE_POINTER_TYPE(Type)		\
    typedef union _ ## Type ## _PTR {		\
	struct _ ## Type *Object;		\
	GLOBAL_HANDLE Handle;			\
    } Type ## _PTR

DECLARE_POINTER_TYPE(THREAD);
DECLARE_POINTER_TYPE(IO_DEVICE_OBJECT);
DECLARE_POINTER_TYPE(IO_FILE_OBJECT);

#undef DECLARE_POINTER_TYPE

/*
 * Parameters for an IO request.
 */
typedef struct _IO_REQUEST_PARAMETERS {
    UCHAR MajorFunction;
    UCHAR MinorFunction;
    UCHAR Flags;
    UCHAR Control;
    IO_DEVICE_OBJECT_PTR Device;
    IO_FILE_OBJECT_PTR File;
    THREAD_PTR OriginatingThread; /* Originating thread. This is used to disambiguate the IRP
				   * identifier below. The system guarantees the uniqueness
				   * of the IRP identifier among all IRPs currently being
				   * requested by a particular thread. For (new) IRPs requested
				   * by a driver object this should be set to NULL. */
    HANDLE Identifier; /* Handle to identify the original IRP. For IRPs generated by the NTOS
			* Executive, this handle is the GLOBAL_HANDLE of the IO packet (allocated
			* on the ExPool) queued on a thread object, and is unique among all IO
			* packets currently being processed by the system. For (new) IRPs generated
			* by a driver, this handle is assigned by the driver. The driver must
			* guarantee that it is unique among all IRPs currently being requested by
			* the driver. Both the system and the driver may reuse old handles of IO
			* packets that have already been processed. */
    union {
	struct {
	    FILE_OBJECT_CREATE_PARAMETERS FileObjectParameters;
	    ULONG Options;
	    ULONG FileAttributes;
	    ULONG ShareAccess;
	} Create;
	struct {
	    FILE_OBJECT_CREATE_PARAMETERS FileObjectParameters;
	    ULONG Options;
	    USHORT ShareAccess;
	    NAMED_PIPE_CREATE_PARAMETERS Parameters;
	} CreatePipe;
	struct {
	    FILE_OBJECT_CREATE_PARAMETERS FileObjectParameters;
	    ULONG Options;
	    USHORT ShareAccess;
	    MAILSLOT_CREATE_PARAMETERS Parameters;
	} CreateMailslot;
	struct {
	    ULONG Length;
	    ULONG Key;
	    LARGE_INTEGER ByteOffset;
	} Read;
	struct {
	    ULONG Length;
	    ULONG Key;
	    LARGE_INTEGER ByteOffset;
	} Write;
	struct {
	    PVOID InputBuffer;	/* Client-side pointer! */
	    PVOID OutputBuffer;	/* Client-side pointer! */
	    ULONG InputBufferLength;
	    ULONG OutputBufferLength;
	    ULONG IoControlCode;
	} DeviceIoControl;
	struct {
	    DEVICE_RELATION_TYPE Type;
	} QueryDeviceRelations;
	struct {
	    BUS_QUERY_ID_TYPE IdType;
	} QueryId;
    } Parameters;
} IO_REQUEST_PARAMETERS, *PIO_REQUEST_PARAMETERS;

/*
 * Message type of server messages
 */
typedef enum _IO_SERVER_MESSAGE_TYPE {
    IoSrvMsgIoCompleted,
    IoSrvMsgAddDevice
} IO_SERVER_MESSAGE_TYPE;

/*
 * Message packet from the server.
 */
typedef struct _IO_PACKET_SERVER_MESSAGE {
    IO_SERVER_MESSAGE_TYPE Type;
    union {
	struct {
	    GLOBAL_HANDLE OriginatingThread; /* OriginalIrp is guaranteed to be unique for every
					      * OriginatingThread. Drivers must use this member to
					      * disambiguate the IRP that is being completed. */
	    HANDLE OriginalIrp;	/* Unique up to each OriginatingThread */
	    IO_STATUS_BLOCK IoStatus; /* Final status of the IO operation */
	    ULONG ResponseDataSize;
	    ULONG_PTR ResponseData[];
	} IoCompleted; /* Used for notifying a client driver that an IRP that it has requested
			* or forwarded is now completed. */
	struct {
	    GLOBAL_HANDLE PhysicalDeviceObject;
	    IO_DEVICE_INFO PhysicalDeviceInfo;
	} AddDevice; /* Used by the PNP manager to inform the driver to add a new device */
    } Parameters;
} IO_PACKET_SERVER_MESSAGE, *PIO_PACKET_SERVER_MESSAGE;

/*
 * Message type of client messages
 */
typedef enum _IO_CLIENT_MESSAGE_TYPE {
    IoCliMsgIoCompleted,
    IoCliMsgForwardIrp
} IO_CLIENT_MESSAGE_TYPE;

/*
 * Message packet from the client.
 */
typedef struct _IO_PACKET_CLIENT_MESSAGE {
    IO_CLIENT_MESSAGE_TYPE Type;
    union {
	struct {
	    GLOBAL_HANDLE OriginatingThread; /* Driver must set this to the OriginatingThread
					      * member of the original IRP. This is used to
					      * disambiguate the IRP being completed */
	    HANDLE OriginalIrp;	/* The OriginalIrp handle of the IRP being completed. */
	    IO_STATUS_BLOCK IoStatus;
	    ULONG ResponseDataSize;
	    ULONG_PTR ResponseData[];
	} IoCompleted;
	struct {
	    GLOBAL_HANDLE OriginatingThread; /* Driver must set this to the Thread.Handle
					      * member of the original IRP. This is used to
					      * disambiguate the IRP being forwarded. */
	    HANDLE OriginalIrp;
	    GLOBAL_HANDLE DeviceObject;
	    BOOLEAN NotifyCompletion; /* TRUE if the client driver wants the server to 
				       * notify it after the IRP has been processed. */
	} ForwardIrp;
    } Parameters;
} IO_PACKET_CLIENT_MESSAGE, *PIO_PACKET_CLIENT_MESSAGE;

/*
 * This is the actual data structure being passed between the server
 * task and the client driver processes. The public IRP struct is exposed
 * in wdm.h in order to remain (semi-)compatible with Windows/ReactOS.
 */
typedef struct _IO_PACKET {
    IO_PACKET_TYPE Type;
    ULONG Size;	/* Size of the IO packet in bytes. This includes all the trailing data. */
    LIST_ENTRY IoPacketLink; /* List entry for either IoPacketQueue or PendingIoPacketList
			      * of the driver object. This is only valid when the IoPacket
			      * object is being queued on the driver object or is in the
			      * driver's pending IoPacket list. */
    union {
	IO_REQUEST_PARAMETERS Request; /* Type == IoPacketTypeRequest */
	IO_PACKET_SERVER_MESSAGE ServerMsg; /* For Type == IoPacketTypeServerMessage */
	IO_PACKET_CLIENT_MESSAGE ClientMsg; /* For Type == IoPacketTypeClientMessage */
    }; /* This must be at the end of the struct since it might have variable length */
} IO_PACKET, *PIO_PACKET;

typedef PTHREAD_START_ROUTINE PIO_WORKER_THREAD_ENTRY;
typedef PTHREAD_START_ROUTINE PIO_INTERRUPT_SERVICE_THREAD_ENTRY;

#include <poppack.h>

/*
 * Inline functions
 */
static inline VOID IoDbgDumpFileObjectCreateParameters(IN PIO_PACKET IoPacket,
						       IN PFILE_OBJECT_CREATE_PARAMETERS Params)
{
    DbgPrint("ReadAccess %s WriteAccess %s DeleteAccess %s SharedRead %s "
	     "SharedWrite %s SharedDelete %s Flags 0x%08x FileName %p\n",
	     Params->ReadAccess ? "TRUE" : "FALSE",
	     Params->WriteAccess ? "TRUE" : "FALSE",
	     Params->DeleteAccess ? "TRUE" : "FALSE",
	     Params->SharedRead ? "TRUE" : "FALSE",
	     Params->SharedWrite ? "TRUE" : "FALSE",
	     Params->SharedDelete ? "TRUE" : "FALSE",
	     Params->Flags, (PUCHAR)IoPacket + Params->FileNameOffset);
}

static inline PCSTR IopDbgDeviceRelationTypeStr(IN DEVICE_RELATION_TYPE Type)
{
    switch (Type) {
    case BusRelations:
	return "BusRelations";
    case EjectionRelations:
	return "EjectionRelations";
    case PowerRelations:
	return "PowerRelations";
    case RemovalRelations:
	return "RemovalRelations";
    case TargetDeviceRelation:
	return "TargetDeviceRelation";
    case SingleBusRelations:
	return "SingleBusRelations";
    case TransportRelations:
	return "TransportRelations";
    }
    return "UNKNOWN";
}

static inline VOID IoDbgDumpIoPacket(IN PIO_PACKET IoPacket,
				     IN BOOLEAN ClientSide)
{
    DbgTrace("Dumping IO Packet %p size %d\n", IoPacket, IoPacket->Size);
    DbgPrint("    TYPE: ");
    switch (IoPacket->Type) {
    case IoPacketTypeRequest:
	DbgPrint("IO REQUEST\n");
	break;
    case IoPacketTypeServerMessage:
	DbgPrint("SERVER MESSAGE\n");
	break;
    case IoPacketTypeClientMessage:
	DbgPrint("CLIENT MESSAGE\n");
	break;
    default:
	DbgPrint("INVALID!!\n");
	return;
    }
    if (IoPacket->Type == IoPacketTypeRequest) {
	DbgPrint("    Major function %d.  Minor function %d.  Flags 0x%x.  Control 0x%x\n",
		 IoPacket->Request.MajorFunction, IoPacket->Request.MinorFunction,
		 IoPacket->Request.Flags, IoPacket->Request.Control);
	switch (IoPacket->Request.MajorFunction) {
	case IRP_MJ_CREATE:
	    DbgPrint("    CREATE  Options 0x%x FileAttr 0x%x ShareAccess 0x%x\n",
		     IoPacket->Request.Parameters.Create.Options,
		     IoPacket->Request.Parameters.Create.FileAttributes,
		     IoPacket->Request.Parameters.Create.ShareAccess);
	    DbgPrint("        FileObjectCreateParameters ");
	    IoDbgDumpFileObjectCreateParameters(IoPacket,
						&IoPacket->Request.Parameters.Create.FileObjectParameters);
	    break;
	case IRP_MJ_DEVICE_CONTROL:
	    DbgPrint("    DEVICE-CONTROL  IoControlCode %d InputBuffer %p Length 0x%x OutputBuffer %p Length 0x%x\n",
		     IoPacket->Request.Parameters.DeviceIoControl.IoControlCode,
		     IoPacket->Request.Parameters.DeviceIoControl.InputBuffer,
		     IoPacket->Request.Parameters.DeviceIoControl.InputBufferLength,
		     IoPacket->Request.Parameters.DeviceIoControl.OutputBuffer,
		     IoPacket->Request.Parameters.DeviceIoControl.OutputBufferLength);
	    break;
	case IRP_MJ_PNP:
	    switch (IoPacket->Request.MinorFunction) {
	    case IRP_MN_QUERY_DEVICE_RELATIONS:
		DbgPrint("    PNP  QUERY-DEVICE-RELATIONS  Type %s\n",
			 IopDbgDeviceRelationTypeStr(IoPacket->Request.Parameters.QueryDeviceRelations.Type));
		break;
	    default:
		DbgPrint("    PNP  UNKNOWN-MINOR-FUNCTION\n");
	    }
	    break;
	default:
	    DbgPrint("    UNKNOWN-MAJOR-FUNCTION\n");
	}
	if (ClientSide) {
	    DbgPrint("    DeviceHandle %p FileHandle %p ThreadHandle %p Identifier %p\n",
		     (PVOID)IoPacket->Request.Device.Handle,
		     (PVOID)IoPacket->Request.File.Handle,
		     (PVOID)IoPacket->Request.OriginatingThread.Handle,
		     IoPacket->Request.Identifier);
	} else {
	    DbgPrint("    DeviceObject %p FileObject %p ThreadObject %p Identifier %p\n",
		     IoPacket->Request.Device.Object,
		     IoPacket->Request.File.Object,
		     IoPacket->Request.OriginatingThread.Object,
		     IoPacket->Request.Identifier);
	}
    } else if (IoPacket->Type == IoPacketTypeServerMessage) {
	switch (IoPacket->ServerMsg.Type) {
	case IoSrvMsgIoCompleted:
	    DbgPrint("    SERVER-MSG IO-COMPLETED ThreadHandle %p Identifier %p\n",
		     (PVOID)IoPacket->ServerMsg.Parameters.IoCompleted.OriginatingThread,
		     IoPacket->ServerMsg.Parameters.IoCompleted.OriginalIrp);
	    DbgPrint("    Final IO status 0x%08x Information %p\n",
		     IoPacket->ServerMsg.Parameters.IoCompleted.IoStatus.Status,
		     (PVOID)IoPacket->ServerMsg.Parameters.IoCompleted.IoStatus.Information);
	    break;
	case IoSrvMsgAddDevice:
	    DbgPrint("    SERVER-MSG ADD-DEVICE PDO %p\n",
		     (PVOID)IoPacket->ServerMsg.Parameters.AddDevice.PhysicalDeviceObject);
	    break;
	}
    } else if (IoPacket->Type == IoPacketTypeClientMessage) {
	switch (IoPacket->ClientMsg.Type) {
	case IoCliMsgIoCompleted:
	    DbgPrint("    CLIENT-MSG IO-COMPLETED ThreadHandle %p Identifier %p\n",
		     (PVOID)IoPacket->ClientMsg.Parameters.IoCompleted.OriginatingThread,
		     IoPacket->ClientMsg.Parameters.IoCompleted.OriginalIrp);
	    DbgPrint("    Final IO status 0x%08x Information %p\n",
		     IoPacket->ClientMsg.Parameters.IoCompleted.IoStatus.Status,
		     (PVOID)IoPacket->ClientMsg.Parameters.IoCompleted.IoStatus.Information);
	    break;
	case IoCliMsgForwardIrp:
	    DbgPrint("    CLIENT-MSG FORWARD-IRP ThreadHandle %p Identifier %p\n",
		     (PVOID)IoPacket->ClientMsg.Parameters.IoCompleted.OriginatingThread,
		     IoPacket->ClientMsg.Parameters.IoCompleted.OriginalIrp);
	    DbgPrint("    DeviceObject %p NotifyCompletion %s\n",
		     (PVOID)IoPacket->ClientMsg.Parameters.ForwardIrp.DeviceObject,
		     IoPacket->ClientMsg.Parameters.ForwardIrp.NotifyCompletion ? "TRUE" : "FALSE");
	    break;
	default:
	    DbgPrint("    INVALID CLIENT MESSAGE TYPE %d\n", IoPacket->ClientMsg.Type);
	}
    }
}
