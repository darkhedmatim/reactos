/*
 * Copyright (C) 1998-1999 Francois Gouget
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

#include <rpc.h>
#include <rpcndr.h>

#ifndef _OBJBASE_H_
#define _OBJBASE_H_

/*****************************************************************************
 * Macros to define a COM interface
 */
/*
 * The goal of the following set of definitions is to provide a way to use the same
 * header file definitions to provide both a C interface and a C++ object oriented
 * interface to COM interfaces. The type of interface is selected automatically
 * depending on the language but it is always possible to get the C interface in C++
 * by defining CINTERFACE.
 *
 * It is based on the following assumptions:
 *  - all COM interfaces derive from IUnknown, this should not be a problem.
 *  - the header file only defines the interface, the actual fields are defined
 *    separately in the C file implementing the interface.
 *
 * The natural approach to this problem would be to make sure we get a C++ class and
 * virtual methods in C++ and a structure with a table of pointer to functions in C.
 * Unfortunately the layout of the virtual table is compiler specific, the layout of
 * g++ virtual tables is not the same as that of an egcs virtual table which is not the
 * same as that generated by Visual C+. There are workarounds to make the virtual tables
 * compatible via padding but unfortunately the one which is imposed to the WINE emulator
 * by the Windows binaries, i.e. the Visual C++ one, is the most compact of all.
 *
 * So the solution I finally adopted does not use virtual tables. Instead I use inline
 * non virtual methods that dereference the method pointer themselves and perform the call.
 *
 * Let's take Direct3D as an example:
 *
 *    #define INTERFACE IDirect3D
 *    DECLARE_INTERFACE_(IDirect3D,IUnknown)
 *    {
 *        // *** IUnknown methods *** //
 *        STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID, void**) PURE;
 *        STDMETHOD_(ULONG,AddRef)(THIS) PURE;
 *        STDMETHOD_(ULONG,Release)(THIS) PURE;
 *        // *** IDirect3D methods *** //
 *        STDMETHOD(Initialize)(THIS_ REFIID) PURE;
 *        STDMETHOD(EnumDevices)(THIS_ LPD3DENUMDEVICESCALLBACK, LPVOID) PURE;
 *        STDMETHOD(CreateLight)(THIS_ LPDIRECT3DLIGHT *, IUnknown *) PURE;
 *        STDMETHOD(CreateMaterial)(THIS_ LPDIRECT3DMATERIAL *, IUnknown *) PURE;
 *        STDMETHOD(CreateViewport)(THIS_ LPDIRECT3DVIEWPORT *, IUnknown *) PURE;
 *        STDMETHOD(FindDevice)(THIS_ LPD3DFINDDEVICESEARCH, LPD3DFINDDEVICERESULT) PURE;
 *    };
 *    #undef INTERFACE
 *
 *    #ifdef COBJMACROS
 *    // *** IUnknown methods *** //
 *    #define IDirect3D_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
 *    #define IDirect3D_AddRef(p)             (p)->lpVtbl->AddRef(p)
 *    #define IDirect3D_Release(p)            (p)->lpVtbl->Release(p)
 *    // *** IDirect3D methods *** //
 *    #define IDirect3D_Initialize(p,a)       (p)->lpVtbl->Initialize(p,a)
 *    #define IDirect3D_EnumDevices(p,a,b)    (p)->lpVtbl->EnumDevice(p,a,b)
 *    #define IDirect3D_CreateLight(p,a,b)    (p)->lpVtbl->CreateLight(p,a,b)
 *    #define IDirect3D_CreateMaterial(p,a,b) (p)->lpVtbl->CreateMaterial(p,a,b)
 *    #define IDirect3D_CreateViewport(p,a,b) (p)->lpVtbl->CreateViewport(p,a,b)
 *    #define IDirect3D_FindDevice(p,a,b)     (p)->lpVtbl->FindDevice(p,a,b)
 *    #endif
 *
 * Comments:
 *  - The INTERFACE macro is used in the STDMETHOD macros to define the type of the 'this'
 *    pointer. Defining this macro here saves us the trouble of having to repeat the interface
 *    name everywhere. Note however that because of the way macros work, a macro like STDMETHOD
 *    cannot use 'INTERFACE##_VTABLE' because this would give 'INTERFACE_VTABLE' and not
 *    'IDirect3D_VTABLE'.
 *  - The DECLARE_INTERFACE declares all the structures necessary for the interface. We have to
 *    explicitly use the interface name for macro expansion reasons again. It defines the list of
 *    methods that are inheritable from this interface. It must be written manually (rather than
 *    using a macro to generate the equivalent code) to avoid macro recursion (which compilers
 *    don't like). It must start with the methods definition of the parent interface so that
 *    method inheritance works properly.
 *  - The 'undef INTERFACE' is here to remind you that using INTERFACE in the following macros
 *    will not work.
 *  - Finally the set of 'IDirect3D_Xxx' macros is a standard set of macros defined to ease access
 *    to the interface methods in C. Unfortunately I don't see any way to avoid having to duplicate
 *    the inherited method definitions there. This time I could have used a trick to use only one
 *    macro whatever the number of parameters but I preferred to have it work the same way as above.
 *  - You probably have noticed that we don't define the fields we need to actually implement this
 *    interface: reference count, pointer to other resources and miscellaneous fields. That's
 *    because these interfaces are just that: interfaces. They may be implemented more than once, in
 *    different contexts and sometimes not even in Wine. Thus it would not make sense to impose
 *    that the interface contains some specific fields.
 *
 *
 * In C this gives:
 *    typedef struct IDirect3DVtbl IDirect3DVtbl;
 *    struct IDirect3D {
 *        IDirect3DVtbl* lpVtbl;
 *    };
 *    struct IDirect3DVtbl {
 *        HRESULT (*QueryInterface)(IDirect3D* me, REFIID riid, LPVOID* ppvObj);
 *        ULONG (*AddRef)(IDirect3D* me);
 *        ULONG (*Release)(IDirect3D* me);
 *        HRESULT (*Initialize)(IDirect3D* me, REFIID a);
 *        HRESULT (*EnumDevices)(IDirect3D* me, LPD3DENUMDEVICESCALLBACK a, LPVOID b);
 *        HRESULT (*CreateLight)(IDirect3D* me, LPDIRECT3DLIGHT* a, IUnknown* b);
 *        HRESULT (*CreateMaterial)(IDirect3D* me, LPDIRECT3DMATERIAL* a, IUnknown* b);
 *        HRESULT (*CreateViewport)(IDirect3D* me, LPDIRECT3DVIEWPORT* a, IUnknown* b);
 *        HRESULT (*FindDevice)(IDirect3D* me, LPD3DFINDDEVICESEARCH a, LPD3DFINDDEVICERESULT b);
 *    };
 *
 *    #ifdef COBJMACROS
 *    // *** IUnknown methods *** //
 *    #define IDirect3D_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
 *    #define IDirect3D_AddRef(p)             (p)->lpVtbl->AddRef(p)
 *    #define IDirect3D_Release(p)            (p)->lpVtbl->Release(p)
 *    // *** IDirect3D methods *** //
 *    #define IDirect3D_Initialize(p,a)       (p)->lpVtbl->Initialize(p,a)
 *    #define IDirect3D_EnumDevices(p,a,b)    (p)->lpVtbl->EnumDevice(p,a,b)
 *    #define IDirect3D_CreateLight(p,a,b)    (p)->lpVtbl->CreateLight(p,a,b)
 *    #define IDirect3D_CreateMaterial(p,a,b) (p)->lpVtbl->CreateMaterial(p,a,b)
 *    #define IDirect3D_CreateViewport(p,a,b) (p)->lpVtbl->CreateViewport(p,a,b)
 *    #define IDirect3D_FindDevice(p,a,b)     (p)->lpVtbl->FindDevice(p,a,b)
 *    #endif
 *
 * Comments:
 *  - IDirect3D only contains a pointer to the IDirect3D virtual/jump table. This is the only thing
 *    the user needs to know to use the interface. Of course the structure we will define to
 *    implement this interface will have more fields but the first one will match this pointer.
 *  - The code generated by DECLARE_INTERFACE defines both the structure representing the interface and
 *    the structure for the jump table.
 *  - Each method is declared as a pointer to function field in the jump table. The implementation
 *    will fill this jump table with appropriate values, probably using a static variable, and
 *    initialize the lpVtbl field to point to this variable.
 *  - The IDirect3D_Xxx macros then just derefence the lpVtbl pointer and use the function pointer
 *    corresponding to the macro name. This emulates the behavior of a virtual table and should be
 *    just as fast.
 *  - This C code should be quite compatible with the Windows headers both for code that uses COM
 *    interfaces and for code implementing a COM interface.
 *
 *
 * And in C++ (with gcc's g++):
 *
 *    typedef struct IDirect3D: public IUnknown {
 *        virtual HRESULT Initialize(REFIID a) = 0;
 *        virtual HRESULT EnumDevices(LPD3DENUMDEVICESCALLBACK a, LPVOID b) = 0;
 *        virtual HRESULT CreateLight(LPDIRECT3DLIGHT* a, IUnknown* b) = 0;
 *        virtual HRESULT CreateMaterial(LPDIRECT3DMATERIAL* a, IUnknown* b) = 0;
 *        virtual HRESULT CreateViewport(LPDIRECT3DVIEWPORT* a, IUnknown* b) = 0;
 *        virtual HRESULT FindDevice(LPD3DFINDDEVICESEARCH a, LPD3DFINDDEVICERESULT b) = 0;
 *    };
 *
 * Comments:
 *  - Of course in C++ we use inheritance so that we don't have to duplicate the method definitions.
 *  - Finally there is no IDirect3D_Xxx macro. These are not needed in C++ unless the CINTERFACE
 *    macro is defined in which case we would not be here.
 *
 *
 * Implementing a COM interface.
 *
 * This continues the above example. This example assumes that the implementation is in C.
 *
 *    typedef struct IDirect3DImpl {
 *        void* lpVtbl;
 *        // ...
 *
 *    } IDirect3DImpl;
 *
 *    static IDirect3DVtbl d3dvt;
 *
 *    // implement the IDirect3D methods here
 *
 *    int IDirect3D_QueryInterface(IDirect3D* me)
 *    {
 *        IDirect3DImpl *This = (IDirect3DImpl *)me;
 *        // ...
 *    }
 *
 *    // ...
 *
 *    static IDirect3DVtbl d3dvt = {
 *        IDirect3D_QueryInterface,
 *        IDirect3D_Add,
 *        IDirect3D_Add2,
 *        IDirect3D_Initialize,
 *        IDirect3D_SetWidth
 *    };
 *
 * Comments:
 *  - We first define what the interface really contains. This is the IDirect3DImpl structure. The
 *    first field must of course be the virtual table pointer. Everything else is free.
 *  - Then we predeclare our static virtual table variable, we will need its address in some
 *    methods to initialize the virtual table pointer of the returned interface objects.
 *  - Then we implement the interface methods. To match what has been declared in the header file
 *    they must take a pointer to an IDirect3D structure and we must cast it to an IDirect3DImpl so
 *    that we can manipulate the fields.
 *  - Finally we initialize the virtual table.
 */

#if defined(__cplusplus) && !defined(CINTERFACE)

/* C++ interface */

#define STDMETHOD(method)        virtual HRESULT STDMETHODCALLTYPE method
#define STDMETHOD_(type,method)  virtual type STDMETHODCALLTYPE method
#define STDMETHODV(method)       virtual HRESULT STDMETHODVCALLTYPE method
#define STDMETHODV_(type,method) virtual type STDMETHODVCALLTYPE method

#define PURE   = 0
#define THIS_
#define THIS   void

#define interface struct
#define DECLARE_INTERFACE(iface)        interface DECLSPEC_NOVTABLE iface
#define DECLARE_INTERFACE_(iface,ibase) interface DECLSPEC_NOVTABLE iface : public ibase
#define DECLARE_INTERFACE_IID_(iface, ibase, iid) interface DECLSPEC_UUID(iid) DECLSPEC_NOVTABLE iface : public ibase

#define BEGIN_INTERFACE
#define END_INTERFACE

#else  /* __cplusplus && !CINTERFACE */

/* C interface */

#define STDMETHOD(method)        HRESULT (STDMETHODCALLTYPE *method)
#define STDMETHOD_(type,method)  type (STDMETHODCALLTYPE *method)
#define STDMETHODV(method)       HRESULT (STDMETHODVCALLTYPE *method)
#define STDMETHODV_(type,method) type (STDMETHODVCALLTYPE *method)

#define PURE
#define THIS_ INTERFACE *This,
#define THIS  INTERFACE *This

#define interface struct

#ifdef __WINESRC__
#define CONST_VTABLE
#endif

#ifdef CONST_VTABLE
#undef CONST_VTBL
#define CONST_VTBL const
#define DECLARE_INTERFACE(iface) \
         typedef interface iface { const struct iface##Vtbl *lpVtbl; } iface; \
         typedef struct iface##Vtbl iface##Vtbl; \
         struct iface##Vtbl
#else
#undef CONST_VTBL
#define CONST_VTBL
#define DECLARE_INTERFACE(iface) \
         typedef interface iface { struct iface##Vtbl *lpVtbl; } iface; \
         typedef struct iface##Vtbl iface##Vtbl; \
         struct iface##Vtbl
#endif
#define DECLARE_INTERFACE_(iface,ibase) DECLARE_INTERFACE(iface)
#define DECLARE_INTERFACE_IID_(iface, ibase, iid) DECLARE_INTERFACE_(iface, ibase)

#define BEGIN_INTERFACE
#define END_INTERFACE

#endif  /* __cplusplus && !CINTERFACE */

#ifndef __IRpcStubBuffer_FWD_DEFINED__
#define __IRpcStubBuffer_FWD_DEFINED__
typedef interface IRpcStubBuffer IRpcStubBuffer;
#endif
#ifndef __IRpcChannelBuffer_FWD_DEFINED__
#define __IRpcChannelBuffer_FWD_DEFINED__
typedef interface IRpcChannelBuffer IRpcChannelBuffer;
#endif

#ifndef RC_INVOKED
/* For compatibility only, at least for now */
#include <stdlib.h>
#endif

#include <wtypes.h>
#include <unknwn.h>
#include <objidl.h>

#include <guiddef.h>
#ifndef INITGUID
#include <cguid.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef NONAMELESSSTRUCT
#define LISet32(li, v)   ((li).HighPart = (v) < 0 ? -1 : 0, (li).LowPart = (v))
#define ULISet32(li, v)  ((li).HighPart = 0, (li).LowPart = (v))
#else
#define LISet32(li, v)   ((li).u.HighPart = (v) < 0 ? -1 : 0, (li).u.LowPart = (v))
#define ULISet32(li, v)  ((li).u.HighPart = 0, (li).u.LowPart = (v))
#endif

/*****************************************************************************
 *	Standard API
 */
DWORD WINAPI CoBuildVersion(void);

typedef enum tagCOINIT
{
    COINIT_APARTMENTTHREADED  = 0x2, /* Apartment model */
    COINIT_MULTITHREADED      = 0x0, /* OLE calls objects on any thread */
    COINIT_DISABLE_OLE1DDE    = 0x4, /* Don't use DDE for Ole1 support */
    COINIT_SPEED_OVER_MEMORY  = 0x8  /* Trade memory for speed */
} COINIT;

_Check_return_ HRESULT WINAPI CoInitialize(_In_opt_ LPVOID lpReserved);

_Check_return_
HRESULT
WINAPI
CoInitializeEx(
  _In_opt_ LPVOID lpReserved,
  _In_ DWORD dwCoInit);

void WINAPI CoUninitialize(void);
DWORD WINAPI CoGetCurrentProcess(void);

HINSTANCE WINAPI CoLoadLibrary(_In_ LPOLESTR lpszLibName, _In_ BOOL bAutoFree);
void WINAPI CoFreeAllLibraries(void);
void WINAPI CoFreeLibrary(_In_ HINSTANCE hLibrary);
void WINAPI CoFreeUnusedLibraries(void);

void
WINAPI
CoFreeUnusedLibrariesEx(
  _In_ DWORD dwUnloadDelay,
  _In_ DWORD dwReserved);

_Check_return_
HRESULT
WINAPI
CoCreateInstance(
  _In_ REFCLSID rclsid,
  _In_opt_ LPUNKNOWN pUnkOuter,
  _In_ DWORD dwClsContext,
  _In_ REFIID iid,
  _Outptr_ _At_(*ppv, _Post_readable_size_(_Inexpressible_(varies))) LPVOID *ppv);

_Check_return_
HRESULT
WINAPI
CoCreateInstanceEx(
  _In_ REFCLSID rclsid,
  _In_opt_ LPUNKNOWN pUnkOuter,
  _In_ DWORD dwClsContext,
  _In_opt_ COSERVERINFO *pServerInfo,
  _In_ ULONG cmq,
  _Inout_updates_(cmq) MULTI_QI *pResults);

_Check_return_
HRESULT
WINAPI
CoGetInstanceFromFile(
  _In_opt_ COSERVERINFO *pServerInfo,
  _In_opt_ CLSID *pClsid,
  _In_opt_ IUnknown *punkOuter,
  _In_ DWORD dwClsCtx,
  _In_ DWORD grfMode,
  _In_ _Null_terminated_ OLECHAR *pwszName,
  _In_ DWORD dwCount,
  _Inout_updates_(dwCount) MULTI_QI *pResults);

_Check_return_
HRESULT
WINAPI
CoGetInstanceFromIStorage(
  _In_opt_ COSERVERINFO *pServerInfo,
  _In_opt_ CLSID *pClsid,
  _In_opt_ IUnknown *punkOuter,
  _In_ DWORD dwClsCtx,
  _In_ IStorage *pstg,
  _In_ DWORD dwCount,
  _Inout_updates_(dwCount) MULTI_QI *pResults);

_Check_return_
HRESULT
WINAPI
CoGetMalloc(
  _In_ DWORD dwMemContext,
  _Outptr_ LPMALLOC *lpMalloc);

_Ret_opt_
_Post_writable_byte_size_(size)
__drv_allocatesMem(Mem)
_Check_return_
LPVOID
WINAPI
CoTaskMemAlloc(_In_ ULONG size) __WINE_ALLOC_SIZE(1);

void
WINAPI
CoTaskMemFree(
  _In_opt_ __drv_freesMem(Mem) _Post_invalid_ LPVOID ptr);

_Ret_opt_
_Post_writable_byte_size_(size)
_When_(size > 0, __drv_allocatesMem(Mem) _Check_return_)
LPVOID
WINAPI
CoTaskMemRealloc(
  _In_opt_ __drv_freesMem(Mem) _Post_invalid_ LPVOID ptr,
  _In_ ULONG size);

HRESULT WINAPI CoRegisterMallocSpy(_In_ LPMALLOCSPY pMallocSpy);
HRESULT WINAPI CoRevokeMallocSpy(void);

_Check_return_ HRESULT WINAPI CoGetContextToken(_Out_ ULONG_PTR *token);

/* class registration flags; passed to CoRegisterClassObject */
typedef enum tagREGCLS
{
    REGCLS_SINGLEUSE = 0,
    REGCLS_MULTIPLEUSE = 1,
    REGCLS_MULTI_SEPARATE = 2,
    REGCLS_SUSPENDED = 4,
    REGCLS_SURROGATE = 8
} REGCLS;

_Check_return_
HRESULT
WINAPI
CoGetClassObject(
  _In_ REFCLSID rclsid,
  _In_ DWORD dwClsContext,
  _In_opt_ COSERVERINFO *pServerInfo,
  _In_ REFIID iid,
  _Outptr_ LPVOID *ppv);

_Check_return_
HRESULT
WINAPI
CoRegisterClassObject(
  _In_ REFCLSID rclsid,
  _In_ LPUNKNOWN pUnk,
  _In_ DWORD dwClsContext,
  _In_ DWORD flags,
  _Out_ LPDWORD lpdwRegister);

_Check_return_
HRESULT
WINAPI
CoRevokeClassObject(
  _In_ DWORD dwRegister);

_Check_return_
HRESULT
WINAPI
CoGetPSClsid(
  _In_ REFIID riid,
  _Out_ CLSID *pclsid);

_Check_return_
HRESULT
WINAPI
CoRegisterPSClsid(
  _In_ REFIID riid,
  _In_ REFCLSID rclsid);

_Check_return_ HRESULT WINAPI CoRegisterSurrogate(_In_ LPSURROGATE pSurrogate);
_Check_return_ HRESULT WINAPI CoSuspendClassObjects(void);
_Check_return_ HRESULT WINAPI CoResumeClassObjects(void);
ULONG WINAPI CoAddRefServerProcess(void);
ULONG WINAPI CoReleaseServerProcess(void);

/* marshalling */

_Check_return_
HRESULT
WINAPI
CoCreateFreeThreadedMarshaler(
  _In_opt_ LPUNKNOWN punkOuter,
  _Outptr_ LPUNKNOWN *ppunkMarshal);

_Check_return_
HRESULT
WINAPI
CoGetInterfaceAndReleaseStream(
  _In_ LPSTREAM pStm,
  _In_ REFIID iid,
  _Outptr_ LPVOID *ppv);

_Check_return_
HRESULT
WINAPI
CoGetMarshalSizeMax(
  _Out_ ULONG *pulSize,
  _In_ REFIID riid,
  _In_ LPUNKNOWN pUnk,
  _In_ DWORD dwDestContext,
  _In_opt_ LPVOID pvDestContext,
  _In_ DWORD mshlflags);

_Check_return_
HRESULT
WINAPI
CoGetStandardMarshal(
  _In_ REFIID riid,
  _In_ LPUNKNOWN pUnk,
  _In_ DWORD dwDestContext,
  _In_opt_ LPVOID pvDestContext,
  _In_ DWORD mshlflags,
  _Outptr_ LPMARSHAL *ppMarshal);

HRESULT WINAPI CoMarshalHresult(_In_ LPSTREAM pstm, _In_ HRESULT hresult);

_Check_return_
HRESULT
WINAPI
CoMarshalInterface(
  _In_ LPSTREAM pStm,
  _In_ REFIID riid,
  _In_ LPUNKNOWN pUnk,
  _In_ DWORD dwDestContext,
  _In_opt_ LPVOID pvDestContext,
  _In_ DWORD mshlflags);

_Check_return_
HRESULT
WINAPI
CoMarshalInterThreadInterfaceInStream(
  _In_ REFIID riid,
  _In_ LPUNKNOWN pUnk,
  _Outptr_ LPSTREAM *ppStm);

_Check_return_ HRESULT WINAPI CoReleaseMarshalData(_In_ LPSTREAM pStm);

_Check_return_
HRESULT
WINAPI
CoDisconnectObject(
  _In_ LPUNKNOWN lpUnk,
  _In_ DWORD reserved);

HRESULT WINAPI CoUnmarshalHresult(_In_ LPSTREAM pstm, _Out_ HRESULT *phresult);

_Check_return_
HRESULT
WINAPI
CoUnmarshalInterface(
  _In_ LPSTREAM pStm,
  _In_ REFIID riid,
  _Outptr_ LPVOID *ppv);

_Check_return_
HRESULT
WINAPI
CoLockObjectExternal(
  _In_ LPUNKNOWN pUnk,
  _In_ BOOL fLock,
  _In_ BOOL fLastUnlockReleases);

BOOL WINAPI CoIsHandlerConnected(_In_ LPUNKNOWN pUnk);

/* security */

_Check_return_
HRESULT
WINAPI
CoInitializeSecurity(
  _In_opt_ PSECURITY_DESCRIPTOR pSecDesc,
  _In_ LONG cAuthSvc,
  _In_reads_opt_(cAuthSvc) SOLE_AUTHENTICATION_SERVICE *asAuthSvc,
  _In_opt_ void *pReserved1,
  _In_ DWORD dwAuthnLevel,
  _In_ DWORD dwImpLevel,
  _In_opt_ void *pReserved2,
  _In_ DWORD dwCapabilities,
  _In_opt_ void *pReserved3);

_Check_return_
HRESULT
WINAPI
CoGetCallContext(
  _In_ REFIID riid,
  _Outptr_ void **ppInterface);

_Check_return_
HRESULT
WINAPI
CoSwitchCallContext(
  _In_opt_ IUnknown *pContext,
  _Outptr_ IUnknown **ppOldContext);

_Check_return_
HRESULT
WINAPI
CoQueryAuthenticationServices(
  _Out_ DWORD *pcAuthSvc,
  _Outptr_result_buffer_(*pcAuthSvc) SOLE_AUTHENTICATION_SERVICE **asAuthSvc);

_Check_return_
HRESULT
WINAPI
CoQueryProxyBlanket(
  _In_ IUnknown *pProxy,
  _Out_opt_ DWORD *pwAuthnSvc,
  _Out_opt_ DWORD *pAuthzSvc,
  _Outptr_opt_ OLECHAR **pServerPrincName,
  _Out_opt_ DWORD *pAuthnLevel,
  _Out_opt_ DWORD *pImpLevel,
  _Out_opt_ RPC_AUTH_IDENTITY_HANDLE *pAuthInfo,
  _Out_opt_ DWORD *pCapabilities);

_Check_return_
HRESULT
WINAPI
CoSetProxyBlanket(
  _In_ IUnknown *pProxy,
  _In_ DWORD dwAuthnSvc,
  _In_ DWORD dwAuthzSvc,
  _In_opt_ OLECHAR *pServerPrincName,
  _In_ DWORD dwAuthnLevel,
  _In_ DWORD dwImpLevel,
  _In_opt_ RPC_AUTH_IDENTITY_HANDLE pAuthInfo,
  _In_ DWORD dwCapabilities);

_Check_return_
HRESULT
WINAPI CoCopyProxy(
  _In_ IUnknown *pProxy,
  _Outptr_ IUnknown **ppCopy);

_Check_return_ HRESULT WINAPI CoImpersonateClient(void);

_Check_return_
HRESULT
WINAPI
CoQueryClientBlanket(
  _Out_opt_ DWORD *pAuthnSvc,
  _Out_opt_ DWORD *pAuthzSvc,
  _Outptr_opt_ OLECHAR **pServerPrincName,
  _Out_opt_ DWORD *pAuthnLevel,
  _Out_opt_ DWORD *pImpLevel,
  _Outptr_opt_ RPC_AUTHZ_HANDLE *pPrivs,
  _Inout_opt_ DWORD *pCapabilities);

_Check_return_ HRESULT WINAPI CoRevertToSelf(void);

/* misc */

_Check_return_
HRESULT
WINAPI
CoGetTreatAsClass(
  _In_ REFCLSID clsidOld,
  _Out_ LPCLSID pClsidNew);

_Check_return_
HRESULT
WINAPI
CoTreatAsClass(
  _In_ REFCLSID clsidOld,
  _In_ REFCLSID clsidNew);

HRESULT
WINAPI
CoAllowSetForegroundWindow(
  _In_ IUnknown *pUnk,
  _In_opt_ LPVOID lpvReserved);

_Check_return_
HRESULT
WINAPI
CoGetObjectContext(
  _In_ REFIID riid,
  _Outptr_ LPVOID *ppv);

_Check_return_ HRESULT WINAPI CoCreateGuid(_Out_ GUID *pguid);
BOOL WINAPI CoIsOle1Class(_In_ REFCLSID rclsid);

BOOL
WINAPI
CoDosDateTimeToFileTime(
  _In_ WORD nDosDate,
  _In_ WORD nDosTime,
  _Out_ FILETIME *lpFileTime);

BOOL
WINAPI
CoFileTimeToDosDateTime(
  _In_ FILETIME *lpFileTime,
  _Out_ WORD *lpDosDate,
  _Out_ WORD *lpDosTime);

HRESULT WINAPI CoFileTimeNow(_Out_ FILETIME *lpFileTime);

_Check_return_
HRESULT
WINAPI
CoRegisterMessageFilter(
  _In_opt_ LPMESSAGEFILTER lpMessageFilter,
  _Outptr_opt_result_maybenull_ LPMESSAGEFILTER *lplpMessageFilter);

HRESULT
WINAPI
CoRegisterChannelHook(
  _In_ REFGUID ExtensionGuid,
  _In_ IChannelHook *pChannelHook);

typedef enum tagCOWAIT_FLAGS
{
    COWAIT_WAITALL   = 0x00000001,
    COWAIT_ALERTABLE = 0x00000002
} COWAIT_FLAGS;

_Check_return_
HRESULT
WINAPI
CoWaitForMultipleHandles(
  _In_ DWORD dwFlags,
  _In_ DWORD dwTimeout,
  _In_ ULONG cHandles,
  _In_reads_(cHandles) LPHANDLE pHandles,
  _Out_ LPDWORD lpdwindex);

/*****************************************************************************
 *	GUID API
 */

_Check_return_
HRESULT
WINAPI
StringFromCLSID(
  _In_ REFCLSID id,
  _Outptr_ LPOLESTR*);

_Check_return_
HRESULT
WINAPI
CLSIDFromString(
  _In_ LPCOLESTR,
  _Out_ LPCLSID);

_Check_return_
HRESULT
WINAPI
CLSIDFromProgID(
  _In_ LPCOLESTR progid,
  _Out_ LPCLSID riid);

_Check_return_
HRESULT
WINAPI
ProgIDFromCLSID(
  _In_ REFCLSID clsid,
  _Outptr_ LPOLESTR *lplpszProgID);

_Check_return_
INT
WINAPI
StringFromGUID2(
  _In_ REFGUID id,
  _Out_writes_to_(cmax, return) LPOLESTR str,
  _In_ INT cmax);

/*****************************************************************************
 *	COM Server dll - exports
 */

_Check_return_
HRESULT
WINAPI
DllGetClassObject(
  _In_ REFCLSID rclsid,
  _In_ REFIID riid,
  _Outptr_ LPVOID *ppv) DECLSPEC_HIDDEN;

HRESULT WINAPI DllCanUnloadNow(void) DECLSPEC_HIDDEN;

/* shouldn't be here, but is nice for type checking */
#ifdef __WINESRC__
HRESULT WINAPI DllRegisterServer(void) DECLSPEC_HIDDEN;
HRESULT WINAPI DllUnregisterServer(void) DECLSPEC_HIDDEN;
#endif


/*****************************************************************************
 *	Data Object
 */

HRESULT
WINAPI
CreateDataAdviseHolder(
  _Outptr_ LPDATAADVISEHOLDER *ppDAHolder);

HRESULT
WINAPI
CreateDataCache(
  _In_opt_ LPUNKNOWN pUnkOuter,
  _In_ REFCLSID rclsid,
  _In_ REFIID iid,
  _Out_ LPVOID *ppv);

/*****************************************************************************
 *	Moniker API
 */

_Check_return_
HRESULT
WINAPI
BindMoniker(
  _In_ LPMONIKER pmk,
  _In_ DWORD grfOpt,
  _In_ REFIID iidResult,
  _Outptr_ LPVOID *ppvResult);

_Check_return_
HRESULT
WINAPI
CoGetObject(
  _In_ LPCWSTR pszName,
  _In_opt_ BIND_OPTS *pBindOptions,
  _In_ REFIID riid,
  _Outptr_ void **ppv);

_Check_return_ HRESULT WINAPI CreateAntiMoniker(_Outptr_ LPMONIKER *ppmk);

_Check_return_
HRESULT
WINAPI
CreateBindCtx(
  _In_ DWORD reserved,
  _Outptr_ LPBC *ppbc);

_Check_return_
HRESULT
WINAPI
CreateClassMoniker(
  _In_ REFCLSID rclsid,
  _Outptr_ LPMONIKER *ppmk);

_Check_return_
HRESULT
WINAPI
CreateFileMoniker(
  _In_ LPCOLESTR lpszPathName,
  _Outptr_ LPMONIKER *ppmk);

_Check_return_
HRESULT
WINAPI
CreateGenericComposite(
  _In_opt_ LPMONIKER pmkFirst,
  _In_opt_ LPMONIKER pmkRest,
  _Outptr_ LPMONIKER *ppmkComposite);

_Check_return_
HRESULT
WINAPI
CreateItemMoniker(
  _In_ LPCOLESTR lpszDelim,
  _In_ LPCOLESTR lpszItem,
  _Outptr_ LPMONIKER *ppmk);

_Check_return_
HRESULT
WINAPI
CreateObjrefMoniker(
  _In_opt_ LPUNKNOWN punk,
  _Outptr_ LPMONIKER *ppmk);

_Check_return_
HRESULT
WINAPI
CreatePointerMoniker(
  _In_opt_ LPUNKNOWN punk,
  _Outptr_ LPMONIKER *ppmk);

_Check_return_
HRESULT
WINAPI
GetClassFile(
  _In_ LPCOLESTR filePathName,
  _Out_ CLSID *pclsid);

_Check_return_
HRESULT
WINAPI
GetRunningObjectTable(
  _In_ DWORD reserved,
  _Outptr_ LPRUNNINGOBJECTTABLE *pprot);

_Check_return_
HRESULT
WINAPI
MkParseDisplayName(
  _In_ LPBC pbc,
  _In_ LPCOLESTR szUserName,
  _Out_ ULONG *pchEaten,
  _Outptr_ LPMONIKER *ppmk);

_Check_return_
HRESULT
WINAPI
MonikerCommonPrefixWith(
  _In_ IMoniker *pmkThis,
  _In_ IMoniker *pmkOther,
  _Outptr_ IMoniker **ppmkCommon);

_Check_return_
HRESULT
WINAPI
MonikerRelativePathTo(
  _In_ LPMONIKER pmkSrc,
  _In_ LPMONIKER pmkDest,
  _Outptr_ LPMONIKER *ppmkRelPath,
  _In_ BOOL dwReserved);

/*****************************************************************************
 *	Storage API
 */
#define STGM_DIRECT		0x00000000
#define STGM_TRANSACTED		0x00010000
#define STGM_SIMPLE		0x08000000
#define STGM_READ		0x00000000
#define STGM_WRITE		0x00000001
#define STGM_READWRITE		0x00000002
#define STGM_SHARE_DENY_NONE	0x00000040
#define STGM_SHARE_DENY_READ	0x00000030
#define STGM_SHARE_DENY_WRITE	0x00000020
#define STGM_SHARE_EXCLUSIVE	0x00000010
#define STGM_PRIORITY		0x00040000
#define STGM_DELETEONRELEASE	0x04000000
#define STGM_CREATE		0x00001000
#define STGM_CONVERT		0x00020000
#define STGM_FAILIFTHERE	0x00000000
#define STGM_NOSCRATCH		0x00100000
#define STGM_NOSNAPSHOT		0x00200000
#define STGM_DIRECT_SWMR	0x00400000

#define STGFMT_STORAGE		0
#define STGFMT_FILE 		3
#define STGFMT_ANY 		4
#define STGFMT_DOCFILE 	5

typedef struct tagSTGOPTIONS
{
    USHORT usVersion;
    USHORT reserved;
    ULONG ulSectorSize;
    const WCHAR* pwcsTemplateFile;
} STGOPTIONS;

_Check_return_
HRESULT
WINAPI
StringFromIID(
  _In_ REFIID rclsid,
  _Outptr_ LPOLESTR *lplpsz);

_Check_return_
HRESULT
WINAPI
StgCreateDocfile(
  _In_opt_ _Null_terminated_ LPCOLESTR pwcsName,
  _In_ DWORD grfMode,
  _Reserved_ DWORD reserved,
  _Outptr_ IStorage **ppstgOpen);

_Check_return_
HRESULT
WINAPI
StgCreateStorageEx(
  _In_opt_ _Null_terminated_ const WCHAR*,
  _In_ DWORD,
  _In_ DWORD,
  _In_ DWORD,
  _Inout_opt_ STGOPTIONS*,
  _In_opt_ void*,
  _In_ REFIID,
  _Outptr_ void**);

_Check_return_
HRESULT
WINAPI
StgIsStorageFile(
  _In_ _Null_terminated_ LPCOLESTR fn);

_Check_return_
HRESULT
WINAPI
StgIsStorageILockBytes(
  _In_ ILockBytes *plkbyt);

_Check_return_
HRESULT
WINAPI
StgOpenStorage(
  _In_opt_ _Null_terminated_ const OLECHAR *pwcsName,
  _In_opt_ IStorage *pstgPriority,
  _In_ DWORD grfMode,
  _In_opt_z_ SNB snbExclude,
  _In_ DWORD reserved,
  _Outptr_ IStorage **ppstgOpen);

_Check_return_
HRESULT
WINAPI
StgOpenStorageEx(
  _In_ _Null_terminated_ const WCHAR *pwcwName,
  _In_ DWORD grfMode,
  _In_ DWORD stgfmt,
  _In_ DWORD grfAttrs,
  _Inout_opt_ STGOPTIONS *pStgOptions,
  _In_opt_ void *reserved,
  _In_ REFIID riid,
  _Outptr_ void **ppObjectOpen);

_Check_return_
HRESULT
WINAPI
StgCreateDocfileOnILockBytes(
  _In_ ILockBytes *plkbyt,
  _In_ DWORD grfMode,
  _In_ DWORD reserved,
  _Outptr_ IStorage **ppstgOpen);

_Check_return_
HRESULT
WINAPI
StgOpenStorageOnILockBytes(
  _In_ ILockBytes *plkbyt,
  _In_opt_ IStorage *pstgPriority,
  _In_ DWORD grfMode,
  _In_opt_z_ SNB snbExclude,
  _Reserved_ DWORD reserved,
  _Outptr_ IStorage **ppstgOpen);

_Check_return_
HRESULT
WINAPI
StgSetTimes(
  _In_ _Null_terminated_ OLECHAR const *lpszName,
  _In_opt_ FILETIME const *pctime,
  _In_opt_ FILETIME const *patime,
  _In_opt_ FILETIME const *pmtime);

#ifdef __cplusplus
}
#endif

#ifndef __WINESRC__
# include <urlmon.h>
#endif
#include <propidl.h>

#ifndef __WINESRC__

#define FARSTRUCT
#define HUGEP

#define WINOLEAPI        STDAPI
#define WINOLEAPI_(type) STDAPI_(type)

#endif /* __WINESRC__ */

#endif /* _OBJBASE_H_ */
