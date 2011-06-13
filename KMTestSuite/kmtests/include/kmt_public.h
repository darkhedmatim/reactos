/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite public declarations
 * PROGRAMMER:      Thomas Faber <thfabba@gmx.de>
 */

#ifndef _KMTEST_PUBLIC_H_
#define _KMTEST_PUBLIC_H_

#define IOCTL_KMTEST_GET_TESTS \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_READ_DATA)

#define IOCTL_KMTEST_RUN_TEST  \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)

#define IOCTL_KMTEST_SET_RESULTBUFFER  \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_NEITHER, FILE_READ_DATA | FILE_WRITE_DATA)

#define KMTEST_DEVICE_NAME L"Kmtest"
#define KMTEST_DEVICE_PATH (L"\\\\.\\Global\\GLOBALROOT\\Device\\" KMTEST_DEVICE_NAME)

#endif /* !defined _KMTEST_PUBLIC_H_ */
