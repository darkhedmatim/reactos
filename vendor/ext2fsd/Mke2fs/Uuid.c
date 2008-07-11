/*
 * PROJECT:          Mke2fs
 * FILE:             Timer.c
 * PROGRAMMER:       Matt Wu <mattwu@163.com>
 * HOMEPAGE:         http://ext2.yeah.net
 */

/* INCLUDES **************************************************************/

#include "stdafx.h"
#include "windows.h"
#include "Ext2fs\types.h"
#include <rpc.h>

/* DEFINITIONS ***********************************************************/


/* FUNCTIONS *************************************************************/

/*
RPC_STATUS RPC_ENTRY UuidCreate( 
  UUID *  Uuid  
);
*/

void uuid_generate(__u8 * uuid)
{
   UuidCreate((UUID *) uuid);
}