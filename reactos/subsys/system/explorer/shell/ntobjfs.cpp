/*
 * Copyright 2003 Martin Fuchs
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


 //
 // Explorer clone
 //
 // ntobjfs.cpp
 //
 // Martin Fuchs, 31.01.2004
 //


#include "../utility/utility.h"
#include "../utility/shellclasses.h"

#include "entries.h"
#include "ntobjfs.h"
#include "regfs.h"


#define	CONSTRUCT_NTDLLFCT(x) x(TEXT("NTDLL"), #x)

typedef DWORD (__stdcall* NTOBJECTOPENFUNCTIONS)(HANDLE*, DWORD, OpenStruct*);

struct NTDLL {
	NTDLL()
	 :	CONSTRUCT_NTDLLFCT(RtlInitAnsiString),
		CONSTRUCT_NTDLLFCT(RtlInitUnicodeString),
		CONSTRUCT_NTDLLFCT(RtlFreeAnsiString),
		CONSTRUCT_NTDLLFCT(RtlFreeUnicodeString),
		CONSTRUCT_NTDLLFCT(RtlAnsiStringToUnicodeString),
		CONSTRUCT_NTDLLFCT(RtlUnicodeStringToAnsiString),
		CONSTRUCT_NTDLLFCT(NtOpenDirectoryObject),
		CONSTRUCT_NTDLLFCT(NtQueryDirectoryObject),
		CONSTRUCT_NTDLLFCT(NtOpenFile),
		CONSTRUCT_NTDLLFCT(NtOpenSymbolicLinkObject),
		CONSTRUCT_NTDLLFCT(NtQuerySymbolicLinkObject),
		CONSTRUCT_NTDLLFCT(NtQueryObject),
		CONSTRUCT_NTDLLFCT(NtOpenMutant),
		CONSTRUCT_NTDLLFCT(NtOpenSection),
		CONSTRUCT_NTDLLFCT(NtOpenEvent),
		CONSTRUCT_NTDLLFCT(NtOpenEventPair),
		CONSTRUCT_NTDLLFCT(NtOpenIoCompletion),
		CONSTRUCT_NTDLLFCT(NtOpenSemaphore),
		CONSTRUCT_NTDLLFCT(NtOpenTimer),
		CONSTRUCT_NTDLLFCT(NtOpenKey),
		CONSTRUCT_NTDLLFCT(NtClose),
		CONSTRUCT_NTDLLFCT(NtOpenProcess),
		CONSTRUCT_NTDLLFCT(NtOpenThread)
	{
		NTOBJECTOPENFUNCTIONS* p = _ObjectOpenFunctions;

		*p++ = *NtOpenDirectoryObject;
		*p++ = *NtOpenSymbolicLinkObject;
		*p++ = *NtOpenMutant;
		*p++ = *NtOpenSection;
		*p++ = *NtOpenEvent;
		*p++ = *NtOpenSemaphore;
		*p++ = *NtOpenTimer;
		*p++ = *NtOpenKey;
		*p++ = *NtOpenEventPair;
		*p++ = *NtOpenIoCompletion;
		*p++ = 0/*Device Object*/;
		*p++ = 0/*NtOpenFile*/;
		*p++ = 0/*CONTROLLER_OBJECT*/;
		*p++ = 0/*PROFILE_OBJECT*/;
		*p++ = 0/*TYPE_OBJECT*/;
		*p++ = 0/*DESKTOP_OBJECT*/;
		*p++ = 0/*WINDOWSTATION_OBJECT*/;
		*p++ = 0/*DRIVER_OBJECT*/;
		*p++ = 0/*TOKEN_OBJECT*/;
		*p++ = 0/*PROCESS_OBJECT*/;
		*p++ = 0/*THREAD_OBJECT*/;
		*p++ = 0/*ADAPTER_OBJECT*/;
		*p++ = 0/*PORT_OBJECT*/;
	}

	NTOBJECTOPENFUNCTIONS _ObjectOpenFunctions[23];
	static const LPCWSTR s_ObjectTypes[];

	DynamicFct<void (__stdcall*)(RtlAnsiString*, LPCSTR)> RtlInitAnsiString;
	DynamicFct<void (__stdcall*)(RtlUnicodeString*, LPCWSTR)> RtlInitUnicodeString;
	DynamicFct<DWORD (__stdcall*)(RtlAnsiString*)> RtlFreeAnsiString;
	DynamicFct<DWORD (__stdcall*)(RtlUnicodeString*)> RtlFreeUnicodeString;
	DynamicFct<DWORD (__stdcall*)(RtlUnicodeString*, const RtlAnsiString*, BOOL)> RtlAnsiStringToUnicodeString;
	DynamicFct<DWORD (__stdcall*)(RtlAnsiString*, const RtlUnicodeString*, BOOL)> RtlUnicodeStringToAnsiString;

	DynamicFct<DWORD (__stdcall*)(HANDLE*, DWORD, OpenStruct*)> NtOpenDirectoryObject;
	DynamicFct<DWORD (__stdcall*)(HANDLE, NtObjectInfo*, DWORD size, BOOL, BOOL, void*, void*)> NtQueryDirectoryObject;
	DynamicFct<DWORD (__stdcall*)(HANDLE*, DWORD, void*, DWORD*, DWORD, OpenStruct*)> NtOpenFile;
	DynamicFct<DWORD (__stdcall*)(HANDLE*, DWORD, OpenStruct*)> NtOpenSymbolicLinkObject;
	DynamicFct<DWORD (__stdcall*)(HANDLE, RtlUnicodeString*, DWORD*)> NtQuerySymbolicLinkObject;
	DynamicFct<DWORD (__stdcall*)(HANDLE, DWORD, NtObject*, DWORD size, DWORD* read)> NtQueryObject;
	DynamicFct<DWORD (__stdcall*)(HANDLE*, DWORD, OpenStruct*)> NtOpenMutant;
	DynamicFct<DWORD (__stdcall*)(HANDLE*, DWORD, OpenStruct*)> NtOpenSection;
	DynamicFct<DWORD (__stdcall*)(HANDLE*, DWORD, OpenStruct*)> NtOpenEvent;
	DynamicFct<DWORD (__stdcall*)(HANDLE*, DWORD, OpenStruct*)> NtOpenEventPair;
	DynamicFct<DWORD (__stdcall*)(HANDLE*, DWORD, OpenStruct*)> NtOpenIoCompletion;
	DynamicFct<DWORD (__stdcall*)(HANDLE*, DWORD, OpenStruct*)> NtOpenSemaphore;
	DynamicFct<DWORD (__stdcall*)(HANDLE*, DWORD, OpenStruct*)> NtOpenTimer;
	DynamicFct<DWORD (__stdcall*)(HANDLE*, DWORD, OpenStruct*)> NtOpenKey;
	DynamicFct<DWORD (__stdcall*)(HANDLE)> NtClose;
	DynamicFct<DWORD (__stdcall*)(HANDLE*, DWORD, OpenStruct*)> NtOpenProcess;
	DynamicFct<DWORD (__stdcall*)(HANDLE*, DWORD, OpenStruct*)> NtOpenThread;
};

const LPCWSTR NTDLL::s_ObjectTypes[] = {
	L"Directory", L"SymbolicLink",
	L"Mutant", L"Section", L"Event", L"Semaphore",
	L"Timer", L"Key", L"EventPair", L"IoCompletion",
	L"Device", L"File", L"Controller", L"Profile",
	L"Type", L"Desktop", L"WindowStatiom", L"Driver",
	L"Token", L"Process", L"Thread", L"Adapter", L"Port",
	0
};

NTDLL* g_NTDLL = NULL;


struct UnicodeString : public RtlUnicodeString {
	UnicodeString(LPCWSTR str)
	{
		(*g_NTDLL->RtlInitUnicodeString)(this, str);
	}

	UnicodeString(size_t len, LPWSTR buffer)
	{
		alloc_len = len;
		string_ptr = buffer;
	}

	operator LPCWSTR() const {return string_ptr;}
};


static DWORD NtOpenObject(OBJECT_TYPE type, HANDLE* phandle, DWORD flags, LPCWSTR path/*, BOOL xflag=FALSE*/)
{
	UnicodeString ustr(path);
	OpenStruct open_struct = {sizeof(OpenStruct), 0x00, &ustr, 0x40};

	if (type==SYMBOLICLINK_OBJECT || type==DIRECTORY_OBJECT)
		flags |= 1;	//@@ ENUMERATE

	/* if (xflag)
		flags |= 0x80000000; */

	DWORD retx;

	if (type>=DIRECTORY_OBJECT && type<=IOCOMPLETITION_OBJECT)
		return g_NTDLL->_ObjectOpenFunctions[type](phandle, flags|0x20000, &open_struct);
	else if (type == FILE_OBJECT)
		return (*g_NTDLL->NtOpenFile)(phandle, flags, &open_struct, &retx, 7, 0);
	else
		return ERROR_INVALID_FUNCTION;
}


void NtObjDirectory::read_directory(int scan_flags)
{
	CONTEXT("NtObjDirectory::read_directory()");

	if (!g_NTDLL)
		g_NTDLL = new NTDLL();

	Entry* first_entry = NULL;
	int level = _level + 1;

	LPCTSTR path = (LPCTSTR)_path;

	TCHAR buffer[MAX_PATH], *p=buffer;
#ifndef UNICODE
	WCHAR wbuffer[MAX_PATH], *w=wbuffer;
#endif

	do {
		*p++ = *path;
#ifndef UNICODE
		*w++ = *path;
#endif
	} while(*path++);
	--p;
#ifndef UNICODE
	--w;
#endif

	DWORD idx1, idx2;
	HANDLE dir_handle;

#ifdef UNICODE
	if (NtOpenObject(_type, &dir_handle, 0, buffer))
#else
	if (NtOpenObject(_type, &dir_handle, 0, wbuffer))
#endif
		return;

#ifdef UNICODE
	if (p[-1] != '\\')
		*p++ = '\\';
#else
	if (w[-1] != '\\')
		*w++ = '\\';
#endif

	if (_type == DIRECTORY_OBJECT) {
		NtObjectInfo* info = (NtObjectInfo*)alloca(0x800);

		if (!(*g_NTDLL->NtQueryDirectoryObject)(dir_handle, info, 0x800, TRUE, TRUE, &idx1, &idx2)) {
			WIN32_FIND_DATA w32fd;
			Entry* last = NULL;
			Entry* entry;

			do {
				memset(&w32fd, 0, sizeof(WIN32_FIND_DATA));

#ifdef UNICODE
				wcscpyn(p, info->name.string_ptr, _MAX_PATH);
				//@@wcscpyn(_class, info->type.string_ptr, 32);
#else
				WideCharToMultiByte(CP_ACP, 0, info->name.string_ptr, info->name.string_len, p, MAX_PATH, 0, 0);
				//@@U2T(info->type.string_ptr, _class, 32);
#endif

				lstrcpy(w32fd.cFileName, p);

				const LPCWSTR* tname = NTDLL::s_ObjectTypes;
				OBJECT_TYPE type = UNKNOWN_OBJECT_TYPE;

				for(; *tname; tname++)
					if (!wcsncmp(info->type.string_ptr, *tname, 32))
						{type=OBJECT_TYPE(tname-NTDLL::s_ObjectTypes); break;}

				if (type == DIRECTORY_OBJECT) {
					w32fd.dwFileAttributes |= FILE_ATTRIBUTE_DIRECTORY;

					entry = new NtObjDirectory(this, buffer);
				}

				else if (type == SYMBOLICLINK_OBJECT) {
					w32fd.dwFileAttributes |= ATTRIBUTE_SYMBOLIC_LINK;

					//@@_toscan |= INF_DESCRIPTION;

					entry = new NtObjDirectory(this, buffer);

					if (*w32fd.cFileName>='A' &&*w32fd.cFileName<='Z' && w32fd.cFileName[1]==':') {
						if (!_tcsncmp(buffer,TEXT("\\??\\"),4) || !_tcsncmp(buffer,TEXT("\\GLOBAL??\\"),10)) {

							///@todo mount drive at this entry

						}
					}
				}

				else if (type == KEY_OBJECT) {
					w32fd.dwFileAttributes |= FILE_ATTRIBUTE_DIRECTORY;

					entry = new RegistryRoot(this, buffer);
				}
				else
					entry = new NtObjEntry(this, type);

				HANDLE handle;

#ifdef UNICODE
				lstrcpyW(p, info->name.string_ptr);
				if (!NtOpenObject(_type, &handle, 0, buffer))
#else
				lstrcpyW(w, info->name.string_ptr);
				if (!NtOpenObject(_type, &handle, 0, wbuffer))
#endif
				{
					NtObject object;
					DWORD read;

					if (!(*g_NTDLL->NtQueryObject)(handle, 0, &object, sizeof(NtObject), &read)) {
						memcpy(&w32fd.ftCreationTime, &object.creation_time, sizeof(FILETIME));

						memset(&entry->_bhfi, 0, sizeof(BY_HANDLE_FILE_INFORMATION));
						entry->_bhfi.nNumberOfLinks = object.reference_count - 1;
						entry->_bhfi_valid = true;
					}

					(*g_NTDLL->NtClose)(handle);
				}

				memcpy(&entry->_data, &w32fd, sizeof(WIN32_FIND_DATA));

				if (!first_entry)
					first_entry = entry;

				if (last)
					last->_next = entry;

				entry->_down = NULL;
				entry->_expanded = false;
				entry->_scanned = false;
				entry->_level = level;

				last = entry;
			} while(!(*g_NTDLL->NtQueryDirectoryObject)(dir_handle, info, 0x800, TRUE, FALSE, &idx1, &idx2));

			last->_next = NULL;
		}
	} else {
/*@@
		assert(_type==SYMBOLICLINK_OBJECT);

		try {
			wcscpy(wcpcpy(buffer, UNC(_name)), L"\\");

			if (GetFileAttributesW(buffer) != 0xFFFFFFFF) {
#ifdef UNICODE
			TLVEntry* entry = new FileSysEntry(system()->tlvctrlr(), buffer);
#else
			TLVEntry* entry = new FileSysEntry(system()->tlvctrlr(), String(buffer));
#endif

			*entry->pparent() = this;
			*pchild() = entry;

			expand();
		} else
			_toscan |= INF_REFRESH_SCAN_CHILD;
		} catch(Exception& e) {
			if (display_errors)
				HandleException(e);
			}
*/
	}

/*@@
	if (_toscan & INF_DESCRIPTION) {
		_toscan &= ~INF_DESCRIPTION;

		if (_type == SYMBOLICLINK_OBJECT) {
			DWORD len;
			WCHAR wbuffer[_MAX_PATH];
			UnicodeString link(_MAX_PATH, wbuffer);

			if (!NtQuerySymbolicLinkObject(dir_handle, &link, &len)) {
#ifdef UNICODE
				wcscpy(_description, link);
#else
				U2T(link, _description, -1);
#endif
				_inf |= INF_DESCRIPTION;
			}
		}
	}
*/

	(*g_NTDLL->NtClose)(dir_handle);

	_down = first_entry;
	_scanned = true;
}


Entry* NtObjDirectory::find_entry(const void* p)
{
	LPCTSTR name = (LPCTSTR)p;

	for(Entry*entry=_down; entry; entry=entry->_next) {
		LPCTSTR p = name;
		LPCTSTR q = entry->_data.cFileName;

		do {
			if (!*p || *p==TEXT('\\') || *p==TEXT('/'))
				return entry;
		} while(tolower(*p++) == tolower(*q++));

		p = name;
		q = entry->_data.cAlternateFileName;

		do {
			if (!*p || *p==TEXT('\\') || *p==TEXT('/'))
				return entry;
		} while(tolower(*p++) == tolower(*q++));
	}

	return NULL;
}


 // get full path of specified directory entry
bool NtObjEntry::get_path(PTSTR path) const
{
	int level = 0;
	int len = 0;
	int l = 0;
	LPCTSTR name = NULL;
	TCHAR buffer[MAX_PATH];

	const Entry* entry;
	for(entry=this; entry; level++) {
		l = 0;

		if (entry->_etype == ET_NTOBJS) {
			name = entry->_data.cFileName;

			for(LPCTSTR s=name; *s && *s!=TEXT('/') && *s!=TEXT('\\'); s++)
				++l;

			if (!entry->_up)
				break;
		} else {
			if (entry->get_path(buffer)) {
				l = _tcslen(buffer);
				name = buffer;

				/* special handling of drive names */
				if (l>0 && buffer[l-1]=='\\' && path[0]=='\\')
					--l;

				memmove(path+l, path, len*sizeof(TCHAR));
				memcpy(path, name, l*sizeof(TCHAR));
				len += l;
			}

			entry = NULL;
			break;
		}

		if (l > 0) {
			memmove(path+l+1, path, len*sizeof(TCHAR));
			memcpy(path+1, name, l*sizeof(TCHAR));
			len += l+1;

			path[0] = TEXT('\\');
		}

		entry = entry->_up;
	}

	if (entry) {
		memmove(path+l, path, len*sizeof(TCHAR));
		memcpy(path, name, l*sizeof(TCHAR));
		len += l;
	}

	if (!level)
		path[len++] = TEXT('\\');

	path[len] = TEXT('\0');

	return true;
}

BOOL NtObjEntry::launch_entry(HWND hwnd, UINT nCmdShow)
{
	return FALSE;
}
