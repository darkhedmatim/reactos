/*************************************************************************
*
* File: cleanup.c
*
* Module: Sample File System Driver (Kernel mode execution only)
*
* Description:
*	Should contain code to handle the "Cleanup" dispatch entry point.
*	This file serves as a placeholder. Please update this file as part
*	of designing and implementing your FSD.
*
* Author: R. Nagar
*
* (c) 1996-97 Rajeev Nagar, All Rights Reserved
*
*************************************************************************/

#include			"sfsd.h"
#include			"ntifs.h"

// define the file specific bug-check id
#define			SFSD_BUG_CHECK_ID				SFSD_FILE_FSCONTROL



/*************************************************************************
* TODOC:
* Function: SFsdCleanup()
*
* Description:
*	The I/O Manager will invoke this routine to handle a cleanup
*	request
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL (invocation at higher IRQL will cause execution
*	to be deferred to a worker thread context)
*
* Return Value: Does not matter!
*
*************************************************************************/
NTSTATUS SFsdFSControl(
PDEVICE_OBJECT		DeviceObject,		// the logical volume device object
PIRP					Irp)					// I/O Request Packet
{
	NTSTATUS				RC = STATUS_SUCCESS;
   PtrSFsdIrpContext	PtrIrpContext = NULL;
	BOOLEAN				AreWeTopLevel = FALSE;

	DbgPrint("MADE IT TO FS CONTROL!\n");
	FsdDbgPrintCall(DeviceObject, Irp);

	FsRtlEnterFileSystem();
	ASSERT(DeviceObject);
	ASSERT(Irp);
	
	FsRtlExitFileSystem();

	return(RC);
}
