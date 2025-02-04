#include "psp.h"

static NTSTATUS PspSuspendThread(IN MWORD Cap)
{
    int Error = seL4_TCB_Suspend(Cap);

    if (Error != 0) {
	DbgTrace("seL4_TCB_Suspend failed for thread cap 0x%zx with error %d\n",
		 Cap, Error);
	return SEL4_ERROR(Error);
    }

    return STATUS_SUCCESS;
}

NTSTATUS PsTerminateThread(IN PTHREAD Thread,
			   IN NTSTATUS ExitStatus)
{
    assert(Thread->Process != NULL);
    Thread->ExitStatus = ExitStatus;
    /* If the thread to terminate is the main event loop of a driver thread,
     * set the InitializationDone event to wake up the thread waiting on NtLoadDriver */
    if (Thread->Process->DriverObject != NULL) {
	PIO_DRIVER_OBJECT DriverObject = Thread->Process->DriverObject;
	KeSetEvent(&DriverObject->InitializationDoneEvent);
    }
    /* For now we simply suspend the thread */
    return PspSuspendThread(Thread->TreeNode.Cap);
}

NTSTATUS PsTerminateSystemThread(IN PSYSTEM_THREAD Thread)
{
    /* For now we simply suspend the thread */
    return PspSuspendThread(Thread->TreeNode.Cap);
}

NTSTATUS NtTerminateThread(IN ASYNC_STATE State,
			   IN PTHREAD Thread,
                           IN HANDLE ThreadHandle,
                           IN NTSTATUS ExitStatus)
{
    PTHREAD ThreadToTerminate = NULL;
    if (ThreadHandle == NtCurrentThread()) {
	ThreadToTerminate = Thread;
    } else {
	RET_ERR(ObReferenceObjectByHandle(Thread->Process, ThreadHandle,
					  OBJECT_TYPE_THREAD, (POBJECT *) &ThreadToTerminate));
    }
    assert(ThreadToTerminate != NULL);
    PsTerminateThread(ThreadToTerminate, ExitStatus);
    /* If the current thread is terminating, do not reply to it */
    if (ThreadHandle == NtCurrentThread()) {
	return STATUS_NTOS_NO_REPLY;
    }
    return STATUS_SUCCESS;
}

NTSTATUS NtTerminateProcess(IN ASYNC_STATE State,
			    IN PTHREAD Thread,
                            IN HANDLE ProcessHandle,
                            IN NTSTATUS ExitStatus)
{
    UNIMPLEMENTED;
}

NTSTATUS NtResumeThread(IN ASYNC_STATE AsyncState,
                        IN PTHREAD Thread,
                        IN HANDLE ThreadHandle,
                        OUT OPTIONAL ULONG *SuspendCount)
{
    PTHREAD ThreadToResume = NULL;
    if (ThreadHandle == NtCurrentThread()) {
	return STATUS_INVALID_PARAMETER_1;
    } else {
	RET_ERR(ObReferenceObjectByHandle(Thread->Process, ThreadHandle,
					  OBJECT_TYPE_THREAD, (POBJECT *) &ThreadToResume));
    }
    assert(ThreadToResume != NULL);
    NTSTATUS Status = STATUS_INTERNAL_ERROR;
    IF_ERR_GOTO(out, Status, PsResumeThread(ThreadToResume));
    Status = STATUS_SUCCESS;
out:
    if (ThreadToResume != NULL) {
	ObDereferenceObject(ThreadToResume);
    }
    return Status;
}

NTSTATUS NtDelayExecution(IN ASYNC_STATE AsyncState,
                          IN PTHREAD Thread,
                          IN BOOLEAN Alertable,
                          IN PLARGE_INTEGER Interval)
{
    UNIMPLEMENTED;
}
