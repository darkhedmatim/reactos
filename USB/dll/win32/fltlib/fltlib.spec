@ stdcall FilterAttach(wstr wstr wstr long wstr)
@ stdcall FilterAttachAtAltitude(wstr wstr wstr wstr long wstr)
@ stdcall FilterClose(ptr)
@ stdcall FilterConnectCommunicationPort(wstr long ptr long ptr ptr)
@ stdcall FilterCreate(wstr ptr)
@ stdcall FilterDetach(wstr wstr wstr)
@ stdcall FilterFindClose(ptr)
@ stdcall FilterFindFirst(long ptr long ptr ptr)
@ stdcall FilterFindNext(ptr ptr long ptr ptr)
@ stdcall FilterGetDosName(wstr wstr long)
@ stdcall FilterGetInformation(ptr long ptr long ptr)
@ stdcall FilterGetMessage(ptr ptr long ptr)
@ stdcall FilterInstanceClose(ptr)
@ stdcall FilterInstanceCreate(wstr wstr wstr ptr)
@ stdcall FilterInstanceFindClose(ptr)
@ stdcall FilterInstanceFindFirst(wstr long ptr long ptr ptr)
@ stdcall FilterInstanceFindNext(ptr long ptr long ptr)
@ stdcall FilterInstanceGetInformation(ptr long ptr long ptr)
@ stdcall FilterLoad(wstr)
@ stdcall FilterReplyMessage(ptr ptr long)
@ stdcall FilterSendMessage(ptr ptr long ptr long ptr)
@ stdcall FilterUnload(wstr)
@ stdcall FilterVolumeFindClose(ptr)
@ stdcall FilterVolumeFindFirst(long ptr long ptr ptr)
@ stdcall FilterVolumeFindNext(ptr long ptr long ptr)
@ stdcall FilterVolumeInstanceFindClose(ptr)
@ stdcall FilterVolumeInstanceFindFirst(wstr long ptr long ptr ptr)
@ stdcall FilterVolumeInstanceFindNext(ptr long ptr long ptr)
