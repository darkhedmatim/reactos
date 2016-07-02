/*
 * Copyright 2013 Mislav Blažević
 * Copyright 2015 Mark Jansen
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

#ifndef APPHELP_H
#define APPHELP_H

#ifdef __cplusplus
extern "C" {
#endif

typedef WORD TAG;
typedef DWORD TAGID;
typedef DWORD TAGREF;
typedef UINT64 QWORD;

#define TAGREF_NULL (0)
#define TAGREF_ROOT (0)

typedef enum _SHIM_LOG_LEVEL {
    SHIM_ERR = 1,
    SHIM_WARN = 2,
    SHIM_INFO = 3,
}SHIM_LOG_LEVEL;

/* apphelp.c */
BOOL WINAPIV ShimDbgPrint(SHIM_LOG_LEVEL Level, PCSTR FunctionName, PCSTR Format, ...);
extern ULONG g_ShimDebugLevel;

#define SHIM_ERR(fmt, ...)  do { if (g_ShimDebugLevel) ShimDbgPrint(SHIM_ERR, __FUNCTION__, fmt, ##__VA_ARGS__ ); } while (0)
#define SHIM_WARN(fmt, ...)  do { if (g_ShimDebugLevel) ShimDbgPrint(SHIM_WARN, __FUNCTION__, fmt, ##__VA_ARGS__ ); } while (0)
#define SHIM_INFO(fmt, ...)  do { if (g_ShimDebugLevel) ShimDbgPrint(SHIM_INFO, __FUNCTION__, fmt, ##__VA_ARGS__ ); } while (0)


/* sdbapi.c */
void SdbpHeapInit(void);
void SdbpHeapDeinit(void);
#if SDBAPI_DEBUG_ALLOC

LPVOID SdbpAlloc(SIZE_T size, int line, const char* file);
LPVOID SdbpReAlloc(LPVOID mem, SIZE_T size, int line, const char* file);
void SdbpFree(LPVOID mem, int line, const char* file);

#define SdbAlloc(size) SdbpAlloc(size, __LINE__, __FILE__)
#define SdbReAlloc(mem, size) SdbpReAlloc(mem, size, __LINE__, __FILE__)
#define SdbFree(mem) SdbpFree(mem, __LINE__, __FILE__)

#else

LPVOID SdbpAlloc(SIZE_T size);
LPVOID SdbpReAlloc(LPVOID mem, SIZE_T size);
void SdbpFree(LPVOID mem);

#define SdbAlloc(size) SdbpAlloc(size)
#define SdbReAlloc(mem, size) SdbpReAlloc(mem, size)
#define SdbFree(mem) SdbpFree(mem)

#endif


/* layer.c */
BOOL WINAPI AllowPermLayer(PCWSTR path);
BOOL WINAPI SdbGetPermLayerKeys(PCWSTR wszPath, PWSTR pwszLayers, PDWORD pdwBytes, DWORD dwFlags);
BOOL WINAPI SetPermLayerState(PCWSTR wszPath, PCWSTR wszLayer, DWORD dwFlags, BOOL bMachine, BOOL bEnable);


#define TAGID_NULL 0x0
#define TAGID_ROOT 0x0

/* The above definition of TAGID_ROOT is used in winapi and can be found
 * on msdn it but doesn't make sense, especially internally, because
 * TAGID represents offset into database data and there is a header at
 * offset 0, NOT a tag. Therfore, this definition should be used internally
 * to represent first valid TAGID. Header size is 12 bytes. */
#define _TAGID_ROOT 12

#define TAG_TYPE_MASK 0xF000

#define TAG_TYPE_NULL 0x1000
#define TAG_TYPE_BYTE 0x2000
#define TAG_TYPE_WORD 0x3000
#define TAG_TYPE_DWORD 0x4000
#define TAG_TYPE_QWORD 0x5000
#define TAG_TYPE_STRINGREF 0x6000
#define TAG_TYPE_LIST 0x7000
#define TAG_TYPE_STRING 0x8000
#define TAG_TYPE_BINARY 0x9000

#define TAG_NULL 0x0

/* TAG_TYPE_NULL */
#define TAG_INCLUDE (0x1 | TAG_TYPE_NULL)
#define TAG_GENERAL (0x2 | TAG_TYPE_NULL)
#define TAG_MATCH_LOGIC_NOT (0x3 | TAG_TYPE_NULL)
#define TAG_APPLY_ALL_SHIMS (0x4 | TAG_TYPE_NULL)
#define TAG_USE_SERVICE_PACK_FILES (0x5 | TAG_TYPE_NULL)
#define TAG_MITIGATION_OS (0x6 | TAG_TYPE_NULL)
#define TAG_BLOCK_UPGRADE (0x7 | TAG_TYPE_NULL)
#define TAG_INCLUDEEXCLUDEDLL (0x8 | TAG_TYPE_NULL)
#define TAG_RAC_EVENT_OFF (0x9 | TAG_TYPE_NULL)
#define TAG_TELEMETRY_OFF (0xA | TAG_TYPE_NULL)
#define TAG_SHIM_ENGINE_OFF (0xB | TAG_TYPE_NULL)
#define TAG_LAYER_PROPAGATION_OFF (0xC | TAG_TYPE_NULL)
#define TAG_REINSTALL_UPGRADE (0xD | TAG_TYPE_NULL)

/* TAG_TYPE_BYTE */

/* TAG_TYPE_WORD */
#define TAG_MATCH_MODE (0x1 | TAG_TYPE_WORD)
#define TAG_TAG (0x801 | TAG_TYPE_WORD)
#define TAG_INDEX_TAG (0x802 | TAG_TYPE_WORD)
#define TAG_INDEX_KEY (0x803 | TAG_TYPE_WORD)

/* TAG_TYPE_DWORD */
#define TAG_SIZE (0x1 | TAG_TYPE_DWORD)
#define TAG_OFFSET (0x2 | TAG_TYPE_DWORD)
#define TAG_CHECKSUM (0x3 | TAG_TYPE_DWORD)
#define TAG_SHIM_TAGID (0x4 | TAG_TYPE_DWORD)
#define TAG_PATCH_TAGID (0x5 | TAG_TYPE_DWORD)
#define TAG_MODULE_TYPE (0x6 | TAG_TYPE_DWORD)
#define TAG_VERDATEHI (0x7 | TAG_TYPE_DWORD)
#define TAG_VERDATELO (0x8 | TAG_TYPE_DWORD)
#define TAG_VERFILEOS (0x9 | TAG_TYPE_DWORD)
#define TAG_VERFILETYPE (0xA | TAG_TYPE_DWORD)
#define TAG_PE_CHECKSUM (0xB | TAG_TYPE_DWORD)
#define TAG_PREVOSMAJORVER (0xC | TAG_TYPE_DWORD)
#define TAG_PREVOSMINORVER (0xD | TAG_TYPE_DWORD)
#define TAG_PREVOSPLATFORMID (0xE | TAG_TYPE_DWORD)
#define TAG_PREVOSBUILDNO (0xF | TAG_TYPE_DWORD)
#define TAG_PROBLEMSEVERITY (0x10 | TAG_TYPE_DWORD)
#define TAG_LANGID (0x11 | TAG_TYPE_DWORD)
#define TAG_VER_LANGUAGE (0x12 | TAG_TYPE_DWORD)
#define TAG_ENGINE (0x14 | TAG_TYPE_DWORD)
#define TAG_HTMLHELPID (0x15 | TAG_TYPE_DWORD)
#define TAG_INDEX_FLAGS (0x16 | TAG_TYPE_DWORD)
#define TAG_FLAGS (0x17 | TAG_TYPE_DWORD)
#define TAG_DATA_VALUETYPE (0x18 | TAG_TYPE_DWORD)
#define TAG_DATA_DWORD (0x19 | TAG_TYPE_DWORD)
#define TAG_LAYER_TAGID (0x1A | TAG_TYPE_DWORD)
#define TAG_MSI_TRANSFORM_TAGID (0x1B | TAG_TYPE_DWORD)
#define TAG_LINKER_VERSION (0x1C | TAG_TYPE_DWORD)
#define TAG_LINK_DATE (0x1D | TAG_TYPE_DWORD)
#define TAG_UPTO_LINK_DATE (0x1E | TAG_TYPE_DWORD)
#define TAG_OS_SERVICE_PACK (0x1F | TAG_TYPE_DWORD)
#define TAG_FLAG_TAGID (0x20 | TAG_TYPE_DWORD)
#define TAG_RUNTIME_PLATFORM (0x21 | TAG_TYPE_DWORD)
#define TAG_OS_SKU (0x22 | TAG_TYPE_DWORD)
#define TAG_OS_PLATFORM (0x23 | TAG_TYPE_DWORD)
#define TAG_APP_NAME_RC_ID (0x24 | TAG_TYPE_DWORD)
#define TAG_VENDOR_NAME_RC_ID (0x25 | TAG_TYPE_DWORD)
#define TAG_SUMMARY_MSG_RC_ID (0x26 | TAG_TYPE_DWORD)
#define TAG_VISTA_SKU (0x27 | TAG_TYPE_DWORD)
#define TAG_DESCRIPTION_RC_ID (0x28 | TAG_TYPE_DWORD)
#define TAG_PARAMETER1_RC_ID (0x29 | TAG_TYPE_DWORD)
#define TAG_CONTEXT_TAGID (0x30 | TAG_TYPE_DWORD)
#define TAG_EXE_WRAPPER (0x31 | TAG_TYPE_DWORD)
#define TAG_URL_ID (0x32 | TAG_TYPE_DWORD)
#define TAG_TAGID (0x801 | TAG_TYPE_DWORD)

/* TAG_TYPE_QWORD */
#define TAG_TIME (0x1 | TAG_TYPE_QWORD)
#define TAG_BIN_FILE_VERSION (0x2 | TAG_TYPE_QWORD)
#define TAG_BIN_PRODUCT_VERSION (0x3 | TAG_TYPE_QWORD)
#define TAG_MODTIME (0x4 | TAG_TYPE_QWORD)
#define TAG_FLAG_MASK_KERNEL (0x5 | TAG_TYPE_QWORD)
#define TAG_UPTO_BIN_PRODUCT_VERSION (0x6 | TAG_TYPE_QWORD)
#define TAG_DATA_QWORD (0x7 | TAG_TYPE_QWORD)
#define TAG_FLAG_MASK_USER (0x8 | TAG_TYPE_QWORD)
#define TAG_FLAGS_NTVDM1 (0x9 | TAG_TYPE_QWORD)
#define TAG_FLAGS_NTVDM2 (0xA | TAG_TYPE_QWORD)
#define TAG_FLAGS_NTVDM3 (0xB | TAG_TYPE_QWORD)
#define TAG_FLAG_MASK_SHELL (0xC | TAG_TYPE_QWORD)
#define TAG_UPTO_BIN_FILE_VERSION (0xD | TAG_TYPE_QWORD)
#define TAG_FLAG_MASK_FUSION (0xE | TAG_TYPE_QWORD)
#define TAG_FLAG_PROCESSPARAM (0xF | TAG_TYPE_QWORD)
#define TAG_FLAG_LUA (0x10 | TAG_TYPE_QWORD)
#define TAG_FLAG_INSTALL (0x11 | TAG_TYPE_QWORD)

/* TAG_TYPE_STRINGREF */
#define TAG_NAME (0x1 | TAG_TYPE_STRINGREF)
#define TAG_DESCRIPTION (0x2 | TAG_TYPE_STRINGREF)
#define TAG_MODULE (0x3 | TAG_TYPE_STRINGREF)
#define TAG_API (0x4 | TAG_TYPE_STRINGREF)
#define TAG_VENDOR (0x5 | TAG_TYPE_STRINGREF)
#define TAG_APP_NAME (0x6 | TAG_TYPE_STRINGREF)
#define TAG_COMMAND_LINE (0x8 | TAG_TYPE_STRINGREF)
#define TAG_COMPANY_NAME (0x9 | TAG_TYPE_STRINGREF)
#define TAG_DLLFILE (0xA | TAG_TYPE_STRINGREF)
#define TAG_WILDCARD_NAME (0xB | TAG_TYPE_STRINGREF)
#define TAG_PRODUCT_NAME (0x10 | TAG_TYPE_STRINGREF)
#define TAG_PRODUCT_VERSION (0x11 | TAG_TYPE_STRINGREF)
#define TAG_FILE_DESCRIPTION (0x12 | TAG_TYPE_STRINGREF)
#define TAG_FILE_VERSION (0x13 | TAG_TYPE_STRINGREF)
#define TAG_ORIGINAL_FILENAME (0x14 | TAG_TYPE_STRINGREF)
#define TAG_INTERNAL_NAME (0x15 | TAG_TYPE_STRINGREF)
#define TAG_LEGAL_COPYRIGHT (0x16 | TAG_TYPE_STRINGREF)
#define TAG_16BIT_DESCRIPTION (0x17 | TAG_TYPE_STRINGREF)
#define TAG_APPHELP_DETAILS (0x18 | TAG_TYPE_STRINGREF)
#define TAG_LINK_URL (0x19 | TAG_TYPE_STRINGREF)
#define TAG_LINK_TEXT (0x1A | TAG_TYPE_STRINGREF)
#define TAG_APPHELP_TITLE (0x1B | TAG_TYPE_STRINGREF)
#define TAG_APPHELP_CONTACT (0x1C | TAG_TYPE_STRINGREF)
#define TAG_SXS_MANIFEST (0x1D | TAG_TYPE_STRINGREF)
#define TAG_DATA_STRING (0x1E | TAG_TYPE_STRINGREF)
#define TAG_MSI_TRANSFORM_FILE (0x1F | TAG_TYPE_STRINGREF)
#define TAG_16BIT_MODULE_NAME (0x20 | TAG_TYPE_STRINGREF)
#define TAG_LAYER_DISPLAYNAME (0x21 | TAG_TYPE_STRINGREF)
#define TAG_COMPILER_VERSION (0x22 | TAG_TYPE_STRINGREF)
#define TAG_ACTION_TYPE (0x23 | TAG_TYPE_STRINGREF)
#define TAG_EXPORT_NAME (0x24 | TAG_TYPE_STRINGREF)
#define TAG_URL (0x25 | TAG_TYPE_STRINGREF)

/* TAG_TYPE_LIST */
#define TAG_DATABASE (0x1 | TAG_TYPE_LIST)
#define TAG_LIBRARY (0x2 | TAG_TYPE_LIST)
#define TAG_INEXCLUD (0x3 | TAG_TYPE_LIST)
#define TAG_SHIM (0x4 | TAG_TYPE_LIST)
#define TAG_PATCH (0x5 | TAG_TYPE_LIST)
#define TAG_APP (0x6 | TAG_TYPE_LIST)
#define TAG_EXE (0x7 | TAG_TYPE_LIST)
#define TAG_MATCHING_FILE (0x8 | TAG_TYPE_LIST)
#define TAG_SHIM_REF (0x9| TAG_TYPE_LIST)
#define TAG_PATCH_REF (0xA | TAG_TYPE_LIST)
#define TAG_LAYER (0xB | TAG_TYPE_LIST)
#define TAG_FILE (0xC | TAG_TYPE_LIST)
#define TAG_APPHELP (0xD | TAG_TYPE_LIST)
#define TAG_LINK (0xE | TAG_TYPE_LIST)
#define TAG_DATA (0xF | TAG_TYPE_LIST)
#define TAG_MSI_TRANSFORM (0x10 | TAG_TYPE_LIST)
#define TAG_MSI_TRANSFORM_REF (0x11 | TAG_TYPE_LIST)
#define TAG_MSI_PACKAGE (0x12 | TAG_TYPE_LIST)
#define TAG_FLAG (0x13 | TAG_TYPE_LIST)
#define TAG_MSI_CUSTOM_ACTION (0x14 | TAG_TYPE_LIST)
#define TAG_FLAG_REF (0x15 | TAG_TYPE_LIST)
#define TAG_ACTION (0x16 | TAG_TYPE_LIST)
#define TAG_LOOKUP (0x17 | TAG_TYPE_LIST)
#define TAG_CONTEXT (0x18 | TAG_TYPE_LIST)
#define TAG_CONTEXT_REF (0x19 | TAG_TYPE_LIST)
#define TAG_SPC (0x20 | TAG_TYPE_LIST)
#define TAG_STRINGTABLE (0x801 | TAG_TYPE_LIST)
#define TAG_INDEXES (0x802 | TAG_TYPE_LIST)
#define TAG_INDEX (0x803 | TAG_TYPE_LIST)

/* TAG_TYPE_STRING */
#define TAG_STRINGTABLE_ITEM (0x801 | TAG_TYPE_STRING)

/* TAG_TYPE_BINARY */
#define TAG_PATCH_BITS (0x2 | TAG_TYPE_BINARY)
#define TAG_FILE_BITS (0x3 | TAG_TYPE_BINARY)
#define TAG_EXE_ID (0x4 | TAG_TYPE_BINARY)
#define TAG_DATA_BITS (0x5 | TAG_TYPE_BINARY)
#define TAG_MSI_PACKAGE_ID (0x6 | TAG_TYPE_BINARY)
#define TAG_DATABASE_ID (0x7 | TAG_TYPE_BINARY)
#define TAG_CONTEXT_PLATFORM_ID (0x8 | TAG_TYPE_BINARY)
#define TAG_CONTEXT_BRANCH_ID (0x9 | TAG_TYPE_BINARY)
#define TAG_FIX_ID (0x10 | TAG_TYPE_BINARY)
#define TAG_APP_ID (0x11 | TAG_TYPE_BINARY)
#define TAG_INDEX_BITS (0x801 | TAG_TYPE_BINARY)

#ifdef __cplusplus
} // extern "C"
#endif

#endif // APPHELP_H
