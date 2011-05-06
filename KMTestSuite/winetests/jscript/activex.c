/*
 * Copyright 2009 Jacek Caban for CodeWeavers
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

#include <stdio.h>

#define COBJMACROS
#define CONST_VTABLE

#include <ole2.h>
#include <dispex.h>
#include <activscp.h>
#include <objsafe.h>
#include <urlmon.h>
#include <mshtmhst.h>

#include "wine/test.h"

static const CLSID CLSID_JScript =
    {0xf414c260,0x6ac0,0x11cf,{0xb6,0xd1,0x00,0xaa,0x00,0xbb,0xbb,0x58}};

#define DEFINE_EXPECT(func) \
    static BOOL expect_ ## func = FALSE, called_ ## func = FALSE

#define SET_EXPECT(func) \
    expect_ ## func = TRUE

#define SET_CALLED(func) \
    called_ ## func = TRUE

#define CHECK_EXPECT2(func) \
    do { \
        ok(expect_ ##func, "unexpected call " #func "\n"); \
        called_ ## func = TRUE; \
    }while(0)

#define CHECK_EXPECT(func) \
    do { \
        CHECK_EXPECT2(func); \
        expect_ ## func = FALSE; \
    }while(0)

#define CHECK_CALLED(func) \
    do { \
        ok(called_ ## func, "expected " #func "\n"); \
        expect_ ## func = called_ ## func = FALSE; \
    }while(0)

DEFINE_EXPECT(CreateInstance);
DEFINE_EXPECT(ProcessUrlAction);
DEFINE_EXPECT(QueryCustomPolicy);
DEFINE_EXPECT(reportSuccess);
DEFINE_EXPECT(Host_QS_SecMgr);
DEFINE_EXPECT(Caller_QS_SecMgr);
DEFINE_EXPECT(QI_IObjectWithSite);
DEFINE_EXPECT(SetSite);

static const WCHAR testW[] = {'t','e','s','t',0};

static HRESULT QS_SecMgr_hres;
static HRESULT ProcessUrlAction_hres;
static DWORD ProcessUrlAction_policy;
static HRESULT CreateInstance_hres;
static HRESULT QueryCustomPolicy_hres;
static DWORD QueryCustomPolicy_psize;
static DWORD QueryCustomPolicy_policy;
static HRESULT QI_IDispatch_hres;
static HRESULT SetSite_hres;

#define TESTOBJ_CLSID "{178fc163-f585-4e24-9c13-4bb7faf80646}"

static const GUID CLSID_TestObj =
    {0x178fc163,0xf585,0x4e24,{0x9c,0x13,0x4b,0xb7,0xfa,0xf8,0x06,0x46}};

/* Defined as extern in urlmon.idl, but not exported by uuid.lib */
const GUID GUID_CUSTOM_CONFIRMOBJECTSAFETY =
    {0x10200490,0xfa38,0x11d0,{0xac,0x0e,0x00,0xa0,0xc9,0xf,0xff,0xc0}};

#define DISPID_TEST_REPORTSUCCESS    0x1000

#define DISPID_GLOBAL_OK             0x2000

static const char *debugstr_guid(REFIID riid)
{
    static char buf[50];

    sprintf(buf, "{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
            riid->Data1, riid->Data2, riid->Data3, riid->Data4[0],
            riid->Data4[1], riid->Data4[2], riid->Data4[3], riid->Data4[4],
            riid->Data4[5], riid->Data4[6], riid->Data4[7]);

    return buf;
}

static BSTR a2bstr(const char *str)
{
    BSTR ret;
    int len;

    len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
    ret = SysAllocStringLen(NULL, len-1);
    MultiByteToWideChar(CP_ACP, 0, str, -1, ret, len);

    return ret;
}

static int strcmp_wa(LPCWSTR strw, const char *stra)
{
    CHAR buf[512];
    WideCharToMultiByte(CP_ACP, 0, strw, -1, buf, sizeof(buf), 0, 0);
    return lstrcmpA(buf, stra);
}

static HRESULT WINAPI ObjectWithSite_QueryInterface(IObjectWithSite *iface, REFIID riid, void **ppv)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static ULONG WINAPI ObjectWithSite_AddRef(IObjectWithSite *iface)
{
    return 2;
}

static ULONG WINAPI ObjectWithSite_Release(IObjectWithSite *iface)
{
    return 1;
}

static HRESULT WINAPI ObjectWithSite_SetSite(IObjectWithSite *iface, IUnknown *pUnkSite)
{
    IServiceProvider *sp;
    HRESULT hres;


    CHECK_EXPECT(SetSite);
    ok(pUnkSite != NULL, "pUnkSite == NULL\n");

    hres = IUnknown_QueryInterface(pUnkSite, &IID_IServiceProvider, (void**)&sp);
    ok(hres == S_OK, "Could not get IServiceProvider iface: %08x\n", hres);
    IServiceProvider_Release(sp);

    return SetSite_hres;
}

static HRESULT WINAPI ObjectWithSite_GetSite(IObjectWithSite *iface, REFIID riid, void **ppvSite)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IObjectWithSiteVtbl ObjectWithSiteVtbl = {
    ObjectWithSite_QueryInterface,
    ObjectWithSite_AddRef,
    ObjectWithSite_Release,
    ObjectWithSite_SetSite,
    ObjectWithSite_GetSite
};

static IObjectWithSite ObjectWithSite = { &ObjectWithSiteVtbl };

static IObjectWithSite *object_with_site;

static HRESULT WINAPI DispatchEx_QueryInterface(IDispatchEx *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if(IsEqualGUID(riid, &IID_IUnknown)) {
       *ppv = iface;
    }else if(IsEqualGUID(riid, &IID_IDispatch) || IsEqualGUID(riid, &IID_IDispatchEx)) {
        if(FAILED(QI_IDispatch_hres))
            return QI_IDispatch_hres;
        *ppv = iface;
    }else if(IsEqualGUID(&IID_IObjectWithSite, riid)) {
        CHECK_EXPECT(QI_IObjectWithSite);
        *ppv = object_with_site;
    }else {
        return E_NOINTERFACE;
    }

    return *ppv ? S_OK : E_NOINTERFACE;
}

static ULONG WINAPI DispatchEx_AddRef(IDispatchEx *iface)
{
    return 2;
}

static ULONG WINAPI DispatchEx_Release(IDispatchEx *iface)
{
    return 1;
}

static HRESULT WINAPI DispatchEx_GetTypeInfoCount(IDispatchEx *iface, UINT *pctinfo)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_GetTypeInfo(IDispatchEx *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_GetIDsOfNames(IDispatchEx *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_Invoke(IDispatchEx *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_DeleteMemberByName(IDispatchEx *iface, BSTR bstrName, DWORD grfdex)
{
    ok(0, "unexpected call %s %x\n", wine_dbgstr_w(bstrName), grfdex);
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_DeleteMemberByDispID(IDispatchEx *iface, DISPID id)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_GetMemberProperties(IDispatchEx *iface, DISPID id, DWORD grfdexFetch, DWORD *pgrfdex)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_GetMemberName(IDispatchEx *iface, DISPID id, BSTR *pbstrName)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_GetNextDispID(IDispatchEx *iface, DWORD grfdex, DISPID id, DISPID *pid)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_GetNameSpaceParent(IDispatchEx *iface, IUnknown **ppunk)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Test_GetDispID(IDispatchEx *iface, BSTR bstrName, DWORD grfdex, DISPID *pid)
{
    if(!strcmp_wa(bstrName, "reportSuccess")) {
        ok(grfdex == fdexNameCaseSensitive, "grfdex = %x\n", grfdex);
        *pid = DISPID_TEST_REPORTSUCCESS;
        return S_OK;
    }

    ok(0, "unexpected name %s\n", wine_dbgstr_w(bstrName));
    return E_NOTIMPL;
}

static HRESULT WINAPI Test_InvokeEx(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    switch(id) {
    case DISPID_TEST_REPORTSUCCESS:
        CHECK_EXPECT(reportSuccess);

        ok(wFlags == INVOKE_FUNC, "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(pdp->cArgs == 0, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(!pvarRes, "pvarRes != NULL\n");
        ok(pei != NULL, "pei == NULL\n");
        break;

    default:
        ok(0, "unexpected call\n");
        return E_NOTIMPL;
    }

    return S_OK;
}

static IDispatchExVtbl testObjVtbl = {
    DispatchEx_QueryInterface,
    DispatchEx_AddRef,
    DispatchEx_Release,
    DispatchEx_GetTypeInfoCount,
    DispatchEx_GetTypeInfo,
    DispatchEx_GetIDsOfNames,
    DispatchEx_Invoke,
    Test_GetDispID,
    Test_InvokeEx,
    DispatchEx_DeleteMemberByName,
    DispatchEx_DeleteMemberByDispID,
    DispatchEx_GetMemberProperties,
    DispatchEx_GetMemberName,
    DispatchEx_GetNextDispID,
    DispatchEx_GetNameSpaceParent
};

static IDispatchEx testObj = { &testObjVtbl };

static HRESULT WINAPI Global_GetDispID(IDispatchEx *iface, BSTR bstrName, DWORD grfdex, DISPID *pid)
{
    if(!strcmp_wa(bstrName, "ok")) {
        ok(grfdex == fdexNameCaseSensitive, "grfdex = %x\n", grfdex);
        *pid = DISPID_GLOBAL_OK;
        return S_OK;
    }

    ok(0, "unexpected name %s\n", wine_dbgstr_w(bstrName));
    return E_NOTIMPL;
}

static HRESULT WINAPI Global_InvokeEx(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    switch(id) {
    case DISPID_GLOBAL_OK:
        ok(wFlags == INVOKE_FUNC || wFlags == (INVOKE_FUNC|INVOKE_PROPERTYGET), "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(pdp->rgvarg != NULL, "rgvarg == NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(pdp->cArgs == 2, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(pei != NULL, "pei == NULL\n");

        ok(V_VT(pdp->rgvarg) == VT_BSTR, "V_VT(psp->rgvargs) = %d\n", V_VT(pdp->rgvarg));
        ok(V_VT(pdp->rgvarg+1) == VT_BOOL, "V_VT(psp->rgvargs+1) = %d\n", V_VT(pdp->rgvarg));
        ok(V_BOOL(pdp->rgvarg+1), "%s\n", wine_dbgstr_w(V_BSTR(pdp->rgvarg)));
        break;

    default:
        ok(0, "unexpected call\n");
        return E_NOTIMPL;
    }

    return S_OK;
}

static IDispatchExVtbl globalObjVtbl = {
    DispatchEx_QueryInterface,
    DispatchEx_AddRef,
    DispatchEx_Release,
    DispatchEx_GetTypeInfoCount,
    DispatchEx_GetTypeInfo,
    DispatchEx_GetIDsOfNames,
    DispatchEx_Invoke,
    Global_GetDispID,
    Global_InvokeEx,
    DispatchEx_DeleteMemberByName,
    DispatchEx_DeleteMemberByDispID,
    DispatchEx_GetMemberProperties,
    DispatchEx_GetMemberName,
    DispatchEx_GetNextDispID,
    DispatchEx_GetNameSpaceParent
};

static IDispatchEx globalObj = { &globalObjVtbl };

static HRESULT WINAPI ClassFactory_QueryInterface(IClassFactory *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IClassFactory, riid)) {
        *ppv = iface;
        return S_OK;
    }

    /* TODO: IClassFactoryEx */
    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI ClassFactory_AddRef(IClassFactory *iface)
{
    return 2;
}

static ULONG WINAPI ClassFactory_Release(IClassFactory *iface)
{
    return 1;
}

static HRESULT WINAPI ClassFactory_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID riid, void **ppv)
{
    CHECK_EXPECT(CreateInstance);

    ok(!outer, "outer = %p\n", outer);
    ok(IsEqualGUID(&IID_IUnknown, riid), "unexpected riid %s\n", debugstr_guid(riid));

    if(SUCCEEDED(CreateInstance_hres))
        *ppv = &testObj;
    return CreateInstance_hres;
}

static HRESULT WINAPI ClassFactory_LockServer(IClassFactory *iface, BOOL dolock)
{
    ok(0, "unexpected call\n");
    return S_OK;
}

static const IClassFactoryVtbl ClassFactoryVtbl = {
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    ClassFactory_CreateInstance,
    ClassFactory_LockServer
};

static IClassFactory activex_cf = { &ClassFactoryVtbl };

static HRESULT WINAPI InternetHostSecurityManager_QueryInterface(IInternetHostSecurityManager *iface, REFIID riid, void **ppv)
{
    ok(0, "unexpected call\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI InternetHostSecurityManager_AddRef(IInternetHostSecurityManager *iface)
{
    return 2;
}

static ULONG WINAPI InternetHostSecurityManager_Release(IInternetHostSecurityManager *iface)
{
    return 1;
}

static HRESULT WINAPI InternetHostSecurityManager_GetSecurityId(IInternetHostSecurityManager *iface,  BYTE *pbSecurityId,
        DWORD *pcbSecurityId, DWORD_PTR dwReserved)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetHostSecurityManager_ProcessUrlAction(IInternetHostSecurityManager *iface, DWORD dwAction,
        BYTE *pPolicy, DWORD cbPolicy, BYTE *pContext, DWORD cbContext, DWORD dwFlags, DWORD dwReserved)
{
    CHECK_EXPECT(ProcessUrlAction);

    ok(dwAction == URLACTION_ACTIVEX_RUN, "dwAction = %x\n", dwAction);
    ok(pPolicy != NULL, "pPolicy == NULL\n");
    ok(cbPolicy == sizeof(DWORD), "cbPolicy = %d\n", cbPolicy);
    ok(pContext != NULL, "pContext == NULL\n");
    ok(cbContext == sizeof(GUID), "cbContext = %d\n", cbContext);
    ok(IsEqualGUID(pContext, &CLSID_TestObj), "pContext = %s\n", debugstr_guid((const IID*)pContext));
    ok(!dwFlags, "dwFlags = %x\n", dwFlags);
    ok(!dwReserved, "dwReserved = %x\n", dwReserved);

    if(SUCCEEDED(ProcessUrlAction_hres))
        *(DWORD*)pPolicy = ProcessUrlAction_policy;
    return ProcessUrlAction_hres;
}

static HRESULT WINAPI InternetHostSecurityManager_QueryCustomPolicy(IInternetHostSecurityManager *iface, REFGUID guidKey,
        BYTE **ppPolicy, DWORD *pcbPolicy, BYTE *pContext, DWORD cbContext, DWORD dwReserved)
{
    const struct CONFIRMSAFETY *cs = (const struct CONFIRMSAFETY*)pContext;
    DWORD *ret;

    CHECK_EXPECT(QueryCustomPolicy);

    ok(IsEqualGUID(&GUID_CUSTOM_CONFIRMOBJECTSAFETY, guidKey), "guidKey = %s\n", debugstr_guid(guidKey));

    ok(ppPolicy != NULL, "ppPolicy == NULL\n");
    ok(pcbPolicy != NULL, "pcbPolicy == NULL\n");
    ok(pContext != NULL, "pContext == NULL\n");
    ok(cbContext == sizeof(struct CONFIRMSAFETY), "cbContext = %d\n", cbContext);
    ok(!dwReserved, "dwReserved = %x\n", dwReserved);

    /* TODO: CLSID */
    ok(cs->pUnk != NULL, "cs->pUnk == NULL\n");
    ok(!cs->dwFlags, "dwFlags = %x\n", cs->dwFlags);

    if(FAILED(QueryCustomPolicy_hres))
        return QueryCustomPolicy_hres;

    ret = CoTaskMemAlloc(QueryCustomPolicy_psize);
    *ppPolicy = (BYTE*)ret;
    *pcbPolicy = QueryCustomPolicy_psize;
    memset(ret, 0, QueryCustomPolicy_psize);
    if(QueryCustomPolicy_psize >= sizeof(DWORD))
        *ret = QueryCustomPolicy_policy;

    return QueryCustomPolicy_hres;
}

static const IInternetHostSecurityManagerVtbl InternetHostSecurityManagerVtbl = {
    InternetHostSecurityManager_QueryInterface,
    InternetHostSecurityManager_AddRef,
    InternetHostSecurityManager_Release,
    InternetHostSecurityManager_GetSecurityId,
    InternetHostSecurityManager_ProcessUrlAction,
    InternetHostSecurityManager_QueryCustomPolicy
};

static IInternetHostSecurityManager InternetHostSecurityManager = { &InternetHostSecurityManagerVtbl };

static IServiceProvider ServiceProvider;

static HRESULT WINAPI ServiceProvider_QueryInterface(IServiceProvider *iface, REFIID riid, void **ppv)
{
    ok(0, "unexpected call\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI ServiceProvider_AddRef(IServiceProvider *iface)
{
    return 2;
}

static ULONG WINAPI ServiceProvider_Release(IServiceProvider *iface)
{
    return 1;
}

static HRESULT WINAPI ServiceProvider_QueryService(IServiceProvider *iface,
        REFGUID guidService, REFIID riid, void **ppv)
{
    if(IsEqualGUID(&SID_GetCaller, guidService))
        return E_NOINTERFACE;

    if(IsEqualGUID(&SID_SInternetHostSecurityManager, guidService)) {
        if(iface == &ServiceProvider)
            CHECK_EXPECT(Host_QS_SecMgr);
        else
            CHECK_EXPECT(Caller_QS_SecMgr);
        ok(IsEqualGUID(&IID_IInternetHostSecurityManager, riid), "unexpected riid %s\n", debugstr_guid(riid));
        if(SUCCEEDED(QS_SecMgr_hres))
            *ppv = &InternetHostSecurityManager;
        return QS_SecMgr_hres;
    }

    ok(0, "unexpected service %s\n", debugstr_guid(guidService));
    return E_NOINTERFACE;
}

static IServiceProviderVtbl ServiceProviderVtbl = {
    ServiceProvider_QueryInterface,
    ServiceProvider_AddRef,
    ServiceProvider_Release,
    ServiceProvider_QueryService
};

static IServiceProvider ServiceProvider = { &ServiceProviderVtbl };
static IServiceProvider caller_sp = { &ServiceProviderVtbl };

static HRESULT WINAPI ActiveScriptSite_QueryInterface(IActiveScriptSite *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(&IID_IUnknown, riid)) {
        *ppv = iface;
    }else if(IsEqualGUID(&IID_IActiveScriptSite, riid)) {
        *ppv = iface;
    }else if(IsEqualGUID(&IID_IServiceProvider, riid)) {
        *ppv = &ServiceProvider;
    }else {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI ActiveScriptSite_AddRef(IActiveScriptSite *iface)
{
    return 2;
}

static ULONG WINAPI ActiveScriptSite_Release(IActiveScriptSite *iface)
{
    return 1;
}

static HRESULT WINAPI ActiveScriptSite_GetLCID(IActiveScriptSite *iface, LCID *plcid)
{
    *plcid = GetUserDefaultLCID();
    return S_OK;
}

static HRESULT WINAPI ActiveScriptSite_GetItemInfo(IActiveScriptSite *iface, LPCOLESTR pstrName,
        DWORD dwReturnMask, IUnknown **ppiunkItem, ITypeInfo **ppti)
{
    ok(dwReturnMask == SCRIPTINFO_IUNKNOWN, "unexpected dwReturnMask %x\n", dwReturnMask);
    ok(!ppti, "ppti != NULL\n");
    ok(!strcmp_wa(pstrName, "test"), "pstrName = %s\n", wine_dbgstr_w(pstrName));

    *ppiunkItem = (IUnknown*)&globalObj;
    return S_OK;
}

static HRESULT WINAPI ActiveScriptSite_GetDocVersionString(IActiveScriptSite *iface, BSTR *pbstrVersion)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScriptSite_OnScriptTerminate(IActiveScriptSite *iface,
        const VARIANT *pvarResult, const EXCEPINFO *pexcepinfo)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScriptSite_OnStateChange(IActiveScriptSite *iface, SCRIPTSTATE ssScriptState)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScriptSite_OnScriptError(IActiveScriptSite *iface, IActiveScriptError *pscripterror)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScriptSite_OnEnterScript(IActiveScriptSite *iface)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScriptSite_OnLeaveScript(IActiveScriptSite *iface)
{
    return E_NOTIMPL;
}

#undef ACTSCPSITE_THIS

static const IActiveScriptSiteVtbl ActiveScriptSiteVtbl = {
    ActiveScriptSite_QueryInterface,
    ActiveScriptSite_AddRef,
    ActiveScriptSite_Release,
    ActiveScriptSite_GetLCID,
    ActiveScriptSite_GetItemInfo,
    ActiveScriptSite_GetDocVersionString,
    ActiveScriptSite_OnScriptTerminate,
    ActiveScriptSite_OnStateChange,
    ActiveScriptSite_OnScriptError,
    ActiveScriptSite_OnEnterScript,
    ActiveScriptSite_OnLeaveScript
};

static IActiveScriptSite ActiveScriptSite = { &ActiveScriptSiteVtbl };

static void set_safety_options(IUnknown *unk)
{
    IObjectSafety *safety;
    DWORD supported, enabled;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IObjectSafety, (void**)&safety);
    ok(hres == S_OK, "Could not get IObjectSafety: %08x\n", hres);
    if(FAILED(hres))
        return;

    hres = IObjectSafety_SetInterfaceSafetyOptions(safety, &IID_IActiveScriptParse,
            INTERFACESAFE_FOR_UNTRUSTED_DATA|INTERFACE_USES_DISPEX|INTERFACE_USES_SECURITY_MANAGER,
            INTERFACESAFE_FOR_UNTRUSTED_DATA|INTERFACE_USES_DISPEX|INTERFACE_USES_SECURITY_MANAGER);
    ok(hres == S_OK, "SetInterfaceSafetyOptions failed: %08x\n", hres);

    supported = enabled = 0xdeadbeef;
    hres = IObjectSafety_GetInterfaceSafetyOptions(safety, &IID_IActiveScriptParse, &supported, &enabled);
    ok(hres == S_OK, "GetInterfaceSafetyOptions failed: %08x\n", hres);
    ok(supported == (INTERFACESAFE_FOR_UNTRUSTED_DATA|INTERFACE_USES_DISPEX|INTERFACE_USES_SECURITY_MANAGER),
       "supported=%x\n", supported);
    ok(enabled == (INTERFACESAFE_FOR_UNTRUSTED_DATA|INTERFACE_USES_DISPEX|INTERFACE_USES_SECURITY_MANAGER),
       "enabled=%x\n", enabled);

    IObjectSafety_Release(safety);
}

#define parse_script_a(p,s) _parse_script_a(__LINE__,p,s)
static void _parse_script_a(unsigned line, IActiveScriptParse *parser, const char *script)
{
    BSTR str;
    HRESULT hres;

    str = a2bstr(script);
    hres = IActiveScriptParse64_ParseScriptText(parser, str, NULL, NULL, NULL, 0, 0, 0, NULL, NULL);
    SysFreeString(str);
    ok_(__FILE__,line)(hres == S_OK, "ParseScriptText failed: %08x\n", hres);
}

static IActiveScriptParse *create_script(void)
{
    IActiveScriptParse *parser;
    IActiveScript *script;
    HRESULT hres;

    QS_SecMgr_hres = S_OK;
    ProcessUrlAction_hres = S_OK;
    ProcessUrlAction_policy = URLPOLICY_ALLOW;
    CreateInstance_hres = S_OK;
    QueryCustomPolicy_hres = S_OK;
    QueryCustomPolicy_psize = sizeof(DWORD);
    QueryCustomPolicy_policy = URLPOLICY_ALLOW;
    QI_IDispatch_hres = S_OK;
    SetSite_hres = S_OK;

    hres = CoCreateInstance(&CLSID_JScript, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IActiveScript, (void**)&script);
    ok(hres == S_OK, "CoCreateInstance failed: %08x\n", hres);
    if(FAILED(hres))
        return NULL;

    set_safety_options((IUnknown*)script);

    hres = IActiveScript_QueryInterface(script, &IID_IActiveScriptParse, (void**)&parser);
    ok(hres == S_OK, "Could not get IActiveScriptParse: %08x\n", hres);

    hres = IActiveScriptParse64_InitNew(parser);
    ok(hres == S_OK, "InitNew failed: %08x\n", hres);

    hres = IActiveScript_SetScriptSite(script, &ActiveScriptSite);
    ok(hres == S_OK, "SetScriptSite failed: %08x\n", hres);

    hres = IActiveScript_AddNamedItem(script, testW,
            SCRIPTITEM_ISVISIBLE|SCRIPTITEM_ISSOURCE|SCRIPTITEM_GLOBALMEMBERS);
    ok(hres == S_OK, "AddNamedItem failed: %08x\n", hres);

    hres = IActiveScript_SetScriptState(script, SCRIPTSTATE_STARTED);
    ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_STARTED) failed: %08x\n", hres);

    IActiveScript_Release(script);

    parse_script_a(parser,
            "function testException(func, type, number) {\n"
            "try {\n"
            "    func();\n"
            "}catch(e) {\n"
            "    ok(e.name === type, 'e.name = ' + e.name + ', expected ' + type)\n"
            "    ok(e.number === number, 'e.number = ' + e.number + ', expected ' + number);\n"
            "    return;\n"
            "}"
            "ok(false, 'exception expected');\n"
            "}");

    return parser;
}

static IDispatchEx *parse_procedure_a(IActiveScriptParse *parser, const char *src)
{
    IActiveScriptParseProcedure2 *parse_proc;
    IDispatchEx *dispex;
    IDispatch *disp;
    BSTR str;
    HRESULT hres;

    hres = IUnknown_QueryInterface(parser, &IID_IActiveScriptParseProcedure2, (void**)&parse_proc);
    ok(hres == S_OK, "Coult not get IActiveScriptParseProcedure2: %08x\n", hres);

    str = a2bstr(src);
    hres = IActiveScriptParseProcedure2_64_ParseProcedureText(parse_proc, str, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, &disp);
    SysFreeString(str);
    IUnknown_Release(parse_proc);
    ok(hres == S_OK, "ParseProcedureText failed: %08x\n", hres);
    ok(disp != NULL, "disp == NULL\n");

    hres = IDispatch_QueryInterface(disp, &IID_IDispatchEx, (void**)&dispex);
    IDispatch_Release(dispex);
    ok(hres == S_OK, "Could not get IDispatchEx iface: %08x\n", hres);

    return dispex;
}

#define call_procedure(p,c) _call_procedure(__LINE__,p,c)
static void _call_procedure(unsigned line, IDispatchEx *proc, IServiceProvider *caller)
{
    DISPPARAMS dp = {NULL,NULL,0,0};
    EXCEPINFO ei = {0};
    HRESULT hres;

    hres = IDispatchEx_InvokeEx(proc, DISPID_VALUE, 0, DISPATCH_METHOD, &dp, NULL, &ei, caller);
    ok_(__FILE__,line)(hres == S_OK, "InvokeEx failed: %08x\n", hres);

}

static void test_ActiveXObject(void)
{
    IActiveScriptParse *parser;
    IDispatchEx *proc;

    parser = create_script();

    SET_EXPECT(Host_QS_SecMgr);
    SET_EXPECT(ProcessUrlAction);
    SET_EXPECT(CreateInstance);
    SET_EXPECT(QueryCustomPolicy);
    SET_EXPECT(QI_IObjectWithSite);
    SET_EXPECT(reportSuccess);
    parse_script_a(parser, "(new ActiveXObject('Wine.Test')).reportSuccess();");
    CHECK_CALLED(Host_QS_SecMgr);
    CHECK_CALLED(ProcessUrlAction);
    CHECK_CALLED(CreateInstance);
    CHECK_CALLED(QueryCustomPolicy);
    CHECK_CALLED(QI_IObjectWithSite);
    CHECK_CALLED(reportSuccess);

    proc = parse_procedure_a(parser, "(new ActiveXObject('Wine.Test')).reportSuccess();");

    SET_EXPECT(ProcessUrlAction);
    SET_EXPECT(CreateInstance);
    SET_EXPECT(QueryCustomPolicy);
    SET_EXPECT(QI_IObjectWithSite);
    SET_EXPECT(reportSuccess);
    call_procedure(proc, NULL);
    CHECK_CALLED(ProcessUrlAction);
    CHECK_CALLED(CreateInstance);
    CHECK_CALLED(QueryCustomPolicy);
    CHECK_CALLED(QI_IObjectWithSite);
    CHECK_CALLED(reportSuccess);

    SET_EXPECT(ProcessUrlAction);
    SET_EXPECT(CreateInstance);
    SET_EXPECT(QueryCustomPolicy);
    SET_EXPECT(QI_IObjectWithSite);
    SET_EXPECT(reportSuccess);
    call_procedure(proc, &caller_sp);
    CHECK_CALLED(ProcessUrlAction);
    CHECK_CALLED(CreateInstance);
    CHECK_CALLED(QueryCustomPolicy);
    CHECK_CALLED(QI_IObjectWithSite);
    CHECK_CALLED(reportSuccess);

    IDispatchEx_Release(proc);
    IUnknown_Release(parser);

    parser = create_script();
    proc = parse_procedure_a(parser, "(new ActiveXObject('Wine.Test')).reportSuccess();");

    SET_EXPECT(Host_QS_SecMgr);
    SET_EXPECT(ProcessUrlAction);
    SET_EXPECT(CreateInstance);
    SET_EXPECT(QueryCustomPolicy);
    SET_EXPECT(QI_IObjectWithSite);
    SET_EXPECT(reportSuccess);
    call_procedure(proc, &caller_sp);
    CHECK_CALLED(Host_QS_SecMgr);
    CHECK_CALLED(ProcessUrlAction);
    CHECK_CALLED(CreateInstance);
    CHECK_CALLED(QueryCustomPolicy);
    CHECK_CALLED(QI_IObjectWithSite);
    CHECK_CALLED(reportSuccess);

    parse_script_a(parser, "testException(function() { new ActiveXObject('Wine.TestABC'); }, 'Error', -2146827859);");

    IDispatchEx_Release(proc);
    IUnknown_Release(parser);

    parser = create_script();
    QS_SecMgr_hres = E_NOINTERFACE;

    SET_EXPECT(Host_QS_SecMgr);
    parse_script_a(parser, "testException(function() { new ActiveXObject('Wine.Test'); }, 'Error', -2146827859);");
    CHECK_CALLED(Host_QS_SecMgr);

    IUnknown_Release(parser);

    parser = create_script();
    ProcessUrlAction_hres = E_FAIL;

    SET_EXPECT(Host_QS_SecMgr);
    SET_EXPECT(ProcessUrlAction);
    parse_script_a(parser, "testException(function() { new ActiveXObject('Wine.Test'); }, 'Error', -2146827859);");
    CHECK_CALLED(Host_QS_SecMgr);
    CHECK_CALLED(ProcessUrlAction);

    IUnknown_Release(parser);

    parser = create_script();
    ProcessUrlAction_policy = URLPOLICY_DISALLOW;

    SET_EXPECT(Host_QS_SecMgr);
    SET_EXPECT(ProcessUrlAction);
    parse_script_a(parser, "testException(function() { new ActiveXObject('Wine.Test'); }, 'Error', -2146827859);");
    CHECK_CALLED(Host_QS_SecMgr);
    CHECK_CALLED(ProcessUrlAction);

    IUnknown_Release(parser);

    parser = create_script();
    CreateInstance_hres = E_FAIL;

    SET_EXPECT(Host_QS_SecMgr);
    SET_EXPECT(ProcessUrlAction);
    SET_EXPECT(CreateInstance);
    parse_script_a(parser, "testException(function() { new ActiveXObject('Wine.Test'); }, 'Error', -2146827859);");
    CHECK_CALLED(Host_QS_SecMgr);
    CHECK_CALLED(ProcessUrlAction);
    CHECK_CALLED(CreateInstance);

    IUnknown_Release(parser);

    parser = create_script();
    QueryCustomPolicy_hres = E_FAIL;

    SET_EXPECT(Host_QS_SecMgr);
    SET_EXPECT(ProcessUrlAction);
    SET_EXPECT(CreateInstance);
    SET_EXPECT(QueryCustomPolicy);
    parse_script_a(parser, "testException(function() { new ActiveXObject('Wine.Test'); }, 'Error', -2146827859);");
    CHECK_CALLED(Host_QS_SecMgr);
    CHECK_CALLED(ProcessUrlAction);
    CHECK_CALLED(CreateInstance);
    CHECK_CALLED(QueryCustomPolicy);

    IUnknown_Release(parser);

    parser = create_script();
    QueryCustomPolicy_psize = 6;

    SET_EXPECT(Host_QS_SecMgr);
    SET_EXPECT(ProcessUrlAction);
    SET_EXPECT(CreateInstance);
    SET_EXPECT(QueryCustomPolicy);
    SET_EXPECT(QI_IObjectWithSite);
    SET_EXPECT(reportSuccess);
    parse_script_a(parser, "(new ActiveXObject('Wine.Test')).reportSuccess();");
    CHECK_CALLED(Host_QS_SecMgr);
    CHECK_CALLED(ProcessUrlAction);
    CHECK_CALLED(CreateInstance);
    CHECK_CALLED(QueryCustomPolicy);
    CHECK_CALLED(QI_IObjectWithSite);
    CHECK_CALLED(reportSuccess);

    IUnknown_Release(parser);

    parser = create_script();
    QueryCustomPolicy_policy = URLPOLICY_DISALLOW;

    SET_EXPECT(Host_QS_SecMgr);
    SET_EXPECT(ProcessUrlAction);
    SET_EXPECT(CreateInstance);
    SET_EXPECT(QueryCustomPolicy);
    parse_script_a(parser, "testException(function() { new ActiveXObject('Wine.Test'); }, 'Error', -2146827859);");
    CHECK_CALLED(Host_QS_SecMgr);
    CHECK_CALLED(ProcessUrlAction);
    CHECK_CALLED(CreateInstance);
    CHECK_CALLED(QueryCustomPolicy);

    QueryCustomPolicy_psize = 6;

    SET_EXPECT(ProcessUrlAction);
    SET_EXPECT(CreateInstance);
    SET_EXPECT(QueryCustomPolicy);
    parse_script_a(parser, "testException(function() { new ActiveXObject('Wine.Test'); }, 'Error', -2146827859);");
    CHECK_CALLED(ProcessUrlAction);
    CHECK_CALLED(CreateInstance);
    CHECK_CALLED(QueryCustomPolicy);

    QueryCustomPolicy_policy = URLPOLICY_ALLOW;
    QueryCustomPolicy_psize = 3;

    SET_EXPECT(ProcessUrlAction);
    SET_EXPECT(CreateInstance);
    SET_EXPECT(QueryCustomPolicy);
    parse_script_a(parser, "testException(function() { new ActiveXObject('Wine.Test'); }, 'Error', -2146827859);");
    CHECK_CALLED(ProcessUrlAction);
    CHECK_CALLED(CreateInstance);
    CHECK_CALLED(QueryCustomPolicy);

    IUnknown_Release(parser);

    parser = create_script();
    object_with_site = &ObjectWithSite;

    SET_EXPECT(Host_QS_SecMgr);
    SET_EXPECT(ProcessUrlAction);
    SET_EXPECT(CreateInstance);
    SET_EXPECT(QueryCustomPolicy);
    SET_EXPECT(QI_IObjectWithSite);
    SET_EXPECT(SetSite);
    SET_EXPECT(reportSuccess);
    parse_script_a(parser, "(new ActiveXObject('Wine.Test')).reportSuccess();");
    CHECK_CALLED(Host_QS_SecMgr);
    CHECK_CALLED(ProcessUrlAction);
    CHECK_CALLED(CreateInstance);
    CHECK_CALLED(QueryCustomPolicy);
    CHECK_CALLED(QI_IObjectWithSite);
    CHECK_CALLED(SetSite);
    CHECK_CALLED(reportSuccess);

    SetSite_hres = E_FAIL;
    SET_EXPECT(ProcessUrlAction);
    SET_EXPECT(CreateInstance);
    SET_EXPECT(QueryCustomPolicy);
    SET_EXPECT(QI_IObjectWithSite);
    SET_EXPECT(SetSite);
    parse_script_a(parser, "testException(function() { new ActiveXObject('Wine.Test'); }, 'Error', -2146827859);");
    CHECK_CALLED(ProcessUrlAction);
    CHECK_CALLED(CreateInstance);
    CHECK_CALLED(QueryCustomPolicy);
    CHECK_CALLED(QI_IObjectWithSite);
    CHECK_CALLED(SetSite);

    IUnknown_Release(parser);
}

static BOOL init_key(const char *key_name, const char *def_value, BOOL init)
{
    HKEY hkey;
    DWORD res;

    if(!init) {
        RegDeleteKey(HKEY_CLASSES_ROOT, key_name);
        return TRUE;
    }

    res = RegCreateKeyA(HKEY_CLASSES_ROOT, key_name, &hkey);
    if(res != ERROR_SUCCESS)
        return FALSE;

    if(def_value)
        res = RegSetValueA(hkey, NULL, REG_SZ, def_value, strlen(def_value));

    RegCloseKey(hkey);

    return res == ERROR_SUCCESS;
}

static BOOL init_registry(BOOL init)
{
    return init_key("Wine.Test\\CLSID", TESTOBJ_CLSID, init);
}

static BOOL register_activex(void)
{
    DWORD regid;
    HRESULT hres;

    if(!init_registry(TRUE)) {
        init_registry(FALSE);
        return FALSE;
    }

    hres = CoRegisterClassObject(&CLSID_TestObj, (IUnknown *)&activex_cf,
                                 CLSCTX_INPROC_SERVER, REGCLS_MULTIPLEUSE, &regid);
    ok(hres == S_OK, "Could not register screipt engine: %08x\n", hres);

    return TRUE;
}

static BOOL check_jscript(void)
{
    IActiveScriptParse *parser;
    BSTR str;
    HRESULT hres;

    parser = create_script();
    if(!parser)
        return FALSE;

    str = a2bstr("if(!('localeCompare' in String.prototype)) throw 1;");
    hres = IActiveScriptParse64_ParseScriptText(parser, str, NULL, NULL, NULL, 0, 0, 0, NULL, NULL);
    SysFreeString(str);
    IUnknown_Release(parser);

    return hres == S_OK;
}

START_TEST(activex)
{
    CoInitialize(NULL);

    if(check_jscript()) {
        register_activex();

        test_ActiveXObject();

        init_registry(FALSE);
    }else {
        win_skip("Broken engine, probably too old\n");
    }

    CoUninitialize();
}
