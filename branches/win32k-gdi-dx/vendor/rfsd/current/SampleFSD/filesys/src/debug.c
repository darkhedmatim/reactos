#include "ntifs.h"
#include "sfsd.h"

static PUCHAR IrpMjStrings[] = {
    "CREATE",
    "CREATE_NAMED_PIPE",
    "CLOSE",
    "READ",
    "WRITE",
    "QUERY_INFORMATION",
    "SET_INFORMATION",
    "QUERY_EA",
    "SET_EA",
    "FLUSH_BUFFERS",
    "QUERY_VOLUME_INFORMATION",
    "SET_VOLUME_INFORMATION",
    "DIRECTORY_CONTROL",
    "FILE_SYSTEM_CONTROL",
    "DEVICE_CONTROL",
    "INTERNAL_DEVICE_CONTROL",
    "SHUTDOWN",
    "LOCK_CONTROL",
    "CLEANUP",
    "CREATE_MAILSLOT",
    "QUERY_SECURITY",
    "SET_SECURITY",
    "POWER",
    "SYSTEM_CONTROL",
    "DEVICE_CHANGE",
    "QUERY_QUOTA",
    "SET_QUOTA",
    "PNP"
};


VOID
FsdDbgPrintCall (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    PIO_STACK_LOCATION      IrpSp = IoGetCurrentIrpStackLocation(Irp);
	 PFILE_OBJECT            FileObject = IrpSp->FileObject;
	 PUCHAR                  FileName = "Unknown";

    //- [ file name prefix ] 

	 DbgPrint(    DRIVER_NAME ": %s\n",                
                IrpMjStrings[IrpSp->MajorFunction]
            );
}
