/*
 * tests for comcat functions
 *
 * Copyright 2006 Aric Stewart for CodeWeavers
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

#define COBJMACROS

#include <stdio.h>
#include <windows.h>

#include "objbase.h"
#include "comcat.h"

#include "wine/test.h"

#define ok_ole_success(hr, func) ok(hr == S_OK, func " failed with error 0x%08x \n", hr)

static void register_testentry(void)
{
	HKEY hkey,hkey2;

	RegCreateKeyA(HKEY_CLASSES_ROOT,"CLSID\\{deadcafe-beed-bead-dead-cafebeaddead}",
			&hkey);
	RegSetValueA(hkey,NULL,REG_SZ,"ComCat Test key",16);
	RegCreateKeyA(hkey,
			"Implemented Categories\\{deadcafe-0000-0000-0000-000000000000}",
			&hkey2);

	RegCloseKey(hkey);
	RegCloseKey(hkey2);
}

static void unregister_testentry(void)
{
	RegDeleteKeyA(HKEY_CLASSES_ROOT,
			"CLSID\\{deadcafe-beed-bead-dead-cafebeaddead}\\Implemented Categories\\{deadcafe-0000-0000-0000-000000000000}");
	RegDeleteKeyA(HKEY_CLASSES_ROOT,
			"CLSID\\{deadcafe-beed-bead-dead-cafebeaddead}\\Implemented Categories");
	RegDeleteKeyA(HKEY_CLASSES_ROOT,
			"CLSID\\{deadcafe-beed-bead-dead-cafebeaddead}");
}

static void do_enum(void)
{
	HRESULT hr;
	REFCLSID rclsid = &CLSID_StdComponentCategoriesMgr;
	ICatInformation *pICat = (ICatInformation*)0xdeadbeef;
	GUID the_guid[1];
	GUID the_cat[1];
	GUID wanted_guid;
	ULONG fetched = -1;
	
	static WCHAR szCatID[] = {
			'{',
			'd','e','a','d','c','a','f','e',
			'-','0','0','0','0','-','0','0','0','0',
			'-','0','0','0','0',
			'-','0','0','0','0','0','0','0','0','0','0','0','0',
			'}',0};
	static WCHAR szGuid[] = {
			'{',
			'd','e','a','d','c','a','f','e','-',
			'b','e','e','d','-',
			'b','e','a','d','-',
			'd','e','a','d','-',
			'c','a','f','e','b','e','a','d','d','e','a','d',
			'}',0};

	IEnumCLSID *pIEnum =(IEnumCLSID*)0xdeadcafe;

	CLSIDFromString((LPOLESTR)szCatID,the_cat);
	CLSIDFromString((LPOLESTR)szGuid,&wanted_guid);
	
	OleInitialize(NULL);

	hr = CoCreateInstance(rclsid,NULL,CLSCTX_INPROC_SERVER,
			&IID_ICatInformation, (void **)&pICat);
	ok_ole_success(hr, "CoCreateInstance");

	hr = ICatInformation_EnumClassesOfCategories(pICat, -1, NULL, -1, NULL,
			&pIEnum);
	ok_ole_success(hr,"ICatInformation_EnumClassesOfCategories");

	IEnumGUID_Release(pIEnum);
	
	hr = ICatInformation_EnumClassesOfCategories(pICat, 1, the_cat, -1, NULL, 
			&pIEnum);
	ok_ole_success(hr,"ICatInformation_EnumClassesOfCategories");

	hr = IEnumGUID_Next(pIEnum,1,the_guid, &fetched);
	ok (fetched == 0,"Fetched wrong number of guids %u\n",fetched);
	IEnumGUID_Release(pIEnum);
	
	register_testentry();
	hr = ICatInformation_EnumClassesOfCategories(pICat, 1, the_cat, -1, NULL, 
			&pIEnum);
	ok_ole_success(hr,"ICatInformation_EnumClassesOfCategories");

	hr = IEnumGUID_Next(pIEnum,1,the_guid, &fetched);
	ok (fetched == 1,"Fetched wrong number of guids %u\n",fetched);
	ok (IsEqualGUID(the_guid,&wanted_guid),"Guids do not match\n");

	IEnumGUID_Release(pIEnum);
	ICatInformation_Release(pICat);
	unregister_testentry();

	OleUninitialize();
}


START_TEST(comcat)
{
 	do_enum(); 
}
