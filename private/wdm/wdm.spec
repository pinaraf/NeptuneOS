@ stdcall IoCreateDevice(ptr long ptr long long long ptr)
@ stdcall IoRegisterDriverReinitialization(ptr ptr ptr)
@ stdcall IoAllocateDriverObjectExtension(ptr ptr long ptr)
@ stdcall IoGetDriverObjectExtension(ptr ptr)
@ stdcall IoGetDeviceObjectPointer(ptr long ptr ptr)
@ stdcall IoAttachDeviceToDeviceStack(ptr ptr)
@ stdcall IoGetAttachedDevice(ptr)
@ stdcall IoRegisterDeviceInterface(ptr ptr ptr ptr)
@ stdcall IoSetDeviceInterfaceState(ptr long)
@ stdcall IoDetachDevice(ptr)
@ stdcall IoDeleteDevice(ptr)
@ stdcall IoStartPacket(ptr ptr ptr ptr)
@ stdcall IoStartNextPacket(ptr long)
@ stdcall IoBuildDeviceIoControlRequest(long ptr ptr long ptr long long ptr)
@ stdcall IoBuildAsynchronousFsdRequest(long ptr ptr long ptr)
@ stdcall IoBuildSynchronousFsdRequest(long ptr ptr long ptr ptr)
@ stdcall IoCallDriverEx(ptr ptr ptr)
@ stdcall IoConnectInterrupt(ptr ptr ptr long long long long long long long)
@ stdcall IoDisconnectInterrupt(ptr)
@ stdcall MmPageEntireDriver(ptr)
@ stdcall KeInsertDeviceQueue(ptr ptr)
@ stdcall KeInsertByKeyDeviceQueue(ptr ptr long)
@ stdcall KeRemoveDeviceQueue(ptr)
@ stdcall KeRemoveByKeyDeviceQueue(ptr long)
@ stdcall KeInitializeTimer(ptr)
@ stdcall KeSetTimer(ptr long long ptr ptr)
@ stdcall KeQueryInterruptTime()
@ stdcall KeStallExecutionProcessor(long)
@ stdcall HalMakeBeep(long)
@ cdecl __inbyte(long)
@ cdecl __outbyte(long long)
@ cdecl _assert(str str long)
