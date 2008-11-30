/*
	imagehlp.h - Include file for IMAGEHLP.DLL APIs

	Written by Mumit Khan <khan@nanotech.wisc.edu>

	This file is part of a free library for the Win32 API.

	NOTE: This strictly does not belong in the Win32 API since it's
	really part of Platform SDK. However,GDB needs it and we might
	as well provide it here.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

*/

#ifndef _IMAGEHLP_H
#define _IMAGEHLP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <wintrust.h>

#define API_VERSION_NUMBER 7
#define BIND_NO_BOUND_IMPORTS 0x00000001
#define BIND_NO_UPDATE 0x00000002
#define BIND_ALL_IMAGES 0x00000004
#define BIND_CACHE_IMPORT_DLLS 0x00000008
#define CBA_DEFERRED_SYMBOL_LOAD_START 1
#define CBA_DEFERRED_SYMBOL_LOAD_COMPLETE 2
#define CBA_DEFERRED_SYMBOL_LOAD_FAILURE 3
#define CBA_SYMBOLS_UNLOADED 4
#define CBA_DUPLICATE_SYMBOL 5
#define CERT_PE_IMAGE_DIGEST_DEBUG_INFO 1
#define CERT_PE_IMAGE_DIGEST_RESOURCES 2
#define CERT_PE_IMAGE_DIGEST_ALL_IMPORT_INFO 4
#define CERT_PE_IMAGE_DIGEST_NON_PE_INFO 8
#define CERT_SECTION_TYPE_ANY 255
#define CHECKSUM_SUCCESS	0
#define CHECKSUM_OPEN_FAILURE	1
#define CHECKSUM_MAP_FAILURE	2
#define CHECKSUM_MAPVIEW_FAILURE	3
#define CHECKSUM_UNICODE_FAILURE	4
#define IMAGE_SEPARATION	65536
#define SPLITSYM_REMOVE_PRIVATE	1
#define SPLITSYM_EXTRACT_ALL	2
#define SPLITSYM_SYMBOLPATH_IS_SRC	4
#define SYMF_OMAP_GENERATED	1
#define SYMF_OMAP_MODIFIED	2
#define SYMOPT_CASE_INSENSITIVE	1
#define SYMOPT_UNDNAME	2
#define SYMOPT_DEFERRED_LOADS	4
#define SYMOPT_NO_CPP	8
#define SYMOPT_LOAD_LINES	16
#define SYMOPT_OMAP_FIND_NEAREST	32
#define UNDNAME_COMPLETE	0
#define UNDNAME_NO_LEADING_UNDERSCORES	1
#define UNDNAME_NO_MS_KEYWORDS	2
#define UNDNAME_NO_FUNCTION_RETURNS	4
#define UNDNAME_NO_ALLOCATION_MODEL	8
#define UNDNAME_NO_ALLOCATION_LANGUAGE	16
#define UNDNAME_NO_MS_THISTYPE	32
#define UNDNAME_NO_CV_THISTYPE	64
#define UNDNAME_NO_THISTYPE	96
#define UNDNAME_NO_ACCESS_SPECIFIERS	128
#define UNDNAME_NO_THROW_SIGNATURES	256
#define UNDNAME_NO_MEMBER_TYPE	512
#define UNDNAME_NO_RETURN_UDT_MODEL	1024
#define UNDNAME_32_BIT_DECODE	2048
#define UNDNAME_NAME_ONLY	4096
#define UNDNAME_NO_ARGUMENTS	8192
#define UNDNAME_NO_SPECIAL_SYMS	16384

#ifdef _IMAGEHLP_SOURCE_
    #define IMAGEAPI __stdcall
    #define DBHLP_DEPRECIATED
#else
    #define IMAGEAPI DECLSPEC_IMPORT __stdcall
    #if (_MSC_VER >= 1300) && !defined(MIDL_PASS)
        #define DBHLP_DEPRECIATED __declspec(deprecated)
    #else
        #define DBHLP_DEPRECIATED
    #endif
#endif

#define DBHLPAPI IMAGEAPI

#ifndef RC_INVOKED

typedef enum _IMAGEHLP_STATUS_REASON {
	BindOutOfMemory,
	BindRvaToVaFailed,
	BindNoRoomInImage,
	BindImportModuleFailed,
	BindImportProcedureFailed,
	BindImportModule,
	BindImportProcedure,
	BindForwarder,
	BindForwarderNOT,
	BindImageModified,
	BindExpandFileHeaders,
	BindImageComplete,
	BindMismatchedSymbols,
	BindSymbolsNotUpdated
} IMAGEHLP_STATUS_REASON;
typedef BOOL(WINAPI*PIMAGEHLP_STATUS_ROUTINE)(IMAGEHLP_STATUS_REASON,LPSTR,LPSTR,ULONG,ULONG);
typedef struct _LOADED_IMAGE {
	LPSTR ModuleName;
	HANDLE hFile;
	PUCHAR MappedAddress;
	PIMAGE_NT_HEADERS FileHeader;
	PIMAGE_SECTION_HEADER LastRvaSection;
	ULONG NumberOfSections;
	PIMAGE_SECTION_HEADER Sections;
	ULONG Characteristics;
	BOOLEAN fSystemImage;
	BOOLEAN fDOSImage;
	LIST_ENTRY Links;
	ULONG SizeOfImage;
} LOADED_IMAGE,*PLOADED_IMAGE;
typedef struct _IMAGE_DEBUG_INFORMATION {
	LIST_ENTRY List;
	DWORD Size;
	PVOID MappedBase;
	USHORT Machine;
	USHORT Characteristics;
	DWORD CheckSum;
	DWORD ImageBase;
	DWORD SizeOfImage;
	DWORD NumberOfSections;
	PIMAGE_SECTION_HEADER Sections;
	DWORD ExportedNamesSize;
	LPSTR ExportedNames;
	DWORD NumberOfFunctionTableEntries;
	PIMAGE_FUNCTION_ENTRY FunctionTableEntries;
	DWORD LowestFunctionStartingAddress;
	DWORD HighestFunctionEndingAddress;
	DWORD NumberOfFpoTableEntries;
	PFPO_DATA FpoTableEntries;
	DWORD SizeOfCoffSymbols;
	PIMAGE_COFF_SYMBOLS_HEADER CoffSymbols;
	DWORD SizeOfCodeViewSymbols;
	PVOID CodeViewSymbols;
	LPSTR ImageFilePath;
	LPSTR ImageFileName;
	LPSTR DebugFilePath;
	DWORD TimeDateStamp;
	BOOL RomImage;
	PIMAGE_DEBUG_DIRECTORY DebugDirectory;
	DWORD NumberOfDebugDirectories;
	DWORD Reserved[3];
} IMAGE_DEBUG_INFORMATION,*PIMAGE_DEBUG_INFORMATION;
typedef enum {
	AddrMode1616,
	AddrMode1632,
	AddrModeReal,
	AddrModeFlat
} ADDRESS_MODE;
typedef struct _tagADDRESS {
	DWORD Offset;
	WORD Segment;
	ADDRESS_MODE Mode;
} ADDRESS,*LPADDRESS;
typedef struct _KDHELP {
	DWORD Thread;
	DWORD ThCallbackStack;
	DWORD NextCallback;
	DWORD FramePointer;
	DWORD KiCallUserMode;
	DWORD KeUserCallbackDispatcher;
	DWORD SystemRangeStart;
} KDHELP,*PKDHELP;
typedef struct _tagSTACKFRAME {
	ADDRESS AddrPC;
	ADDRESS AddrReturn;
	ADDRESS AddrFrame;
	ADDRESS AddrStack;
	LPVOID FuncTableEntry;
	DWORD Params[4];
	BOOL Far;
	BOOL Virtual;
	DWORD Reserved[3];
	KDHELP KdHelp;
} STACKFRAME,*LPSTACKFRAME;
typedef BOOL(WINAPI*PREAD_PROCESS_MEMORY_ROUTINE)(HANDLE ,LPCVOID,LPVOID,DWORD,LPDWORD);
typedef LPVOID(WINAPI*PFUNCTION_TABLE_ACCESS_ROUTINE)(HANDLE,DWORD);
typedef DWORD(WINAPI*PGET_MODULE_BASE_ROUTINE)(HANDLE,DWORD);
typedef DWORD(WINAPI*PTRANSLATE_ADDRESS_ROUTINE)(HANDLE,HANDLE,LPADDRESS);
typedef struct API_VERSION {
	USHORT MajorVersion;
	USHORT MinorVersion;
	USHORT Revision;
	USHORT Reserved;
} API_VERSION,*LPAPI_VERSION;
typedef BOOL(CALLBACK*PSYM_ENUMMODULES_CALLBACK)(LPSTR,ULONG,PVOID);
typedef BOOL(CALLBACK*PSYM_ENUMSYMBOLS_CALLBACK)(LPSTR,ULONG,ULONG,PVOID);
typedef BOOL(CALLBACK*PENUMLOADED_MODULES_CALLBACK)(LPSTR,ULONG,ULONG,PVOID);
typedef BOOL(CALLBACK*PSYMBOL_REGISTERED_CALLBACK)(HANDLE,ULONG,PVOID,PVOID);
typedef enum {
	SymNone,
	SymCoff,
	SymCv,
	SymPdb,
	SymExport,
	SymDeferred,
	SymSym
} SYM_TYPE;
typedef struct _IMAGEHLP_SYMBOL {
	DWORD SizeOfStruct;
	DWORD Address;
	DWORD Size;
	DWORD Flags;
	DWORD MaxNameLength;
	CHAR Name[1];
} IMAGEHLP_SYMBOL,*PIMAGEHLP_SYMBOL;
typedef struct _IMAGEHLP_MODULE {
	DWORD SizeOfStruct;
	DWORD BaseOfImage;
	DWORD ImageSize;
	DWORD TimeDateStamp;
	DWORD CheckSum;
	DWORD NumSyms;
	SYM_TYPE SymType;
	CHAR ModuleName[32];
	CHAR ImageName[256];
	CHAR LoadedImageName[256];
} IMAGEHLP_MODULE,*PIMAGEHLP_MODULE;
typedef struct _IMAGEHLP_LINE {
	DWORD SizeOfStruct;
	DWORD Key;
	DWORD LineNumber;
	PCHAR FileName;
	DWORD Address;
} IMAGEHLP_LINE,*PIMAGEHLP_LINE;
typedef struct _IMAGEHLP_DEFERRED_SYMBOL_LOAD {
	DWORD SizeOfStruct;
	DWORD BaseOfImage;
	DWORD CheckSum;
	DWORD TimeDateStamp;
	CHAR FileName[MAX_PATH];
	BOOLEAN Reparse;
} IMAGEHLP_DEFERRED_SYMBOL_LOAD,*PIMAGEHLP_DEFERRED_SYMBOL_LOAD;
typedef struct _IMAGEHLP_DUPLICATE_SYMBOL {
	DWORD SizeOfStruct;
	DWORD NumberOfDups;
	PIMAGEHLP_SYMBOL Symbol;
	ULONG SelectedSymbol;
} IMAGEHLP_DUPLICATE_SYMBOL,*PIMAGEHLP_DUPLICATE_SYMBOL;
typedef PVOID DIGEST_HANDLE;
typedef BOOL(WINAPI*DIGEST_FUNCTION)(DIGEST_HANDLE refdata,PBYTE pData,DWORD dwLength);

PIMAGE_NT_HEADERS IMAGEAPI CheckSumMappedFile(LPVOID,DWORD,LPDWORD,LPDWORD);
DWORD IMAGEAPI MapFileAndCheckSumA(LPSTR,LPDWORD,LPDWORD);
DWORD IMAGEAPI MapFileAndCheckSumW(PWSTR,LPDWORD,LPDWORD);
BOOL IMAGEAPI TouchFileTimes(HANDLE,LPSYSTEMTIME);
BOOL IMAGEAPI SplitSymbols(LPSTR,LPSTR,LPSTR,DWORD);
HANDLE IMAGEAPI FindDebugInfoFile(LPSTR,LPSTR,LPSTR);
HANDLE IMAGEAPI FindExecutableImage(LPSTR,LPSTR,LPSTR);
BOOL IMAGEAPI UpdateDebugInfoFile(LPSTR,LPSTR,LPSTR,PIMAGE_NT_HEADERS);
BOOL IMAGEAPI UpdateDebugInfoFileEx(LPSTR,LPSTR,LPSTR,PIMAGE_NT_HEADERS,DWORD);
BOOL IMAGEAPI BindImage(IN LPSTR,IN LPSTR,IN LPSTR);
BOOL IMAGEAPI BindImageEx(IN DWORD,IN LPSTR,IN LPSTR,IN LPSTR,IN PIMAGEHLP_STATUS_ROUTINE);
BOOL IMAGEAPI ReBaseImage(IN LPSTR,IN LPSTR,IN BOOL, IN BOOL,IN BOOL, IN ULONG, OUT ULONG*, OUT ULONG*, OUT ULONG*, IN OUT ULONG*, IN ULONG);

PLOADED_IMAGE IMAGEAPI ImageLoad(LPSTR,LPSTR);
BOOL IMAGEAPI ImageUnload(PLOADED_IMAGE);
PIMAGE_NT_HEADERS IMAGEAPI ImageNtHeader(IN PVOID);
PVOID IMAGEAPI ImageDirectoryEntryToData(IN PVOID,IN BOOLEAN,IN USHORT,OUT PULONG);
PVOID IMAGEAPI ImageDirectoryEntryToDataEx(IN PVOID Base, IN BOOLEAN MappedAsImage, IN USHORT DirectoryEntry, OUT PULONG Size, OUT PIMAGE_SECTION_HEADER *FoundSection OPTIONAL);
PVOID IMAGEAPI ImageDirectoryEntryToData32(PVOID Base, BOOLEAN MappedAsImage, USHORT DirectoryEntry, PULONG Size, PIMAGE_SECTION_HEADER *FoundHeader OPTIONAL, PIMAGE_FILE_HEADER FileHeader, PIMAGE_OPTIONAL_HEADER OptionalHeader);
PIMAGE_SECTION_HEADER IMAGEAPI ImageRvaToSection(IN PIMAGE_NT_HEADERS,IN PVOID,IN ULONG);
PVOID IMAGEAPI ImageRvaToVa(IN PIMAGE_NT_HEADERS,IN PVOID,IN ULONG,IN OUT PIMAGE_SECTION_HEADER*);
BOOL IMAGEAPI MapAndLoad(LPSTR,LPSTR,PLOADED_IMAGE,BOOL,BOOL);
BOOL IMAGEAPI GetImageConfigInformation(PLOADED_IMAGE,PIMAGE_LOAD_CONFIG_DIRECTORY);
DWORD IMAGEAPI GetImageUnusedHeaderBytes(PLOADED_IMAGE,LPDWORD);
BOOL IMAGEAPI SetImageConfigInformation(PLOADED_IMAGE,PIMAGE_LOAD_CONFIG_DIRECTORY);
BOOL IMAGEAPI UnMapAndLoad(PLOADED_IMAGE);
PIMAGE_DEBUG_INFORMATION IMAGEAPI MapDebugInformation(HANDLE,LPSTR,LPSTR,DWORD);
BOOL IMAGEAPI UnmapDebugInformation(PIMAGE_DEBUG_INFORMATION);
HANDLE IMAGEAPI FindExecutableImage(LPSTR,LPSTR,LPSTR);
BOOL IMAGEAPI SearchTreeForFile(LPSTR,LPSTR,LPSTR);
BOOL IMAGEAPI MakeSureDirectoryPathExists(LPCSTR);
DWORD IMAGEAPI WINAPI UnDecorateSymbolName(LPCSTR,LPSTR,DWORD, DWORD);
BOOL
IMAGEAPI
StackWalk(DWORD,HANDLE,HANDLE,LPSTACKFRAME,LPVOID,PREAD_PROCESS_MEMORY_ROUTINE,PFUNCTION_TABLE_ACCESS_ROUTINE,PGET_MODULE_BASE_ROUTINE,PTRANSLATE_ADDRESS_ROUTINE);
LPAPI_VERSION IMAGEAPI ImagehlpApiVersion(VOID);
LPAPI_VERSION IMAGEAPI ImagehlpApiVersionEx(LPAPI_VERSION);
DWORD IMAGEAPI GetTimestampForLoadedLibrary(HMODULE);
BOOL IMAGEAPI RemovePrivateCvSymbolic(PCHAR,PCHAR*,ULONG*);
VOID IMAGEAPI RemoveRelocations(PCHAR);
DWORD IMAGEAPI SymSetOptions(IN DWORD);
DWORD IMAGEAPI SymGetOptions(VOID);
BOOL IMAGEAPI SymCleanup(IN HANDLE);
BOOL IMAGEAPI SymEnumerateModules(IN HANDLE,IN PSYM_ENUMMODULES_CALLBACK,IN PVOID);
BOOL IMAGEAPI SymEnumerateSymbols(IN HANDLE,IN DWORD,IN PSYM_ENUMSYMBOLS_CALLBACK,IN PVOID);
BOOL IMAGEAPI EnumerateLoadedModules(IN HANDLE,IN PENUMLOADED_MODULES_CALLBACK,IN PVOID);
LPVOID IMAGEAPI SymFunctionTableAccess(HANDLE,DWORD);
BOOL IMAGEAPI SymGetModuleInfo(IN HANDLE,IN DWORD,OUT PIMAGEHLP_MODULE);
DWORD IMAGEAPI SymGetModuleBase(IN HANDLE,IN DWORD);
BOOL IMAGEAPI SymGetSymFromAddr(IN HANDLE,IN DWORD,OUT PDWORD,OUT PIMAGEHLP_SYMBOL);
BOOL IMAGEAPI SymGetSymFromName(IN HANDLE,IN LPSTR,OUT PIMAGEHLP_SYMBOL);
BOOL IMAGEAPI SymGetSymNext(IN HANDLE,IN OUT PIMAGEHLP_SYMBOL);
BOOL IMAGEAPI SymGetSymPrev(IN HANDLE,IN OUT PIMAGEHLP_SYMBOL);
BOOL IMAGEAPI SymGetLineFromAddr(IN HANDLE,IN DWORD,OUT PDWORD,OUT PIMAGEHLP_LINE);
BOOL IMAGEAPI SymGetLineFromName(IN HANDLE,IN LPSTR,IN LPSTR,IN DWORD,OUT PLONG,IN OUT PIMAGEHLP_LINE);
BOOL IMAGEAPI SymGetLineNext(IN HANDLE,IN OUT PIMAGEHLP_LINE);
BOOL IMAGEAPI SymGetLinePrev(IN HANDLE,IN OUT PIMAGEHLP_LINE);
BOOL IMAGEAPI SymMatchFileName(IN LPSTR,IN LPSTR,OUT LPSTR*,OUT LPSTR*);
BOOL IMAGEAPI SymInitialize(IN HANDLE,IN LPSTR,IN BOOL);
BOOL IMAGEAPI SymGetSearchPath(IN HANDLE,OUT LPSTR,IN DWORD);
BOOL IMAGEAPI SymSetSearchPath(IN HANDLE,IN LPSTR);
DWORD IMAGEAPI SymLoadModule(IN HANDLE,IN HANDLE,IN PSTR,IN PSTR,IN DWORD,IN DWORD);
BOOL IMAGEAPI SymUnloadModule(IN HANDLE,IN DWORD);
BOOL IMAGEAPI SymUnDName(IN PIMAGEHLP_SYMBOL,OUT LPSTR,IN DWORD);
BOOL IMAGEAPI SymRegisterCallback(IN HANDLE,IN PSYMBOL_REGISTERED_CALLBACK,IN PVOID);
BOOL IMAGEAPI ImageGetDigestStream(IN HANDLE,IN DWORD,IN DIGEST_FUNCTION,IN DIGEST_HANDLE);
BOOL IMAGEAPI ImageAddCertificate(IN HANDLE,IN LPWIN_CERTIFICATE,OUT PDWORD);
BOOL IMAGEAPI ImageRemoveCertificate(IN HANDLE,IN DWORD);
BOOL IMAGEAPI ImageEnumerateCertificates(IN HANDLE,IN WORD,OUT PDWORD,IN OUT PDWORD OPTIONAL,IN OUT DWORD OPTIONAL);
BOOL IMAGEAPI ImageGetCertificateData(IN HANDLE,IN DWORD,OUT LPWIN_CERTIFICATE,IN OUT PDWORD);
BOOL IMAGEAPI ImageGetCertificateHeader(IN HANDLE,IN DWORD,IN OUT LPWIN_CERTIFICATE);
BOOL IMAGEAPI CopyPdb(CHAR const*,CHAR const*,BOOL);
BOOL IMAGEAPI RemovePrivateCvSymbolicEx(PCHAR,ULONG,PCHAR*,ULONG*);

#endif /* RC_INVOKED */

#ifdef UNICODE
#define MapFileAndCheckSum MapFileAndCheckSumW
#else
#define MapFileAndCheckSum MapFileAndCheckSumA
#endif

#ifdef __cplusplus
}
#endif

#endif /* _IMAGEHLP_H */

