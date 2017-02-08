#ifndef ACPPAGE_PRECOMP_H
#define ACPPAGE_PRECOMP_H

#define COBJMACROS
#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H
#define NTOS_MODE_USER

#include <windef.h>
#include <winbase.h>
#include <shlobj.h>
#include <atlbase.h>
#include <atlcom.h>

ULONG DbgPrint(PCH Format,...);

extern const GUID CLSID_CLayerUIPropPage;
extern HMODULE g_hModule;
extern LONG g_ModuleRefCnt;

EXTERN_C BOOL WINAPI GetExeFromLnk(PCWSTR pszLnk, PWSTR pszExe, size_t cchSize);

#include "resource.h"
#include "CLayerUIPropPage.hpp"

#endif /* ACPPAGE_PRECOMP_H */
