/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2 DLL
 * FILE:        include/catalog.h
 * PURPOSE:     Service Provider Catalog definitions
 */
#ifndef __CATALOG_H
#define __CATALOG_H

#include <ws2_32.h>
#include <wsahelp.h>

typedef struct _CATALOG_ENTRY {
    LIST_ENTRY ListEntry;
    CRITICAL_SECTION Lock;
    WCHAR LibraryName[MAX_PATH];
    HMODULE hModule;
    PWINSOCK_MAPPING Mapping;
    LPWSPSTARTUP WSPStartup;
    WSPDATA WSPData;
    WSPPROC_TABLE ProcTable;
} CATALOG_ENTRY, *PCATALOG_ENTRY;

extern LIST_ENTRY Catalog;

PCATALOG_ENTRY CreateCatalogEntry(
    LPWSTR LibraryName);

INT DestroyCatalogEntry(
    PCATALOG_ENTRY Provider);

PCATALOG_ENTRY LocateProvider(
    LPWSAPROTOCOL_INFOW lpProtocolInfo);

INT LoadProvider(
    PCATALOG_ENTRY Provider,
    LPWSAPROTOCOL_INFOW lpProtocolInfo);

INT UnloadProvider(
    PCATALOG_ENTRY Provider);

VOID CreateCatalog(VOID);

VOID DestroyCatalog(VOID);

#endif /* __CATALOG_H */

/* EOF */
