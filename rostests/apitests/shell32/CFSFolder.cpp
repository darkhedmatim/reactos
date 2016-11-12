/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for CMyComputer
 * PROGRAMMER:      Giannis Adamopoulos
 */

#include "shelltest.h"
#include <atlbase.h>
#include <atlcom.h>
#include <strsafe.h>

#define NDEBUG
#include <debug.h>
#include <shellutils.h>

VOID TestUninitialized()
{
    CComPtr<IShellFolder> psf;
    CComPtr<IEnumIDList> penum;
    CComPtr<IDropTarget> pdt;
    CComPtr<IContextMenu> pcm;
    CComPtr<IShellView> psv;
    LPITEMIDLIST retrievedPidl;
    ULONG pceltFetched;
    HRESULT hr;

    /* Create a CFSFolder */
    hr = CoCreateInstance(CLSID_ShellFSFolder, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IShellFolder, &psf));
    ok(hr == S_OK, "hr = %lx\n", hr);

    /* An uninitialized CFSFolder doesn't contain any items */
    hr = psf->EnumObjects(NULL, 0, &penum);
    ok(hr == S_OK, "hr = %lx\n", hr);
    hr = penum->Next(0, &retrievedPidl, &pceltFetched);
    ok(hr == S_FALSE, "hr = %lx\n", hr);
    hr = penum->Next(1, &retrievedPidl, &pceltFetched);
    ok(hr == S_FALSE, "hr = %lx\n", hr);

    /* It supports viewing */
    hr = psf->CreateViewObject(NULL, IID_PPV_ARG(IDropTarget, &pdt));
    ok(hr == S_OK, "hr = %lx\n", hr);
    hr = psf->CreateViewObject(NULL, IID_PPV_ARG(IContextMenu, &pcm));
    ok(hr == S_OK, "hr = %lx\n", hr);
    hr = psf->CreateViewObject(NULL, IID_PPV_ARG(IShellView, &psv));
    ok(hr == S_OK, "hr = %lx\n", hr);

    /* And its display name is ... "C:\Documents and Settings\" */
    STRRET strretName;
    hr = psf->GetDisplayNameOf(NULL,SHGDN_FORPARSING,&strretName);
    ok(hr == S_OK, "hr = %lx\n", hr);
    ok(strretName.uType == STRRET_WSTR, "strretName.uType == %x\n", strretName.uType);
    ok(wcscmp(strretName.pOleStr, L"C:\\Documents and Settings\\") == 0, "wrong name, got: %S\n", strretName.pOleStr);

    hr = psf->GetDisplayNameOf(NULL,SHGDN_FORPARSING|SHGDN_INFOLDER,&strretName);
    ok(hr == E_INVALIDARG, "hr = %lx\n", hr);
    
    
    
    
    /* Use Initialize method with  a dummy pidl and test the still non initialized CFSFolder */
    CComPtr<IPersistFolder2> ppf2;
    hr = psf->QueryInterface(IID_PPV_ARG(IPersistFolder2, &ppf2));
    ok(hr == S_OK, "hr = %lx\n", hr);

    /* Create a tiny pidl with no contents */
    LPITEMIDLIST testpidl = (LPITEMIDLIST)SHAlloc(3 * sizeof(WORD));
    testpidl->mkid.cb = 2 * sizeof(WORD);
    *(WORD*)((char*)testpidl + (int)(2 * sizeof(WORD))) = 0;

    hr = ppf2->Initialize(testpidl);
    ok(hr == S_OK, "hr = %lx\n", hr);
    
    LPITEMIDLIST pidl;
    hr = ppf2->GetCurFolder(&pidl);
    ok(hr == S_OK, "hr = %lx\n", hr);
    ok(pidl->mkid.cb == 2 * sizeof(WORD), "got wrong pidl size, cb = %x\n", pidl->mkid.cb);

    /* methods that worked before, now fail */
    hr = psf->GetDisplayNameOf(NULL,SHGDN_FORPARSING,&strretName);
    ok(hr == E_FAIL, "hr = %lx\n", hr);
    hr = psf->EnumObjects(NULL, 0, &penum);
    ok(hr == HRESULT_FROM_WIN32(ERROR_CANCELLED), "hr = %lx\n", hr);
    
    /* The following continue to work though */
    hr = psf->CreateViewObject(NULL, IID_PPV_ARG(IDropTarget, &pdt));
    ok(hr == S_OK, "hr = %lx\n", hr);
    hr = psf->CreateViewObject(NULL, IID_PPV_ARG(IContextMenu, &pcm));
    ok(hr == S_OK, "hr = %lx\n", hr);
    hr = psf->CreateViewObject(NULL, IID_PPV_ARG(IShellView, &psv));
    ok(hr == S_OK, "hr = %lx\n", hr);

}

START_TEST(CFSFolder)
{  
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    TestUninitialized();
}