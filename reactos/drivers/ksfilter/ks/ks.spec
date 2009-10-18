@ stdcall KsCreateAllocator(ptr ptr ptr)
@ stdcall KsCreateDefaultAllocator(ptr)
@ stdcall KsValidateAllocatorCreateRequest(ptr ptr)
@ stdcall KsCreateDefaultAllocatorEx(ptr ptr ptr ptr ptr ptr)
@ stdcall KsValidateAllocatorFramingEx(ptr long ptr)

@ stdcall KsCreateClock(ptr ptr ptr)
@ stdcall KsCreateDefaultClock(ptr ptr)
@ stdcall KsAllocateDefaultClock(ptr)
@ stdcall KsAllocateDefaultClockEx(ptr ptr ptr ptr ptr ptr long)
@ stdcall KsFreeDefaultClock(ptr)
@ stdcall KsValidateClockCreateRequest(ptr ptr)
@ stdcall KsGetDefaultClockState(ptr)
@ stdcall KsSetDefaultClockState(ptr long)
@ stdcall KsGetDefaultClockTime(ptr)
@ stdcall KsSetDefaultClockTime(ptr long long)

@ stdcall KsMethodHandler(ptr long ptr)
@ stdcall KsMethodHandlerWithAllocator(ptr long ptr ptr long)
@ stdcall KsFastMethodHandler(ptr ptr long ptr long ptr long ptr)

@ stdcall KsPropertyHandler(ptr long ptr)
@ stdcall KsPropertyHandlerWithAllocator(ptr long ptr ptr long)
@ stdcall KsUnserializeObjectPropertiesFromRegistry(ptr ptr ptr)
@ stdcall KsFastPropertyHandler(ptr ptr long ptr long ptr long ptr)

@ stdcall KsGenerateEvent(ptr)
@ stdcall KsEnableEventWithAllocator(ptr long ptr ptr long ptr ptr long)
@ stdcall KsGenerateDataEvent(ptr long ptr)
@ stdcall KsEnableEvent(ptr long ptr ptr long ptr)
@ stdcall KsDiscardEvent(ptr)
@ stdcall KsDisableEvent(ptr ptr long ptr)
@ stdcall KsFreeEventList(ptr ptr long ptr)

@ stdcall KsValidateTopologyNodeCreateRequest(ptr ptr ptr)
@ stdcall KsCreateTopologyNode(ptr ptr long ptr)
@ stdcall KsTopologyPropertyHandler(ptr ptr ptr ptr)

@ stdcall KsCreatePin(ptr ptr long ptr)
@ stdcall KsValidateConnectRequest(ptr long ptr ptr)
@ stdcall KsPinPropertyHandler(ptr ptr ptr long ptr)
@ stdcall KsPinDataIntersection(ptr ptr ptr long ptr ptr)

@ stdcall KsHandleSizedListQuery(ptr long long ptr)

@ stdcall KsAcquireResetValue(ptr ptr)
@ stdcall KsDefaultForwardIrp(ptr ptr)
@ stdcall KsAddIrpToCancelableQueue(ptr ptr ptr long ptr)
@ stdcall KsAddObjectCreateItemToDeviceHeader(ptr ptr ptr wstr ptr)
@ stdcall KsAddObjectCreateItemToObjectHeader(ptr ptr ptr wstr ptr)
@ stdcall KsAllocateDeviceHeader(ptr long ptr)
@ stdcall KsAllocateExtraData(ptr long ptr)
@ stdcall KsAllocateObjectCreateItem(long ptr long ptr)
@ stdcall KsAllocateObjectHeader(ptr long ptr ptr ptr)
@ stdcall KsCancelIo(ptr ptr)
@ stdcall KsCancelRoutine(ptr ptr)
@ stdcall KsDefaultDeviceIoCompletion(ptr ptr)
@ stdcall KsDispatchFastIoDeviceControlFailure(ptr long ptr long ptr long long ptr ptr)
@ stdcall KsDispatchFastReadFailure(ptr ptr long long long ptr ptr ptr)
; KsDispatchFastWriteFailure@32
@ stdcall KsDispatchInvalidDeviceRequest(ptr ptr)
@ stdcall KsDispatchIrp(ptr ptr)
@ stdcall KsDispatchSpecificMethod(ptr ptr)
@ stdcall KsDispatchSpecificProperty(ptr ptr)
@ stdcall KsForwardAndCatchIrp(ptr ptr ptr long)
@ stdcall KsForwardIrp(ptr ptr long)
@ stdcall KsFreeDeviceHeader(ptr)
@ stdcall KsFreeObjectHeader(ptr)
@ stdcall KsGetChildCreateParameter(ptr ptr)
@ stdcall KsMoveIrpsOnCancelableQueue(ptr ptr ptr ptr long ptr ptr)
@ stdcall KsProbeStreamIrp(ptr long long)
@ stdcall KsQueryInformationFile(ptr ptr long long)
@ stdcall KsQueryObjectAccessMask(ptr)
@ stdcall KsQueryObjectCreateItem(ptr)
@ stdcall KsReadFile(ptr ptr ptr ptr ptr long long long)
@ stdcall KsReleaseIrpOnCancelableQueue(ptr ptr)
@ stdcall KsRemoveIrpFromCancelableQueue(ptr ptr long long)
@ stdcall KsRemoveSpecificIrpFromCancelableQueue(ptr)
@ stdcall KsSetInformationFile(ptr ptr long long)
@ stdcall KsSetMajorFunctionHandler(ptr long)
@ stdcall KsStreamIo(ptr ptr ptr ptr ptr long ptr ptr long long long)
@ stdcall KsWriteFile(ptr ptr ptr ptr ptr long long long)

@ stdcall KsRegisterWorker(long ptr)
@ stdcall KsUnregisterWorker(ptr)
@ stdcall KsRegisterCountedWorker(long ptr ptr)
@ stdcall KsDecrementCountedWorker(ptr)
@ stdcall KsIncrementCountedWorker(ptr)
@ stdcall KsQueueWorkItem(ptr ptr)

@ stdcall KsCacheMedium(ptr ptr long)
@ stdcall KsDefaultDispatchPnp(ptr ptr)
@ stdcall KsSetDevicePnpAndBaseObject(ptr ptr ptr)
@ stdcall KsDefaultDispatchPower(ptr ptr)
@ stdcall KsSetPowerDispatch(ptr ptr ptr)
@ stdcall KsReferenceBusObject(ptr)
@ stdcall KsDereferenceBusObject(ptr)
@ stdcall KsFreeObjectCreateItem(ptr ptr)
@ stdcall KsFreeObjectCreateItemsByContext(ptr ptr)
@ stdcall KsLoadResource(ptr long ptr long ptr ptr)
@ stdcall KsNullDriverUnload(ptr)
@ stdcall KsPinDataIntersectionEx(ptr ptr ptr long ptr long ptr ptr)
@ stdcall KsQueryDevicePnpObject(ptr)
@ stdcall KsRecalculateStackDepth(ptr long)
@ stdcall KsSetTargetDeviceObject(ptr ptr)
@ stdcall KsSetTargetState(ptr long)
@ stdcall KsSynchronousIoControlDevice(ptr long long ptr long ptr long ptr)
@ stdcall KsInitializeDriver(ptr ptr ptr)

; Kernel COM
@ stdcall KoCreateInstance(ptr ptr long ptr ptr)
