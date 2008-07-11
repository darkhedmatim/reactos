/*
 * PROJECT:          Mke2fs
 * FILE:             Timer.c
 * PROGRAMMER:       Matt Wu <mattwu@163.com>
 * HOMEPAGE:         http://ext2.yeah.net
 */

/* INCLUDES **************************************************************/

#include "Mke2fs.h"

/* DEFINITIONS ***********************************************************/

/* FUNCTIONS *************************************************************/

LONGLONG ext2_nt_time (ULONG i_time)
{
    LARGE_INTEGER   NtTime;

    RtlSecondsSince1970ToTime(i_time, &NtTime);

    return NtTime.QuadPart;
}

ULONG ext2_unix_time (LONGLONG n_time)
{
    LARGE_INTEGER   NtTime;
    ULONG           unix_time;

    NtTime.QuadPart = n_time;

    RtlTimeToSecondsSince1970( &NtTime, &unix_time);

    return unix_time;
}
