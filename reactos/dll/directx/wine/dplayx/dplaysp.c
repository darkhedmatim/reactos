/* This contains the implementation of the interface Service
 * Providers require to communicate with Direct Play
 *
 * Copyright 2000 Peter Hunnisett
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "dplayx_global.h"

/* FIXME: Need to add interface locking inside procedures */

/* Prototypes */
static BOOL DPSP_CreateIUnknown( LPVOID lpSP );
static BOOL DPSP_DestroyIUnknown( LPVOID lpSP );
static BOOL DPSP_CreateDirectPlaySP( void *lpSP, IDirectPlayImpl *dp );
static BOOL DPSP_DestroyDirectPlaySP( LPVOID lpSP );

/* Predefine the interface */
typedef struct IDirectPlaySPImpl IDirectPlaySPImpl;

typedef struct tagDirectPlaySPIUnknownData
{
  LONG              ulObjRef;
  CRITICAL_SECTION  DPSP_lock;
} DirectPlaySPIUnknownData;

typedef struct tagDirectPlaySPData
{
  LPVOID lpSpRemoteData;
  DWORD  dwSpRemoteDataSize; /* Size of data pointed to by lpSpRemoteData */

  LPVOID lpSpLocalData;
  DWORD  dwSpLocalDataSize; /* Size of data pointed to by lpSpLocalData */

  IDirectPlayImpl *dplay; /* FIXME: This should perhaps be iface not impl */

} DirectPlaySPData;

#define DPSP_IMPL_FIELDS \
   LONG ulInterfaceRef; \
   DirectPlaySPIUnknownData* unk; \
   DirectPlaySPData* sp;

struct IDirectPlaySPImpl
{
  const IDirectPlaySPVtbl *lpVtbl;
  DPSP_IMPL_FIELDS
};

/* Forward declaration of virtual tables */
static const IDirectPlaySPVtbl directPlaySPVT;

/* This structure is passed to the DP object for safe keeping */
typedef struct tagDP_SPPLAYERDATA
{
  LPVOID lpPlayerLocalData;
  DWORD  dwPlayerLocalDataSize;

  LPVOID lpPlayerRemoteData;
  DWORD  dwPlayerRemoteDataSize;
} DP_SPPLAYERDATA, *LPDP_SPPLAYERDATA;

/* Create the SP interface */
HRESULT DPSP_CreateInterface( REFIID riid, void **ppvObj, IDirectPlayImpl *dp )
{
  TRACE( " for %s\n", debugstr_guid( riid ) );

  *ppvObj = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                       sizeof( IDirectPlaySPImpl ) );

  if( *ppvObj == NULL )
  {
    return DPERR_OUTOFMEMORY;
  }

  if( IsEqualGUID( &IID_IDirectPlaySP, riid ) )
  {
    IDirectPlaySPImpl *This = *ppvObj;
    This->lpVtbl = &directPlaySPVT;
  }
  else
  {
    /* Unsupported interface */
    HeapFree( GetProcessHeap(), 0, *ppvObj );
    *ppvObj = NULL;

    return E_NOINTERFACE;
  }

  /* Initialize it */
  if( DPSP_CreateIUnknown( *ppvObj ) &&
      DPSP_CreateDirectPlaySP( *ppvObj, dp )
    )
  {
    IDirectPlaySP_AddRef( (LPDIRECTPLAYSP)*ppvObj );
    return S_OK;
  }

  /* Initialize failed, destroy it */
  DPSP_DestroyDirectPlaySP( *ppvObj );
  DPSP_DestroyIUnknown( *ppvObj );

  HeapFree( GetProcessHeap(), 0, *ppvObj );
  *ppvObj = NULL;

  return DPERR_NOMEMORY;
}

static BOOL DPSP_CreateIUnknown( LPVOID lpSP )
{
  IDirectPlaySPImpl *This = lpSP;

  This->unk = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof( *(This->unk) ) );

  if ( This->unk == NULL )
  {
    return FALSE;
  }

  InitializeCriticalSection( &This->unk->DPSP_lock );
  This->unk->DPSP_lock.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": IDirectPlaySPImpl*->DirectPlaySPIUnknownData*->DPSP_lock");

  return TRUE;
}

static BOOL DPSP_DestroyIUnknown( LPVOID lpSP )
{
  IDirectPlaySPImpl *This = lpSP;

  This->unk->DPSP_lock.DebugInfo->Spare[0] = 0;
  DeleteCriticalSection( &This->unk->DPSP_lock );
  HeapFree( GetProcessHeap(), 0, This->unk );

  return TRUE;
}


static BOOL DPSP_CreateDirectPlaySP( void *lpSP, IDirectPlayImpl *dp )
{
  IDirectPlaySPImpl *This = lpSP;

  This->sp = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof( *(This->sp) ) );

  if ( This->sp == NULL )
  {
    return FALSE;
  }

  This->sp->dplay = dp;

  return TRUE;
}

static BOOL DPSP_DestroyDirectPlaySP( LPVOID lpSP )
{
  IDirectPlaySPImpl *This = lpSP;

  HeapFree( GetProcessHeap(), 0, This->sp->lpSpRemoteData );
  HeapFree( GetProcessHeap(), 0, This->sp->lpSpLocalData );

  /* FIXME: Need to delete player queue */

  HeapFree( GetProcessHeap(), 0, This->sp );
  return TRUE;
}

/* Interface implementation */

static HRESULT WINAPI DPSP_QueryInterface
( LPDIRECTPLAYSP iface,
  REFIID riid,
  LPVOID* ppvObj )
{
  IDirectPlaySPImpl *This = (IDirectPlaySPImpl *)iface;
  TRACE("(%p)->(%s,%p)\n", This, debugstr_guid( riid ), ppvObj );

  *ppvObj = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                       sizeof( *This ) );

  if( *ppvObj == NULL )
  {
    return DPERR_OUTOFMEMORY;
  }

  CopyMemory( *ppvObj, This, sizeof( *This )  );
  (*(IDirectPlaySPImpl**)ppvObj)->ulInterfaceRef = 0;

  if( IsEqualGUID( &IID_IDirectPlaySP, riid ) )
  {
    IDirectPlaySPImpl *This = *ppvObj;
    This->lpVtbl = &directPlaySPVT;
  }
  else
  {
    /* Unsupported interface */
    HeapFree( GetProcessHeap(), 0, *ppvObj );
    *ppvObj = NULL;

    return E_NOINTERFACE;
  }

  IDirectPlaySP_AddRef( (LPDIRECTPLAYSP)*ppvObj );

  return S_OK;
}

static ULONG WINAPI DPSP_AddRef
( LPDIRECTPLAYSP iface )
{
  ULONG ulInterfaceRefCount, ulObjRefCount;
  IDirectPlaySPImpl *This = (IDirectPlaySPImpl *)iface;

  ulObjRefCount       = InterlockedIncrement( &This->unk->ulObjRef );
  ulInterfaceRefCount = InterlockedIncrement( &This->ulInterfaceRef );

  TRACE( "ref count incremented to %u:%u for %p\n",
         ulInterfaceRefCount, ulObjRefCount, This );

  return ulObjRefCount;
}

static ULONG WINAPI DPSP_Release
( LPDIRECTPLAYSP iface )
{
  ULONG ulInterfaceRefCount, ulObjRefCount;
  IDirectPlaySPImpl *This = (IDirectPlaySPImpl *)iface;

  ulObjRefCount       = InterlockedDecrement( &This->unk->ulObjRef );
  ulInterfaceRefCount = InterlockedDecrement( &This->ulInterfaceRef );

  TRACE( "ref count decremented to %u:%u for %p\n",
         ulInterfaceRefCount, ulObjRefCount, This );

  /* Deallocate if this is the last reference to the object */
  if( ulObjRefCount == 0 )
  {
     DPSP_DestroyDirectPlaySP( This );
     DPSP_DestroyIUnknown( This );
  }

  if( ulInterfaceRefCount == 0 )
  {
    HeapFree( GetProcessHeap(), 0, This );
  }

  return ulInterfaceRefCount;
}

static HRESULT WINAPI IDirectPlaySPImpl_AddMRUEntry
( LPDIRECTPLAYSP iface,
  LPCWSTR lpSection,
  LPCWSTR lpKey,
  LPCVOID lpData,
  DWORD   dwDataSize,
  DWORD   dwMaxEntries
)
{
  IDirectPlaySPImpl *This = (IDirectPlaySPImpl *)iface;

  /* Should be able to call the comctl32 undocumented MRU routines.
     I suspect that the interface works appropriately */
  FIXME( "(%p)->(%p,%p%p,0x%08x,0x%08x): stub\n",
         This, lpSection, lpKey, lpData, dwDataSize, dwMaxEntries );

  return DP_OK;
}

static HRESULT WINAPI IDirectPlaySPImpl_CreateAddress
( LPDIRECTPLAYSP iface,
  REFGUID guidSP,
  REFGUID guidDataType,
  LPCVOID lpData,
  DWORD   dwDataSize,
  LPVOID  lpAddress,
  LPDWORD lpdwAddressSize
)
{
  IDirectPlaySPImpl *This = (IDirectPlaySPImpl *)iface;

  FIXME( "(%p)->(%s,%s,%p,0x%08x,%p,%p): stub\n",
         This, debugstr_guid(guidSP), debugstr_guid(guidDataType),
         lpData, dwDataSize, lpAddress, lpdwAddressSize );

  return DP_OK;
}

static HRESULT WINAPI IDirectPlaySPImpl_EnumAddress
( LPDIRECTPLAYSP iface,
  LPDPENUMADDRESSCALLBACK lpEnumAddressCallback,
  LPCVOID lpAddress,
  DWORD dwAddressSize,
  LPVOID lpContext
)
{
  IDirectPlaySPImpl *This = (IDirectPlaySPImpl *)iface;

  TRACE( "(%p)->(%p,%p,0x%08x,%p)\n",
         This, lpEnumAddressCallback, lpAddress, dwAddressSize, lpContext );

  DPL_EnumAddress( lpEnumAddressCallback, lpAddress, dwAddressSize, lpContext );

  return DP_OK;
}

static HRESULT WINAPI IDirectPlaySPImpl_EnumMRUEntries
( LPDIRECTPLAYSP iface,
  LPCWSTR lpSection,
  LPCWSTR lpKey,
  LPENUMMRUCALLBACK lpEnumMRUCallback,
  LPVOID lpContext
)
{
  IDirectPlaySPImpl *This = (IDirectPlaySPImpl *)iface;

  /* Should be able to call the comctl32 undocumented MRU routines.
     I suspect that the interface works appropriately */
  FIXME( "(%p)->(%p,%p,%p,%p,): stub\n",
         This, lpSection, lpKey, lpEnumMRUCallback, lpContext );

  return DP_OK;
}

static HRESULT WINAPI IDirectPlaySPImpl_GetPlayerFlags
( LPDIRECTPLAYSP iface,
  DPID idPlayer,
  LPDWORD lpdwPlayerFlags
)
{
  IDirectPlaySPImpl *This = (IDirectPlaySPImpl *)iface;

  FIXME( "(%p)->(0x%08x,%p): stub\n",
         This, idPlayer, lpdwPlayerFlags );

  return DP_OK;
}

static HRESULT WINAPI IDirectPlaySPImpl_GetSPPlayerData
( LPDIRECTPLAYSP iface,
  DPID idPlayer,
  LPVOID* lplpData,
  LPDWORD lpdwDataSize,
  DWORD dwFlags
)
{
  HRESULT hr;
  LPDP_SPPLAYERDATA lpPlayerData;
  IDirectPlaySPImpl *This = (IDirectPlaySPImpl *)iface;

  TRACE( "(%p)->(0x%08x,%p,%p,0x%08x)\n",
         This, idPlayer, lplpData, lpdwDataSize, dwFlags );

  hr = DP_GetSPPlayerData( This->sp->dplay, idPlayer, (LPVOID*)&lpPlayerData );

  if( FAILED(hr) )
  {
    TRACE( "Couldn't get player data: %s\n", DPLAYX_HresultToString(hr) );
    return DPERR_INVALIDPLAYER;
  }

  /* What to do in the case where there is nothing set yet? */
  if( dwFlags == DPSET_LOCAL )
  {
    *lplpData     = lpPlayerData->lpPlayerLocalData;
    *lpdwDataSize = lpPlayerData->dwPlayerLocalDataSize;
  }
  else if( dwFlags == DPSET_REMOTE )
  {
    *lplpData     = lpPlayerData->lpPlayerRemoteData;
    *lpdwDataSize = lpPlayerData->dwPlayerRemoteDataSize;
  }

  if( *lplpData == NULL )
  {
    hr = DPERR_GENERIC;
  }

  return hr;
}

static HRESULT WINAPI IDirectPlaySPImpl_HandleMessage
( LPDIRECTPLAYSP iface,
  LPVOID lpMessageBody,
  DWORD  dwMessageBodySize,
  LPVOID lpMessageHeader
)
{
  LPDPMSG_SENDENVELOPE lpMsg = lpMessageBody;
  HRESULT hr = DPERR_GENERIC;
  WORD wCommandId;
  WORD wVersion;
  DPSP_REPLYDATA data;

  IDirectPlaySPImpl *This = (IDirectPlaySPImpl *)iface;

  FIXME( "(%p)->(%p,0x%08x,%p): mostly stub\n",
         This, lpMessageBody, dwMessageBodySize, lpMessageHeader );

  wCommandId = lpMsg->wCommandId;
  wVersion   = lpMsg->wVersion;

  TRACE( "Incoming message has envelope of 0x%08x, %u, %u\n",
         lpMsg->dwMagic, wCommandId, wVersion );

  if( lpMsg->dwMagic != DPMSGMAGIC_DPLAYMSG )
  {
    ERR( "Unknown magic 0x%08x!\n", lpMsg->dwMagic );
    return DPERR_GENERIC;
  }

#if 0
  {
    const LPDWORD lpcHeader = lpMessageHeader;

    TRACE( "lpMessageHeader = [0x%08lx] [0x%08lx] [0x%08lx] [0x%08lx] [0x%08lx]\n",
           lpcHeader[0], lpcHeader[1], lpcHeader[2], lpcHeader[3], lpcHeader[4] );
   }
#endif

  /* Pass everything else to Direct Play */
  data.lpMessage     = NULL;
  data.dwMessageSize = 0;

  /* Pass this message to the dplay interface to handle */
  hr = DP_HandleMessage( This->sp->dplay, lpMessageBody, dwMessageBodySize,
                         lpMessageHeader, wCommandId, wVersion,
                         &data.lpMessage, &data.dwMessageSize );

  if( FAILED(hr) )
  {
    ERR( "Command processing failed %s\n", DPLAYX_HresultToString(hr) );
  }

  /* Do we want a reply? */
  if( data.lpMessage != NULL )
  {
    data.lpSPMessageHeader = lpMessageHeader;
    data.idNameServer      = 0;
    data.lpISP             = iface;

    hr = (This->sp->dplay->dp2->spData.lpCB->Reply)( &data );

    if( FAILED(hr) )
    {
      ERR( "Reply failed %s\n", DPLAYX_HresultToString(hr) );
    }
  }

  return hr;

#if 0
  HRESULT hr = DP_OK;
  HANDLE  hReceiveEvent = 0;
  /* FIXME: Acquire some sort of interface lock */
  /* FIXME: Need some sort of context for this callback. Need to determine
   *        how this is actually done with the SP
   */
  /* FIXME: Who needs to delete the message when done? */
  switch( lpMsg->dwType )
  {
    case DPSYS_CREATEPLAYERORGROUP:
    {
      LPDPMSG_CREATEPLAYERORGROUP msg = lpMsg;

      if( msg->dwPlayerType == DPPLAYERTYPE_PLAYER )
      {
        hr = DP_IF_CreatePlayer( This, lpMessageHeader, msg->dpId,
                                 &msg->dpnName, 0, msg->lpData,
                                 msg->dwDataSize, msg->dwFlags, ... );
      }
      else if( msg->dwPlayerType == DPPLAYERTYPE_GROUP )
      {
        /* Group in group situation? */
        if( msg->dpIdParent == DPID_NOPARENT_GROUP )
        {
          hr = DP_IF_CreateGroup( This, lpMessageHeader, msg->dpId,
                                  &msg->dpnName, 0, msg->lpData,
                                  msg->dwDataSize, msg->dwFlags, ... );
        }
        else /* Group in Group */
        {
          hr = DP_IF_CreateGroupInGroup( This, lpMessageHeader, msg->dpIdParent,
                                         &msg->dpnName, 0, msg->lpData,
                                         msg->dwDataSize, msg->dwFlags, ... );
        }
      }
      else /* Hmmm? */
      {
        ERR( "Corrupt msg->dwPlayerType for DPSYS_CREATEPLAYERORGROUP\n" );
        return;
      }

      break;
    }

    case DPSYS_DESTROYPLAYERORGROUP:
    {
      LPDPMSG_DESTROYPLAYERORGROUP msg = lpMsg;

      if( msg->dwPlayerType == DPPLAYERTYPE_PLAYER )
      {
        hr = DP_IF_DestroyPlayer( This, msg->dpId, ... );
      }
      else if( msg->dwPlayerType == DPPLAYERTYPE_GROUP )
      {
        hr = DP_IF_DestroyGroup( This, msg->dpId, ... );
      }
      else /* Hmmm? */
      {
        ERR( "Corrupt msg->dwPlayerType for DPSYS_DESTROYPLAYERORGROUP\n" );
        return;
      }

      break;
    }

    case DPSYS_ADDPLAYERTOGROUP:
    {
      LPDPMSG_ADDPLAYERTOGROUP msg = lpMsg;

      hr = DP_IF_AddPlayerToGroup( This, msg->dpIdGroup, msg->dpIdPlayer, ... );
      break;
    }

    case DPSYS_DELETEPLAYERFROMGROUP:
    {
      LPDPMSG_DELETEPLAYERFROMGROUP msg = lpMsg;

      hr = DP_IF_DeletePlayerFromGroup( This, msg->dpIdGroup, msg->dpIdPlayer,
                                        ... );

      break;
    }

    case DPSYS_SESSIONLOST:
    {
      LPDPMSG_SESSIONLOST msg = lpMsg;

      FIXME( "DPSYS_SESSIONLOST not handled\n" );

      break;
    }

    case DPSYS_HOST:
    {
      LPDPMSG_HOST msg = lpMsg;

      FIXME( "DPSYS_HOST not handled\n" );

      break;
    }

    case DPSYS_SETPLAYERORGROUPDATA:
    {
      LPDPMSG_SETPLAYERORGROUPDATA msg = lpMsg;

      if( msg->dwPlayerType == DPPLAYERTYPE_PLAYER )
      {
        hr = DP_IF_SetPlayerData( This, msg->dpId, msg->lpData, msg->dwDataSize,                                  DPSET_REMOTE, ... );
      }
      else if( msg->dwPlayerType == DPPLAYERTYPE_GROUP )
      {
        hr = DP_IF_SetGroupData( This, msg->dpId, msg->lpData, msg->dwDataSize,
                                 DPSET_REMOTE, ... );
      }
      else /* Hmmm? */
      {
        ERR( "Corrupt msg->dwPlayerType for LPDPMSG_SETPLAYERORGROUPDATA\n" );
        return;
      }

      break;
    }

    case DPSYS_SETPLAYERORGROUPNAME:
    {
      LPDPMSG_SETPLAYERORGROUPNAME msg = lpMsg;

      if( msg->dwPlayerType == DPPLAYERTYPE_PLAYER )
      {
        hr = DP_IF_SetPlayerName( This, msg->dpId, msg->dpnName, ... );
      }
      else if( msg->dwPlayerType == DPPLAYERTYPE_GROUP )
      {
        hr = DP_IF_SetGroupName( This, msg->dpId, msg->dpnName, ... );
      }
      else /* Hmmm? */
      {
        ERR( "Corrupt msg->dwPlayerType for LPDPMSG_SETPLAYERORGROUPDATA\n" );
        return;
      }

      break;
    }

    case DPSYS_SETSESSIONDESC;
    {
      LPDPMSG_SETSESSIONDESC msg = lpMsg;

      hr = DP_IF_SetSessionDesc( This, &msg->dpDesc );

      break;
    }

    case DPSYS_ADDGROUPTOGROUP:
    {
      LPDPMSG_ADDGROUPTOGROUP msg = lpMsg;

      hr = DP_IF_AddGroupToGroup( This, msg->dpIdParentGroup, msg->dpIdGroup,
                                  ... );

      break;
    }

    case DPSYS_DELETEGROUPFROMGROUP:
    {
      LPDPMSG_DELETEGROUPFROMGROUP msg = lpMsg;

      hr = DP_IF_DeleteGroupFromGroup( This, msg->dpIdParentGroup,
                                       msg->dpIdGroup, ... );

      break;
    }

    case DPSYS_SECUREMESSAGE:
    {
      LPDPMSG_SECUREMESSAGE msg = lpMsg;

      FIXME( "DPSYS_SECUREMESSAGE not implemented\n" );

      break;
    }

    case DPSYS_STARTSESSION:
    {
      LPDPMSG_STARTSESSION msg = lpMsg;

      FIXME( "DPSYS_STARTSESSION not implemented\n" );

      break;
    }

    case DPSYS_CHAT:
    {
      LPDPMSG_CHAT msg = lpMsg;

      FIXME( "DPSYS_CHAT not implemeneted\n" );

      break;
    }

    case DPSYS_SETGROUPOWNER:
    {
      LPDPMSG_SETGROUPOWNER msg = lpMsg;

      FIXME( "DPSYS_SETGROUPOWNER not implemented\n" );

      break;
    }

    case DPSYS_SENDCOMPLETE:
    {
      LPDPMSG_SENDCOMPLETE msg = lpMsg;

      FIXME( "DPSYS_SENDCOMPLETE not implemented\n" );

      break;
    }

    default:
    {
      /* NOTE: This should be a user defined type. There is nothing that we
       *       need to do with it except queue it.
       */
      TRACE( "Received user message type(?) 0x%08lx through SP.\n",
              lpMsg->dwType );
      break;
    }
  }

  FIXME( "Queue message in the receive queue. Need some context data!\n" );

  if( FAILED(hr) )
  {
    ERR( "Unable to perform action for msg type 0x%08lx\n", lpMsg->dwType );
  }
  /* If a receive event was registered for this player, invoke it */
  if( hReceiveEvent )
  {
    SetEvent( hReceiveEvent );
  }
#endif
}

static HRESULT WINAPI IDirectPlaySPImpl_SetSPPlayerData
( LPDIRECTPLAYSP iface,
  DPID idPlayer,
  LPVOID lpData,
  DWORD dwDataSize,
  DWORD dwFlags
)
{
  HRESULT           hr;
  LPDP_SPPLAYERDATA lpPlayerEntry;
  LPVOID            lpPlayerData;

  IDirectPlaySPImpl *This = (IDirectPlaySPImpl *)iface;

/*  TRACE( "Called on process 0x%08lx\n", GetCurrentProcessId() ); */
  TRACE( "(%p)->(0x%08x,%p,0x%08x,0x%08x)\n",
         This, idPlayer, lpData, dwDataSize, dwFlags );

  hr = DP_GetSPPlayerData( This->sp->dplay, idPlayer, (LPVOID*)&lpPlayerEntry );
  if( FAILED(hr) )
  {
    /* Player must not exist */
    return DPERR_INVALIDPLAYER;
  }

  lpPlayerData = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, dwDataSize );
  CopyMemory( lpPlayerData, lpData, dwDataSize );

  if( dwFlags == DPSET_LOCAL )
  {
    lpPlayerEntry->lpPlayerLocalData = lpPlayerData;
    lpPlayerEntry->dwPlayerLocalDataSize = dwDataSize;
  }
  else if( dwFlags == DPSET_REMOTE )
  {
    lpPlayerEntry->lpPlayerRemoteData = lpPlayerData;
    lpPlayerEntry->dwPlayerRemoteDataSize = dwDataSize;
  }

  hr = DP_SetSPPlayerData( This->sp->dplay, idPlayer, lpPlayerEntry );

  return hr;
}

static HRESULT WINAPI IDirectPlaySPImpl_CreateCompoundAddress
( LPDIRECTPLAYSP iface,
  LPCDPCOMPOUNDADDRESSELEMENT lpElements,
  DWORD dwElementCount,
  LPVOID lpAddress,
  LPDWORD lpdwAddressSize
)
{
  IDirectPlaySPImpl *This = (IDirectPlaySPImpl *)iface;

  FIXME( "(%p)->(%p,0x%08x,%p,%p): stub\n",
         This, lpElements, dwElementCount, lpAddress, lpdwAddressSize );

  return DP_OK;
}

static HRESULT WINAPI IDirectPlaySPImpl_GetSPData
( LPDIRECTPLAYSP iface,
  LPVOID* lplpData,
  LPDWORD lpdwDataSize,
  DWORD dwFlags
)
{
  HRESULT hr = DP_OK;
  IDirectPlaySPImpl *This = (IDirectPlaySPImpl *)iface;

/*  TRACE( "Called on process 0x%08lx\n", GetCurrentProcessId() ); */
  TRACE( "(%p)->(%p,%p,0x%08x)\n",
         This, lplpData, lpdwDataSize, dwFlags );

#if 0
  /* This is what the documentation says... */
  if( dwFlags != DPSET_REMOTE )
  {
    return DPERR_INVALIDPARAMS;
  }
#else
  /* ... but most service providers call this with 1 */
  /* Guess that this is using a DPSET_LOCAL or DPSET_REMOTE type of
   * thing?
   */
  if( dwFlags != DPSET_REMOTE )
  {
    TRACE( "Undocumented dwFlags 0x%08x used\n", dwFlags );
  }
#endif

  /* FIXME: What to do in the case where this isn't initialized yet? */

  /* Yes, we're supposed to return a pointer to the memory we have stored! */
  if( dwFlags == DPSET_REMOTE )
  {
    *lpdwDataSize = This->sp->dwSpRemoteDataSize;
    *lplpData     = This->sp->lpSpRemoteData;

    if( This->sp->lpSpRemoteData == NULL )
    {
      hr = DPERR_GENERIC;
    }
  }
  else if( dwFlags == DPSET_LOCAL )
  {
    *lpdwDataSize = This->sp->dwSpLocalDataSize;
    *lplpData     = This->sp->lpSpLocalData;

    if( This->sp->lpSpLocalData == NULL )
    {
      hr = DPERR_GENERIC;
    }
  }

  return hr;
}

static HRESULT WINAPI IDirectPlaySPImpl_SetSPData
( LPDIRECTPLAYSP iface,
  LPVOID lpData,
  DWORD dwDataSize,
  DWORD dwFlags
)
{
  LPVOID lpSpData;

  IDirectPlaySPImpl *This = (IDirectPlaySPImpl *)iface;

/*  TRACE( "Called on process 0x%08lx\n", GetCurrentProcessId() ); */
  TRACE( "(%p)->(%p,0x%08x,0x%08x)\n",
         This, lpData, dwDataSize, dwFlags );

#if 0
  /* This is what the documentation says... */
  if( dwFlags != DPSET_REMOTE )
  {
    return DPERR_INVALIDPARAMS;
  }
#else
  /* ... but most service providers call this with 1 */
  /* Guess that this is using a DPSET_LOCAL or DPSET_REMOTE type of
   * thing?
   */
  if( dwFlags != DPSET_REMOTE )
  {
    TRACE( "Undocumented dwFlags 0x%08x used\n", dwFlags );
  }
#endif

  lpSpData = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, dwDataSize );
  CopyMemory( lpSpData, lpData, dwDataSize );

  /* If we have data already allocated, free it and replace it */
  if( dwFlags == DPSET_REMOTE )
  {
    HeapFree( GetProcessHeap(), 0, This->sp->lpSpRemoteData );
    This->sp->dwSpRemoteDataSize = dwDataSize;
    This->sp->lpSpRemoteData = lpSpData;
  }
  else if ( dwFlags == DPSET_LOCAL )
  {
    HeapFree( GetProcessHeap(), 0, This->sp->lpSpLocalData );
    This->sp->lpSpLocalData     = lpSpData;
    This->sp->dwSpLocalDataSize = dwDataSize;
  }

  return DP_OK;
}

static VOID WINAPI IDirectPlaySPImpl_SendComplete
( LPDIRECTPLAYSP iface,
  LPVOID unknownA,
  DWORD unknownB
)
{
  IDirectPlaySPImpl *This = (IDirectPlaySPImpl *)iface;

  FIXME( "(%p)->(%p,0x%08x): stub\n",
         This, unknownA, unknownB );
}

static const IDirectPlaySPVtbl directPlaySPVT =
{

  DPSP_QueryInterface,
  DPSP_AddRef,
  DPSP_Release,

  IDirectPlaySPImpl_AddMRUEntry,
  IDirectPlaySPImpl_CreateAddress,
  IDirectPlaySPImpl_EnumAddress,
  IDirectPlaySPImpl_EnumMRUEntries,
  IDirectPlaySPImpl_GetPlayerFlags,
  IDirectPlaySPImpl_GetSPPlayerData,
  IDirectPlaySPImpl_HandleMessage,
  IDirectPlaySPImpl_SetSPPlayerData,
  IDirectPlaySPImpl_CreateCompoundAddress,
  IDirectPlaySPImpl_GetSPData,
  IDirectPlaySPImpl_SetSPData,
  IDirectPlaySPImpl_SendComplete
};


/* DP external interfaces to call into DPSP interface */

/* Allocate the structure */
LPVOID DPSP_CreateSPPlayerData(void)
{
  TRACE( "Creating SPPlayer data struct\n" );
  return HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                    sizeof( DP_SPPLAYERDATA ) );
}
