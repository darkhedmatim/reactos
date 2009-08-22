/*
 * Marshaling Tests
 *
 * Copyright 2004 Robert Shearman
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "propidl.h" /* for LPSAFEARRAY_User* routines */

#include "wine/test.h"

#if (__STDC__ && !defined(_FORCENAMELESSUNION)) || defined(NONAMELESSUNION)
# define V_U2(A)  ((A)->n1.n2)
#else
# define V_U2(A)  (*(A))
#endif

#define LPSAFEARRAY_UNMARSHAL_WORKS 1
#define BSTR_UNMARSHAL_WORKS 1
#define VARIANT_UNMARSHAL_WORKS 1

static inline SF_TYPE get_union_type(SAFEARRAY *psa)
{
    VARTYPE vt;
    HRESULT hr;

    hr = SafeArrayGetVartype(psa, &vt);
    if (FAILED(hr))
    {
        if(psa->fFeatures & FADF_VARIANT) return SF_VARIANT;

        switch(psa->cbElements)
        {
        case 1: vt = VT_I1; break;
        case 2: vt = VT_I2; break;
        case 4: vt = VT_I4; break;
        case 8: vt = VT_I8; break;
        default: return 0;
        }
    }

    if (psa->fFeatures & FADF_HAVEIID)
        return SF_HAVEIID;

    switch (vt)
    {
    case VT_I1:
    case VT_UI1:      return SF_I1;
    case VT_BOOL:
    case VT_I2:
    case VT_UI2:      return SF_I2;
    case VT_INT:
    case VT_UINT:
    case VT_I4:
    case VT_UI4:
    case VT_R4:       return SF_I4;
    case VT_DATE:
    case VT_CY:
    case VT_R8:
    case VT_I8:
    case VT_UI8:      return SF_I8;
    case VT_INT_PTR:
    case VT_UINT_PTR: return (sizeof(UINT_PTR) == 4 ? SF_I4 : SF_I8);
    case VT_BSTR:     return SF_BSTR;
    case VT_DISPATCH: return SF_DISPATCH;
    case VT_VARIANT:  return SF_VARIANT;
    case VT_UNKNOWN:  return SF_UNKNOWN;
    /* Note: Return a non-zero size to indicate vt is valid. The actual size
     * of a UDT is taken from the result of IRecordInfo_GetSize().
     */
    case VT_RECORD:   return SF_RECORD;
    default:          return SF_ERROR;
    }
}

static ULONG get_cell_count(const SAFEARRAY *psa)
{
    const SAFEARRAYBOUND* psab = psa->rgsabound;
    USHORT cCount = psa->cDims;
    ULONG ulNumCells = 1;

    while (cCount--)
    {
         if (!psab->cElements)
            return 0;
        ulNumCells *= psab->cElements;
        psab++;
    }
    return ulNumCells;
}

static void check_safearray(void *buffer, LPSAFEARRAY lpsa)
{
    unsigned char *wiresa = buffer;
    VARTYPE vt;
    SF_TYPE sftype;
    ULONG cell_count;

    if(!lpsa)
    {
        ok(*(DWORD *)wiresa == 0, "wiresa + 0x0 should be NULL instead of 0x%08x\n", *(DWORD *)wiresa);
        return;
    }

    if(FAILED(SafeArrayGetVartype(lpsa, &vt)))
        vt = 0;

    sftype = get_union_type(lpsa);
    cell_count = get_cell_count(lpsa);

    ok(*(DWORD *)wiresa, "wiresa + 0x0 should be non-NULL instead of 0x%08x\n", *(DWORD *)wiresa); /* win2k: this is lpsa. winxp: this is 0x00000001 */
    wiresa += sizeof(DWORD);
    ok(*(DWORD *)wiresa == lpsa->cDims, "wiresa + 0x4 should be lpsa->cDims instead of 0x%08x\n", *(DWORD *)wiresa);
    wiresa += sizeof(DWORD);
    ok(*(WORD *)wiresa == lpsa->cDims, "wiresa + 0x8 should be lpsa->cDims instead of 0x%04x\n", *(WORD *)wiresa);
    wiresa += sizeof(WORD);
    ok(*(WORD *)wiresa == lpsa->fFeatures, "wiresa + 0xa should be lpsa->fFeatures instead of 0x%08x\n", *(WORD *)wiresa);
    wiresa += sizeof(WORD);
    ok(*(DWORD *)wiresa == lpsa->cbElements, "wiresa + 0xc should be lpsa->cbElements instead of 0x%08x\n", *(DWORD *)wiresa);
    wiresa += sizeof(DWORD);
    ok(*(WORD *)wiresa == lpsa->cLocks, "wiresa + 0x10 should be lpsa->cLocks instead of 0x%04x\n", *(WORD *)wiresa);
    wiresa += sizeof(WORD);
    ok(*(WORD *)wiresa == vt, "wiresa + 0x12 should be %04x instead of 0x%04x\n", vt, *(WORD *)wiresa);
    wiresa += sizeof(WORD);
    ok(*(DWORD *)wiresa == sftype, "wiresa + 0x14 should be %08x instead of 0x%08x\n", (DWORD)sftype, *(DWORD *)wiresa);
    wiresa += sizeof(DWORD);
    ok(*(DWORD *)wiresa == cell_count, "wiresa + 0x18 should be %u instead of %u\n", cell_count, *(DWORD *)wiresa);
    wiresa += sizeof(DWORD);
    ok(*(DWORD *)wiresa, "wiresa + 0x1c should be non-zero instead of 0x%08x\n", *(DWORD *)wiresa);
    wiresa += sizeof(DWORD);
    if(sftype == SF_HAVEIID)
    {
        GUID guid;
        SafeArrayGetIID(lpsa, &guid);
        ok(IsEqualGUID(&guid, wiresa), "guid mismatch\n");
        wiresa += sizeof(GUID);
    }
    ok(!memcmp(wiresa, lpsa->rgsabound, sizeof(lpsa->rgsabound[0]) * lpsa->cDims), "bounds mismatch\n");
    wiresa += sizeof(lpsa->rgsabound[0]) * lpsa->cDims;

    ok(*(DWORD *)wiresa == cell_count, "wiresa + 0x28 should be %u instead of %u\n", cell_count, *(DWORD*)wiresa);
    wiresa += sizeof(DWORD);
    /* elements are now pointed to by wiresa */
}

static void * WINAPI user_allocate(SIZE_T size)
{
    return CoTaskMemAlloc(size);
}

static void WINAPI user_free(void *p)
{
    CoTaskMemFree(p);
}

static void init_user_marshal_cb(USER_MARSHAL_CB *umcb,
                                 PMIDL_STUB_MESSAGE stub_msg,
                                 PRPC_MESSAGE rpc_msg, unsigned char *buffer,
                                 unsigned int size, MSHCTX context)
{
    memset(rpc_msg, 0, sizeof(*rpc_msg));
    rpc_msg->Buffer = buffer;
    rpc_msg->BufferLength = size;

    memset(stub_msg, 0, sizeof(*stub_msg));
    stub_msg->RpcMsg = rpc_msg;
    stub_msg->Buffer = buffer;
    stub_msg->pfnAllocate = user_allocate;
    stub_msg->pfnFree = user_free;

    memset(umcb, 0, sizeof(*umcb));
    umcb->Flags = MAKELONG(context, NDR_LOCAL_DATA_REPRESENTATION);
    umcb->pStubMsg = stub_msg;
    umcb->Signature = USER_MARSHAL_CB_SIGNATURE;
    umcb->CBType = buffer ? USER_MARSHAL_CB_UNMARSHALL : USER_MARSHAL_CB_BUFFER_SIZE;
}

static void test_marshal_LPSAFEARRAY(void)
{
    unsigned char *buffer;
    ULONG size, expected;
    LPSAFEARRAY lpsa;
    LPSAFEARRAY lpsa2 = NULL;
    SAFEARRAYBOUND sab;
    RPC_MESSAGE rpc_msg;
    MIDL_STUB_MESSAGE stub_msg;
    USER_MARSHAL_CB umcb;
    HRESULT hr;
    VARTYPE vt;

    sab.lLbound = 5;
    sab.cElements = 10;

    lpsa = SafeArrayCreate(VT_I2, 1, &sab);
    *(DWORD *)lpsa->pvData = 0xcafebabe;

    lpsa->cLocks = 7;
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
    size = LPSAFEARRAY_UserSize(&umcb.Flags, 1, &lpsa);
    expected = (44 + 1 + sizeof(ULONG) - 1) & ~(sizeof(ULONG) - 1);
    expected += sab.cElements * sizeof(USHORT);
    ok(size == expected || size == expected + 12, /* win64 */
       "size should be %u bytes, not %u\n", expected, size);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
    size = LPSAFEARRAY_UserSize(&umcb.Flags, 0, &lpsa);
    expected = 44 + sab.cElements * sizeof(USHORT);
    ok(size == expected || size == expected + 12, /* win64 */
       "size should be %u bytes, not %u\n", expected, size);
    buffer = HeapAlloc(GetProcessHeap(), 0, size);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_DIFFERENTMACHINE);
    LPSAFEARRAY_UserMarshal(&umcb.Flags, buffer, &lpsa);

    check_safearray(buffer, lpsa);

    if (LPSAFEARRAY_UNMARSHAL_WORKS)
    {
        VARTYPE vt, vt2;
        init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_DIFFERENTMACHINE);
        LPSAFEARRAY_UserUnmarshal(&umcb.Flags, buffer, &lpsa2);
        ok(lpsa2 != NULL, "LPSAFEARRAY didn't unmarshal\n");
        SafeArrayGetVartype(lpsa, &vt);
        SafeArrayGetVartype(lpsa2, &vt2);
        ok(vt == vt2, "vts differ %x %x\n", vt, vt2);
        init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
        LPSAFEARRAY_UserFree(&umcb.Flags, &lpsa2);
    }
    HeapFree(GetProcessHeap(), 0, buffer);
    lpsa->cLocks = 0;
    SafeArrayDestroy(lpsa);

    /* test NULL safe array */
    lpsa = NULL;

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
    size = LPSAFEARRAY_UserSize(&umcb.Flags, 0, &lpsa);
    ok(size == 4, "size should be 4 bytes, not %d\n", size);
    buffer = HeapAlloc(GetProcessHeap(), 0, size);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_DIFFERENTMACHINE);
    LPSAFEARRAY_UserMarshal(&umcb.Flags, buffer, &lpsa);
    check_safearray(buffer, lpsa);

    if (LPSAFEARRAY_UNMARSHAL_WORKS)
    {
        init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_DIFFERENTMACHINE);
        LPSAFEARRAY_UserUnmarshal(&umcb.Flags, buffer, &lpsa2);
        ok(lpsa2 == NULL, "NULL LPSAFEARRAY didn't unmarshal\n");
        init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
        LPSAFEARRAY_UserFree(&umcb.Flags, &lpsa2);
    }
    HeapFree(GetProcessHeap(), 0, buffer);

    sab.lLbound = 5;
    sab.cElements = 10;

    lpsa = SafeArrayCreate(VT_R8, 1, &sab);
    *(double *)lpsa->pvData = 3.1415;

    lpsa->cLocks = 7;
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
    size = LPSAFEARRAY_UserSize(&umcb.Flags, 1, &lpsa);
    expected = (44 + 1 + (sizeof(double) - 1)) & ~(sizeof(double) - 1);
    expected += sab.cElements * sizeof(double);
    ok(size == expected || size == expected + 16, /* win64 */
       "size should be %u bytes, not %u\n", expected, size);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
    expected = (44 + (sizeof(double) - 1)) & ~(sizeof(double) - 1);
    expected += sab.cElements * sizeof(double);
    size = LPSAFEARRAY_UserSize(&umcb.Flags, 0, &lpsa);
    ok(size == expected || size == expected + 8, /* win64 */
       "size should be %u bytes, not %u\n", expected, size);
    buffer = HeapAlloc(GetProcessHeap(), 0, size);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_DIFFERENTMACHINE);
    LPSAFEARRAY_UserMarshal(&umcb.Flags, buffer, &lpsa);

    check_safearray(buffer, lpsa);

    HeapFree(GetProcessHeap(), 0, buffer);
    lpsa->cLocks = 0;
    SafeArrayDestroy(lpsa);

    /* VARTYPE-less arrays can be marshaled if cbElements is 1,2,4 or 8 as type SF_In */
    hr = SafeArrayAllocDescriptor(1, &lpsa);
    ok(hr == S_OK, "saad failed %08x\n", hr);
    lpsa->cbElements = 8;
    lpsa->rgsabound[0].lLbound = 2;
    lpsa->rgsabound[0].cElements = 48;
    hr = SafeArrayAllocData(lpsa);
    ok(hr == S_OK, "saad failed %08x\n", hr);

    hr = SafeArrayGetVartype(lpsa, &vt);
    ok(hr == E_INVALIDARG, "ret %08x\n", hr);

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
    size = LPSAFEARRAY_UserSize(&umcb.Flags, 0, &lpsa);
    expected = (44 + lpsa->cbElements - 1) & ~(lpsa->cbElements - 1);
    expected += lpsa->cbElements * lpsa->rgsabound[0].cElements;
    ok(size == expected || size == expected + 8,  /* win64 */
       "size should be %u bytes, not %u\n", expected, size);
    buffer = HeapAlloc(GetProcessHeap(), 0, size);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_DIFFERENTMACHINE);
    LPSAFEARRAY_UserMarshal(&umcb.Flags, buffer, &lpsa);
    check_safearray(buffer, lpsa);
    HeapFree(GetProcessHeap(), 0, buffer);
    SafeArrayDestroyData(lpsa);
    SafeArrayDestroyDescriptor(lpsa);

    /* VARTYPE-less arrays with FADF_VARIANT */
    hr = SafeArrayAllocDescriptor(1, &lpsa);
    ok(hr == S_OK, "saad failed %08x\n", hr);
    lpsa->cbElements = sizeof(VARIANT);
    lpsa->fFeatures = FADF_VARIANT;
    lpsa->rgsabound[0].lLbound = 2;
    lpsa->rgsabound[0].cElements = 48;
    hr = SafeArrayAllocData(lpsa);
    ok(hr == S_OK, "saad failed %08x\n", hr);

    hr = SafeArrayGetVartype(lpsa, &vt);
    ok(hr == E_INVALIDARG, "ret %08x\n", hr);

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
    size = LPSAFEARRAY_UserSize(&umcb.Flags, 0, &lpsa);
    expected = 44 + 28 * lpsa->rgsabound[0].cElements;
    todo_wine
    ok(size == expected || size == expected + 8,  /* win64 */
       "size should be %u bytes, not %u\n", expected, size);
    buffer = HeapAlloc(GetProcessHeap(), 0, size);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_DIFFERENTMACHINE);
    LPSAFEARRAY_UserMarshal(&umcb.Flags, buffer, &lpsa);
    lpsa->cbElements = 16;  /* VARIANT wire size */
    check_safearray(buffer, lpsa);
    HeapFree(GetProcessHeap(), 0, buffer);
    SafeArrayDestroyData(lpsa);
    SafeArrayDestroyDescriptor(lpsa);
}

static void check_bstr(void *buffer, BSTR b)
{
    DWORD *wireb = buffer;
    DWORD len = SysStringByteLen(b);

    ok(*wireb == (len + 1) / 2, "wv[0] %08x\n", *wireb);
    wireb++;
    if(b)
        ok(*wireb == len, "wv[1] %08x\n", *wireb);
    else
        ok(*wireb == 0xffffffff, "wv[1] %08x\n", *wireb);
    wireb++;
    ok(*wireb == (len + 1) / 2, "wv[2] %08x\n", *wireb);
    if(len)
    {
        wireb++;
        ok(!memcmp(wireb, b, (len + 1) & ~1), "strings differ\n");
    }
    return;
}

static void test_marshal_BSTR(void)
{
    ULONG size;
    RPC_MESSAGE rpc_msg;
    MIDL_STUB_MESSAGE stub_msg;
    USER_MARSHAL_CB umcb;
    unsigned char *buffer, *next;
    BSTR b, b2;
    WCHAR str[] = {'m','a','r','s','h','a','l',' ','t','e','s','t','1',0};
    DWORD len;

    b = SysAllocString(str);
    len = SysStringLen(b);
    ok(len == 13, "get %d\n", len);

    /* BSTRs are DWORD aligned */

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
    size = BSTR_UserSize(&umcb.Flags, 1, &b);
    ok(size == 42, "size %d\n", size);

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
    size = BSTR_UserSize(&umcb.Flags, 0, &b);
    ok(size == 38, "size %d\n", size);

    buffer = HeapAlloc(GetProcessHeap(), 0, size);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_DIFFERENTMACHINE);
    next = BSTR_UserMarshal(&umcb.Flags, buffer, &b);
    ok(next == buffer + size, "got %p expect %p\n", next, buffer + size);
    check_bstr(buffer, b);

    if (BSTR_UNMARSHAL_WORKS)
    {
        b2 = NULL;
        init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_DIFFERENTMACHINE);
        next = BSTR_UserUnmarshal(&umcb.Flags, buffer, &b2);
        ok(next == buffer + size, "got %p expect %p\n", next, buffer + size);
        ok(b2 != NULL, "BSTR didn't unmarshal\n");
        ok(!memcmp(b, b2, (len + 1) * 2), "strings differ\n");
        init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
        BSTR_UserFree(&umcb.Flags, &b2);
    }

    HeapFree(GetProcessHeap(), 0, buffer);
    SysFreeString(b);

    b = NULL;
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
    size = BSTR_UserSize(&umcb.Flags, 0, &b);
    ok(size == 12, "size %d\n", size);

    buffer = HeapAlloc(GetProcessHeap(), 0, size);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_DIFFERENTMACHINE);
    next = BSTR_UserMarshal(&umcb.Flags, buffer, &b);
    ok(next == buffer + size, "got %p expect %p\n", next, buffer + size);

    check_bstr(buffer, b);
    if (BSTR_UNMARSHAL_WORKS)
    {
        b2 = NULL;
        init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_DIFFERENTMACHINE);
        next = BSTR_UserUnmarshal(&umcb.Flags, buffer, &b2);
        ok(next == buffer + size, "got %p expect %p\n", next, buffer + size);
        ok(b2 == NULL, "NULL BSTR didn't unmarshal\n");
        init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
        BSTR_UserFree(&umcb.Flags, &b2);
    }
    HeapFree(GetProcessHeap(), 0, buffer);

    b = SysAllocStringByteLen("abc", 3);
    *(((char*)b) + 3) = 'd';
    len = SysStringLen(b);
    ok(len == 1, "get %d\n", len);
    len = SysStringByteLen(b);
    ok(len == 3, "get %d\n", len);

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
    size = BSTR_UserSize(&umcb.Flags, 0, &b);
    ok(size == 16, "size %d\n", size);

    buffer = HeapAlloc(GetProcessHeap(), 0, size);
    memset(buffer, 0xcc, size);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_DIFFERENTMACHINE);
    next = BSTR_UserMarshal(&umcb.Flags, buffer, &b);
    ok(next == buffer + size, "got %p expect %p\n", next, buffer + size);
    check_bstr(buffer, b);
    ok(buffer[15] == 'd', "buffer[15] %02x\n", buffer[15]);

    if (BSTR_UNMARSHAL_WORKS)
    {
        b2 = NULL;
        init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_DIFFERENTMACHINE);
        next = BSTR_UserUnmarshal(&umcb.Flags, buffer, &b2);
        ok(next == buffer + size, "got %p expect %p\n", next, buffer + size);
        ok(b2 != NULL, "BSTR didn't unmarshal\n");
        ok(!memcmp(b, b2, len), "strings differ\n");
        init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
        BSTR_UserFree(&umcb.Flags, &b2);
    }
    HeapFree(GetProcessHeap(), 0, buffer);
    SysFreeString(b);

    b = SysAllocStringByteLen("", 0);
    len = SysStringLen(b);
    ok(len == 0, "get %d\n", len);
    len = SysStringByteLen(b);
    ok(len == 0, "get %d\n", len);

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
    size = BSTR_UserSize(&umcb.Flags, 0, &b);
    ok(size == 12, "size %d\n", size);

    buffer = HeapAlloc(GetProcessHeap(), 0, size);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_DIFFERENTMACHINE);
    next = BSTR_UserMarshal(&umcb.Flags, buffer, &b);
    ok(next == buffer + size, "got %p expect %p\n", next, buffer + size);
    check_bstr(buffer, b);

    if (BSTR_UNMARSHAL_WORKS)
    {
        b2 = NULL;
        init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_DIFFERENTMACHINE);
        next = BSTR_UserUnmarshal(&umcb.Flags, buffer, &b2);
        ok(next == buffer + size, "got %p expect %p\n", next, buffer + size);
        ok(b2 != NULL, "NULL LPSAFEARRAY didn't unmarshal\n");
        len = SysStringByteLen(b2);
        ok(len == 0, "byte len %d\n", len);
        init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
        BSTR_UserFree(&umcb.Flags, &b2);
    }
    HeapFree(GetProcessHeap(), 0, buffer);
    SysFreeString(b);
}

typedef struct
{
    const IUnknownVtbl *lpVtbl;
    ULONG refs;
} HeapUnknown;

static HRESULT WINAPI HeapUnknown_QueryInterface(IUnknown *iface, REFIID riid, void **ppv)
{
    if (IsEqualIID(riid, &IID_IUnknown))
    {
        IUnknown_AddRef(iface);
        *ppv = iface;
        return S_OK;
    }
    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI HeapUnknown_AddRef(IUnknown *iface)
{
    HeapUnknown *This = (HeapUnknown *)iface;
    return InterlockedIncrement((LONG*)&This->refs);
}

static ULONG WINAPI HeapUnknown_Release(IUnknown *iface)
{
    HeapUnknown *This = (HeapUnknown *)iface;
    ULONG refs = InterlockedDecrement((LONG*)&This->refs);
    if (!refs) HeapFree(GetProcessHeap(), 0, This);
    return refs;
}

static const IUnknownVtbl HeapUnknown_Vtbl =
{
    HeapUnknown_QueryInterface,
    HeapUnknown_AddRef,
    HeapUnknown_Release
};

static void check_variant_header(DWORD *wirev, VARIANT *v, ULONG size)
{
    WORD *wp;
    DWORD switch_is;

    ok(*wirev == (size + 7) >> 3, "wv[0] %08x, expected %08x\n", *wirev, (size + 7) >> 3);
    wirev++;
    ok(*wirev == 0, "wv[1] %08x\n", *wirev);
    wirev++;
    wp = (WORD*)wirev;
    ok(*wp == V_VT(v), "vt %04x expected %04x\n", *wp, V_VT(v));
    wp++;
    ok(*wp == V_U2(v).wReserved1, "res1 %04x expected %04x\n", *wp, V_U2(v).wReserved1);
    wp++;
    ok(*wp == V_U2(v).wReserved2, "res2 %04x expected %04x\n", *wp, V_U2(v).wReserved2);
    wp++;
    ok(*wp == V_U2(v).wReserved3, "res3 %04x expected %04x\n", *wp, V_U2(v).wReserved3);
    wp++;
    wirev = (DWORD*)wp;
    switch_is = V_VT(v);
    if(switch_is & VT_ARRAY)
        switch_is &= ~VT_TYPEMASK;
    ok(*wirev == switch_is, "switch_is %08x expected %08x\n", *wirev, switch_is);
}

/* Win9x and WinME don't always align as needed. Variants have
 * an alignment of 8.
 */
static void *alloc_aligned(SIZE_T size, void **buf)
{
    *buf = HeapAlloc(GetProcessHeap(), 0, size + 7);
    return (void *)(((UINT_PTR)*buf + 7) & ~7);
}

static void test_marshal_VARIANT(void)
{
    VARIANT v, v2;
    MIDL_STUB_MESSAGE stubMsg = { 0 };
    RPC_MESSAGE rpcMsg = { 0 };
    USER_MARSHAL_CB umcb = { 0 };
    unsigned char *buffer, *next;
    void *oldbuffer;
    ULONG ul;
    short s;
    double d;
    DWORD *wirev;
    BSTR b;
    WCHAR str[] = {'m','a','r','s','h','a','l',' ','t','e','s','t',0};
    SAFEARRAYBOUND sab;
    LPSAFEARRAY lpsa;
    DECIMAL dec, dec2;
    HeapUnknown *heap_unknown;
    DWORD expected;

    stubMsg.RpcMsg = &rpcMsg;

    umcb.Flags = MAKELONG(MSHCTX_DIFFERENTMACHINE, NDR_LOCAL_DATA_REPRESENTATION);
    umcb.pStubMsg = &stubMsg;
    umcb.pReserve = NULL;
    umcb.Signature = USER_MARSHAL_CB_SIGNATURE;
    umcb.CBType = USER_MARSHAL_CB_UNMARSHALL;

    /*** I1 ***/
    VariantInit(&v);
    V_VT(&v) = VT_I1;
    V_I1(&v) = 0x12;

    /* check_variant_header tests wReserved[123], so initialize to unique values.
     * (Could probably also do this by setting the variant to a known DECIMAL.)
     */
    V_U2(&v).wReserved1 = 0x1234;
    V_U2(&v).wReserved2 = 0x5678;
    V_U2(&v).wReserved3 = 0x9abc;

    /* Variants have an alignment of 8 */
    rpcMsg.BufferLength = stubMsg.BufferLength = VARIANT_UserSize(&umcb.Flags, 1, &v);
    ok(stubMsg.BufferLength == 29, "size %d\n", stubMsg.BufferLength);

    rpcMsg.BufferLength = stubMsg.BufferLength = VARIANT_UserSize(&umcb.Flags, 0, &v);
    ok(stubMsg.BufferLength == 21, "size %d\n", stubMsg.BufferLength);

    buffer = rpcMsg.Buffer = stubMsg.Buffer = stubMsg.BufferStart = alloc_aligned(stubMsg.BufferLength, &oldbuffer);
    stubMsg.BufferEnd = stubMsg.Buffer + stubMsg.BufferLength;
    next = VARIANT_UserMarshal(&umcb.Flags, buffer, &v);
    ok(next == buffer + stubMsg.BufferLength, "got %p expect %p\n", next, buffer + stubMsg.BufferLength);
    wirev = (DWORD*)buffer;
    
    check_variant_header(wirev, &v, stubMsg.BufferLength);
    wirev += 5;
    ok(*(char*)wirev == V_I1(&v), "wv[5] %08x\n", *wirev);
    if (VARIANT_UNMARSHAL_WORKS)
    {
        VariantInit(&v2);
        stubMsg.Buffer = buffer;
        next = VARIANT_UserUnmarshal(&umcb.Flags, buffer, &v2);
        ok(next == buffer + stubMsg.BufferLength, "got %p expect %p\n", next, buffer + stubMsg.BufferLength);
        ok(V_VT(&v) == V_VT(&v2), "got vt %d expect %d\n", V_VT(&v), V_VT(&v2));
        ok(V_I1(&v) == V_I1(&v2), "got i1 %x expect %x\n", V_I1(&v), V_I1(&v2));

        VARIANT_UserFree(&umcb.Flags, &v2);
    }
    HeapFree(GetProcessHeap(), 0, oldbuffer);

    /*** I2 ***/
    VariantInit(&v);
    V_VT(&v) = VT_I2;
    V_I2(&v) = 0x1234;

    rpcMsg.BufferLength = stubMsg.BufferLength = VARIANT_UserSize(&umcb.Flags, 0, &v);
    ok(stubMsg.BufferLength == 22, "size %d\n", stubMsg.BufferLength);

    buffer = rpcMsg.Buffer = stubMsg.Buffer = stubMsg.BufferStart = alloc_aligned(stubMsg.BufferLength, &oldbuffer);
    stubMsg.BufferEnd = stubMsg.Buffer + stubMsg.BufferLength;
    next = VARIANT_UserMarshal(&umcb.Flags, buffer, &v);
    ok(next == buffer + stubMsg.BufferLength, "got %p expect %p\n", next, buffer + stubMsg.BufferLength);
    wirev = (DWORD*)buffer;

    check_variant_header(wirev, &v, stubMsg.BufferLength);
    wirev += 5;
    ok(*(short*)wirev == V_I2(&v), "wv[5] %08x\n", *wirev);
    if (VARIANT_UNMARSHAL_WORKS)
    {
        VariantInit(&v2);
        stubMsg.Buffer = buffer;
        next = VARIANT_UserUnmarshal(&umcb.Flags, buffer, &v2);
        ok(next == buffer + stubMsg.BufferLength, "got %p expect %p\n", next, buffer + stubMsg.BufferLength);
        ok(V_VT(&v) == V_VT(&v2), "got vt %d expect %d\n", V_VT(&v), V_VT(&v2));
        ok(V_I2(&v) == V_I2(&v2), "got i2 %x expect %x\n", V_I2(&v), V_I2(&v2));

        VARIANT_UserFree(&umcb.Flags, &v2);
    }
    HeapFree(GetProcessHeap(), 0, oldbuffer);

    /*** I2 BYREF ***/
    VariantInit(&v);
    V_VT(&v) = VT_I2 | VT_BYREF;
    s = 0x1234;
    V_I2REF(&v) = &s;

    rpcMsg.BufferLength = stubMsg.BufferLength = VARIANT_UserSize(&umcb.Flags, 0, &v);
    ok(stubMsg.BufferLength == 26, "size %d\n", stubMsg.BufferLength);

    buffer = rpcMsg.Buffer = stubMsg.Buffer = stubMsg.BufferStart = alloc_aligned(stubMsg.BufferLength, &oldbuffer);
    stubMsg.BufferEnd = stubMsg.Buffer + stubMsg.BufferLength;
    next = VARIANT_UserMarshal(&umcb.Flags, buffer, &v);
    ok(next == buffer + stubMsg.BufferLength, "got %p expect %p\n", next, buffer + stubMsg.BufferLength);
    wirev = (DWORD*)buffer;

    check_variant_header(wirev, &v, stubMsg.BufferLength);
    wirev += 5;
    ok(*wirev == 0x4, "wv[5] %08x\n", *wirev);
    wirev++;
    ok(*(short*)wirev == s, "wv[6] %08x\n", *wirev);
    if (VARIANT_UNMARSHAL_WORKS)
    {
        void *mem;
        VariantInit(&v2);
        V_VT(&v2) = VT_I2 | VT_BYREF;
        V_BYREF(&v2) = mem = CoTaskMemAlloc(sizeof(V_I2(&v2)));
        stubMsg.Buffer = buffer;
        next = VARIANT_UserUnmarshal(&umcb.Flags, buffer, &v2);
        ok(next == buffer + stubMsg.BufferLength, "got %p expect %p\n", next, buffer + stubMsg.BufferLength);
        ok(V_VT(&v) == V_VT(&v2), "got vt %d expect %d\n", V_VT(&v), V_VT(&v2));
        ok(V_BYREF(&v2) == mem, "didn't reuse existing memory\n");
        ok(*V_I2REF(&v) == *V_I2REF(&v2), "got i2 ref %x expect ui4 ref %x\n", *V_I2REF(&v), *V_I2REF(&v2));

        VARIANT_UserFree(&umcb.Flags, &v2);
    }
    HeapFree(GetProcessHeap(), 0, oldbuffer);

    /*** I4 ***/
    VariantInit(&v);
    V_VT(&v) = VT_I4;
    V_I4(&v) = 0x1234;

    rpcMsg.BufferLength = stubMsg.BufferLength = VARIANT_UserSize(&umcb.Flags, 0, &v);
    ok(stubMsg.BufferLength == 24, "size %d\n", stubMsg.BufferLength);

    buffer = rpcMsg.Buffer = stubMsg.Buffer = stubMsg.BufferStart = alloc_aligned(stubMsg.BufferLength, &oldbuffer);
    stubMsg.BufferEnd = stubMsg.Buffer + stubMsg.BufferLength;
    next = VARIANT_UserMarshal(&umcb.Flags, buffer, &v);
    ok(next == buffer + stubMsg.BufferLength, "got %p expect %p\n", next, buffer + stubMsg.BufferLength);
    wirev = (DWORD*)buffer;
    
    check_variant_header(wirev, &v, stubMsg.BufferLength);
    wirev += 5;
    ok(*wirev == V_I4(&v), "wv[5] %08x\n", *wirev);

    if (VARIANT_UNMARSHAL_WORKS)
    {
        VariantInit(&v2);
        stubMsg.Buffer = buffer;
        next = VARIANT_UserUnmarshal(&umcb.Flags, buffer, &v2);
        ok(next == buffer + stubMsg.BufferLength, "got %p expect %p\n", next, buffer + stubMsg.BufferLength);
        ok(V_VT(&v) == V_VT(&v2), "got vt %d expect %d\n", V_VT(&v), V_VT(&v2));
        ok(V_I4(&v) == V_I4(&v2), "got i4 %x expect %x\n", V_I4(&v), V_I4(&v2));

        VARIANT_UserFree(&umcb.Flags, &v2);
    }
    HeapFree(GetProcessHeap(), 0, oldbuffer);

    /*** UI4 ***/
    VariantInit(&v);
    V_VT(&v) = VT_UI4;
    V_UI4(&v) = 0x1234;

    rpcMsg.BufferLength = stubMsg.BufferLength = VARIANT_UserSize(&umcb.Flags, 0, &v);
    ok(stubMsg.BufferLength == 24, "size %d\n", stubMsg.BufferLength);

    buffer = rpcMsg.Buffer = stubMsg.Buffer = stubMsg.BufferStart = alloc_aligned(stubMsg.BufferLength, &oldbuffer);
    stubMsg.BufferEnd = stubMsg.Buffer + stubMsg.BufferLength;
    next = VARIANT_UserMarshal(&umcb.Flags, buffer, &v);
    ok(next == buffer + stubMsg.BufferLength, "got %p expect %p\n", next, buffer + stubMsg.BufferLength);
    wirev = (DWORD*)buffer;
    
    check_variant_header(wirev, &v, stubMsg.BufferLength);
    wirev += 5;
    ok(*wirev == 0x1234, "wv[5] %08x\n", *wirev);
    if (VARIANT_UNMARSHAL_WORKS)
    {
        VariantInit(&v2);
        stubMsg.Buffer = buffer;
        next = VARIANT_UserUnmarshal(&umcb.Flags, buffer, &v2);
        ok(next == buffer + stubMsg.BufferLength, "got %p expect %p\n", next, buffer + stubMsg.BufferLength);
        ok(V_VT(&v) == V_VT(&v2), "got vt %d expect %d\n", V_VT(&v), V_VT(&v2));
        ok(V_UI4(&v) == V_UI4(&v2), "got ui4 %x expect %x\n", V_UI4(&v), V_UI4(&v2));

        VARIANT_UserFree(&umcb.Flags, &v2);
    }
    HeapFree(GetProcessHeap(), 0, oldbuffer);

    /*** UI4 BYREF ***/
    VariantInit(&v);
    V_VT(&v) = VT_UI4 | VT_BYREF;
    ul = 0x1234;
    V_UI4REF(&v) = &ul;

    rpcMsg.BufferLength = stubMsg.BufferLength = VARIANT_UserSize(&umcb.Flags, 0, &v);
    ok(stubMsg.BufferLength == 28, "size %d\n", stubMsg.BufferLength);

    buffer = rpcMsg.Buffer = stubMsg.Buffer = stubMsg.BufferStart = alloc_aligned(stubMsg.BufferLength, &oldbuffer);
    stubMsg.BufferEnd = stubMsg.Buffer + stubMsg.BufferLength;
    next = VARIANT_UserMarshal(&umcb.Flags, buffer, &v);
    ok(next == buffer + stubMsg.BufferLength, "got %p expect %p\n", next, buffer + stubMsg.BufferLength);
    wirev = (DWORD*)buffer;
    
    check_variant_header(wirev, &v, stubMsg.BufferLength);
    wirev += 5;
    ok(*wirev == 0x4, "wv[5] %08x\n", *wirev);
    wirev++;
    ok(*wirev == ul, "wv[6] %08x\n", *wirev);

    if (VARIANT_UNMARSHAL_WORKS)
    {
        VariantInit(&v2);
        stubMsg.Buffer = buffer;
        next = VARIANT_UserUnmarshal(&umcb.Flags, buffer, &v2);
        ok(next == buffer + stubMsg.BufferLength, "got %p expect %p\n", next, buffer + stubMsg.BufferLength);
        ok(V_VT(&v) == V_VT(&v2), "got vt %d expect %d\n", V_VT(&v), V_VT(&v2));
        ok(*V_UI4REF(&v) == *V_UI4REF(&v2), "got ui4 ref %x expect ui4 ref %x\n", *V_UI4REF(&v), *V_UI4REF(&v2));

        VARIANT_UserFree(&umcb.Flags, &v2);
    }
    HeapFree(GetProcessHeap(), 0, oldbuffer);

    /*** R4 ***/
    VariantInit(&v);
    V_VT(&v) = VT_R4;
    V_R8(&v) = 3.1415;

    rpcMsg.BufferLength = stubMsg.BufferLength = VARIANT_UserSize(&umcb.Flags, 0, &v);
    ok(stubMsg.BufferLength == 24, "size %d\n", stubMsg.BufferLength);

    buffer = rpcMsg.Buffer = stubMsg.Buffer = stubMsg.BufferStart = alloc_aligned(stubMsg.BufferLength, &oldbuffer);
    stubMsg.BufferEnd = stubMsg.Buffer + stubMsg.BufferLength;
    next = VARIANT_UserMarshal(&umcb.Flags, buffer, &v);
    ok(next == buffer + stubMsg.BufferLength, "got %p expect %p\n", next, buffer + stubMsg.BufferLength);
    wirev = (DWORD*)buffer;
     
    check_variant_header(wirev, &v, stubMsg.BufferLength);
    wirev += 5;
    ok(*(float*)wirev == V_R4(&v), "wv[5] %08x\n", *wirev);
    if (VARIANT_UNMARSHAL_WORKS)
    {
        VariantInit(&v2);
        stubMsg.Buffer = buffer;
        next = VARIANT_UserUnmarshal(&umcb.Flags, buffer, &v2);
        ok(next == buffer + stubMsg.BufferLength, "got %p expect %p\n", next, buffer + stubMsg.BufferLength);
        ok(V_VT(&v) == V_VT(&v2), "got vt %d expect %d\n", V_VT(&v), V_VT(&v2));
        ok(V_R4(&v) == V_R4(&v2), "got r4 %f expect %f\n", V_R4(&v), V_R4(&v2));

        VARIANT_UserFree(&umcb.Flags, &v2);
    }
    HeapFree(GetProcessHeap(), 0, oldbuffer);

    /*** R8 ***/
    VariantInit(&v);
    V_VT(&v) = VT_R8;
    V_R8(&v) = 3.1415;

    rpcMsg.BufferLength = stubMsg.BufferLength = VARIANT_UserSize(&umcb.Flags, 0, &v);
    ok(stubMsg.BufferLength == 32, "size %d\n", stubMsg.BufferLength);

    buffer = rpcMsg.Buffer = stubMsg.Buffer = stubMsg.BufferStart = alloc_aligned(stubMsg.BufferLength, &oldbuffer);
    stubMsg.BufferEnd = stubMsg.Buffer + stubMsg.BufferLength;
    memset(buffer, 0xcc, stubMsg.BufferLength);
    next = VARIANT_UserMarshal(&umcb.Flags, buffer, &v);
    ok(next == buffer + stubMsg.BufferLength, "got %p expect %p\n", next, buffer + stubMsg.BufferLength);
    wirev = (DWORD*)buffer;
    
    check_variant_header(wirev, &v, stubMsg.BufferLength);
    wirev += 5;
    ok(*wirev == 0xcccccccc, "wv[5] %08x\n", *wirev); /* pad */
    wirev++;
    ok(*(double*)wirev == V_R8(&v), "wv[6] %08x, wv[7] %08x\n", *wirev, *(wirev+1));
    if (VARIANT_UNMARSHAL_WORKS)
    {
        VariantInit(&v2);
        stubMsg.Buffer = buffer;
        next = VARIANT_UserUnmarshal(&umcb.Flags, buffer, &v2);
        ok(next == buffer + stubMsg.BufferLength, "got %p expect %p\n", next, buffer + stubMsg.BufferLength);
        ok(V_VT(&v) == V_VT(&v2), "got vt %d expect %d\n", V_VT(&v), V_VT(&v2));
        ok(V_R8(&v) == V_R8(&v2), "got r8 %f expect %f\n", V_R8(&v), V_R8(&v2));

        VARIANT_UserFree(&umcb.Flags, &v2);
    }
    HeapFree(GetProcessHeap(), 0, oldbuffer);

    /*** R8 BYREF ***/
    VariantInit(&v);
    V_VT(&v) = VT_R8 | VT_BYREF;
    d = 3.1415;
    V_R8REF(&v) = &d;

    rpcMsg.BufferLength = stubMsg.BufferLength = VARIANT_UserSize(&umcb.Flags, 0, &v);
    ok(stubMsg.BufferLength == 32, "size %d\n", stubMsg.BufferLength);

    buffer = rpcMsg.Buffer = stubMsg.Buffer = stubMsg.BufferStart = alloc_aligned(stubMsg.BufferLength, &oldbuffer);
    stubMsg.BufferEnd = stubMsg.Buffer + stubMsg.BufferLength;
    next = VARIANT_UserMarshal(&umcb.Flags, buffer, &v);
    ok(next == buffer + stubMsg.BufferLength, "got %p expect %p\n", next, buffer + stubMsg.BufferLength);
    wirev = (DWORD*)buffer;
    
    check_variant_header(wirev, &v, stubMsg.BufferLength);
    wirev += 5;
    ok(*wirev == 8, "wv[5] %08x\n", *wirev);
    wirev++;
    ok(*(double*)wirev == d, "wv[6] %08x wv[7] %08x\n", *wirev, *(wirev+1));
    if (VARIANT_UNMARSHAL_WORKS)
    {
        VariantInit(&v2);
        stubMsg.Buffer = buffer;
        next = VARIANT_UserUnmarshal(&umcb.Flags, buffer, &v2);
        ok(next == buffer + stubMsg.BufferLength, "got %p expect %p\n", next, buffer + stubMsg.BufferLength);
        ok(V_VT(&v) == V_VT(&v2), "got vt %d expect %d\n", V_VT(&v), V_VT(&v2));
        ok(*V_R8REF(&v) == *V_R8REF(&v2), "got r8 ref %f expect %f\n", *V_R8REF(&v), *V_R8REF(&v2));

        VARIANT_UserFree(&umcb.Flags, &v2);
    }
    HeapFree(GetProcessHeap(), 0, oldbuffer);

    /*** VARIANT_BOOL ***/
    VariantInit(&v);
    V_VT(&v) = VT_BOOL;
    V_BOOL(&v) = 0x1234;

    rpcMsg.BufferLength = stubMsg.BufferLength = VARIANT_UserSize(&umcb.Flags, 0, &v);
    ok(stubMsg.BufferLength == 22, "size %d\n", stubMsg.BufferLength);

    buffer = rpcMsg.Buffer = stubMsg.Buffer = stubMsg.BufferStart = alloc_aligned(stubMsg.BufferLength, &oldbuffer);
    stubMsg.BufferEnd = stubMsg.Buffer + stubMsg.BufferLength;
    next = VARIANT_UserMarshal(&umcb.Flags, buffer, &v);
    ok(next == buffer + stubMsg.BufferLength, "got %p expect %p\n", next, buffer + stubMsg.BufferLength);
    wirev = (DWORD*)buffer;
    
    check_variant_header(wirev, &v, stubMsg.BufferLength);
    wirev += 5;
    ok(*(short*)wirev == V_BOOL(&v), "wv[5] %04x\n", *(WORD*)wirev);
    if (VARIANT_UNMARSHAL_WORKS)
    {
        VariantInit(&v2);
        stubMsg.Buffer = buffer;
        next = VARIANT_UserUnmarshal(&umcb.Flags, buffer, &v2);
        ok(next == buffer + stubMsg.BufferLength, "got %p expect %p\n", next, buffer + stubMsg.BufferLength);
        ok(V_VT(&v) == V_VT(&v2), "got vt %d expect %d\n", V_VT(&v), V_VT(&v2));
        ok(V_BOOL(&v) == V_BOOL(&v2), "got bool %x expect %x\n", V_BOOL(&v), V_BOOL(&v2));

        VARIANT_UserFree(&umcb.Flags, &v2);
    }
    HeapFree(GetProcessHeap(), 0, oldbuffer);

    /*** DECIMAL ***/
    VarDecFromI4(0x12345678, &dec);
    dec.wReserved = 0xfedc;          /* Also initialize reserved field, as we check it later */
    VariantInit(&v);
    V_DECIMAL(&v) = dec;
    V_VT(&v) = VT_DECIMAL;

    rpcMsg.BufferLength = stubMsg.BufferLength = VARIANT_UserSize(&umcb.Flags, 0, &v);
    ok(stubMsg.BufferLength == 40, "size %d\n", stubMsg.BufferLength);

    buffer = rpcMsg.Buffer = stubMsg.Buffer = stubMsg.BufferStart = alloc_aligned(stubMsg.BufferLength, &oldbuffer);
    stubMsg.BufferEnd = stubMsg.Buffer + stubMsg.BufferLength;
    memset(buffer, 0xcc, stubMsg.BufferLength);
    next = VARIANT_UserMarshal(&umcb.Flags, buffer, &v);
    ok(next == buffer + stubMsg.BufferLength, "got %p expect %p\n", next, buffer + stubMsg.BufferLength);
    wirev = (DWORD*)buffer;

    check_variant_header(wirev, &v, stubMsg.BufferLength);
    wirev += 5;
    ok(*wirev == 0xcccccccc, "wirev[5] %08x\n", *wirev); /* pad */
    wirev++;
    dec2 = dec;
    dec2.wReserved = VT_DECIMAL;
    ok(!memcmp(wirev, &dec2, sizeof(dec2)), "wirev[6] %08x wirev[7] %08x wirev[8] %08x wirev[9] %08x\n",
       *wirev, *(wirev + 1), *(wirev + 2), *(wirev + 3));
    if (VARIANT_UNMARSHAL_WORKS)
    {
        VariantInit(&v2);
        stubMsg.Buffer = buffer;
        next = VARIANT_UserUnmarshal(&umcb.Flags, buffer, &v2);
        ok(next == buffer + stubMsg.BufferLength, "got %p expect %p\n", next, buffer + stubMsg.BufferLength);
        ok(V_VT(&v) == V_VT(&v2), "got vt %d expect %d\n", V_VT(&v), V_VT(&v2));
        ok(!memcmp(&V_DECIMAL(&v), & V_DECIMAL(&v2), sizeof(DECIMAL)), "decimals differ\n");

        VARIANT_UserFree(&umcb.Flags, &v2);
    }
    HeapFree(GetProcessHeap(), 0, oldbuffer);

    /*** DECIMAL BYREF ***/
    VariantInit(&v);
    V_VT(&v) = VT_DECIMAL | VT_BYREF;
    V_DECIMALREF(&v) = &dec;

    rpcMsg.BufferLength = stubMsg.BufferLength = VARIANT_UserSize(&umcb.Flags, 0, &v);
    ok(stubMsg.BufferLength == 40, "size %d\n", stubMsg.BufferLength);

    buffer = rpcMsg.Buffer = stubMsg.Buffer = stubMsg.BufferStart = alloc_aligned(stubMsg.BufferLength, &oldbuffer);
    stubMsg.BufferEnd = stubMsg.Buffer + stubMsg.BufferLength;
    next = VARIANT_UserMarshal(&umcb.Flags, buffer, &v);
    ok(next == buffer + stubMsg.BufferLength, "got %p expect %p\n", next, buffer + stubMsg.BufferLength);
    wirev = (DWORD*)buffer;
    
    check_variant_header(wirev, &v, stubMsg.BufferLength);
    wirev += 5;
    ok(*wirev == 16, "wv[5] %08x\n", *wirev);
    wirev++;
    ok(!memcmp(wirev, &dec, sizeof(dec)), "wirev[6] %08x wirev[7] %08x wirev[8] %08x wirev[9] %08x\n", *wirev, *(wirev + 1), *(wirev + 2), *(wirev + 3));
    if (VARIANT_UNMARSHAL_WORKS)
    {
        VariantInit(&v2);
        /* check_variant_header tests wReserved[123], so initialize to unique values.
         * (Could probably also do this by setting the variant to a known DECIMAL.)
         */
        V_U2(&v2).wReserved1 = 0x0123;
        V_U2(&v2).wReserved2 = 0x4567;
        V_U2(&v2).wReserved3 = 0x89ab;

        stubMsg.Buffer = buffer;
        next = VARIANT_UserUnmarshal(&umcb.Flags, buffer, &v2);
        ok(next == buffer + stubMsg.BufferLength, "got %p expect %p\n", next, buffer + stubMsg.BufferLength);
        ok(V_VT(&v) == V_VT(&v2), "got vt %d expect %d\n", V_VT(&v), V_VT(&v2));
        ok(!memcmp(V_DECIMALREF(&v), V_DECIMALREF(&v2), sizeof(DECIMAL)), "decimals differ\n");

        VARIANT_UserFree(&umcb.Flags, &v2);
    }
    HeapFree(GetProcessHeap(), 0, oldbuffer);

    /*** EMPTY ***/
    VariantInit(&v);
    V_VT(&v) = VT_EMPTY;

    rpcMsg.BufferLength = stubMsg.BufferLength = VARIANT_UserSize(&umcb.Flags, 0, &v);
    ok(stubMsg.BufferLength == 20, "size %d\n", stubMsg.BufferLength);

    buffer = rpcMsg.Buffer = stubMsg.Buffer = stubMsg.BufferStart = alloc_aligned(stubMsg.BufferLength, &oldbuffer);
    stubMsg.BufferEnd = stubMsg.Buffer + stubMsg.BufferLength;
    next = VARIANT_UserMarshal(&umcb.Flags, buffer, &v);
    ok(next == buffer + stubMsg.BufferLength, "got %p expect %p\n", next, buffer + stubMsg.BufferLength);
    wirev = (DWORD*)buffer;

    check_variant_header(wirev, &v, stubMsg.BufferLength);
    if (VARIANT_UNMARSHAL_WORKS)
    {
        VariantInit(&v2);
        stubMsg.Buffer = buffer;
        next = VARIANT_UserUnmarshal(&umcb.Flags, buffer, &v2);
        ok(next == buffer + stubMsg.BufferLength, "got %p expect %p\n", next, buffer + stubMsg.BufferLength);
        ok(V_VT(&v) == V_VT(&v2), "got vt %d expect %d\n", V_VT(&v), V_VT(&v2));

        VARIANT_UserFree(&umcb.Flags, &v2);
    }
    HeapFree(GetProcessHeap(), 0, oldbuffer);

    /*** NULL ***/
    VariantInit(&v);
    V_VT(&v) = VT_NULL;

    rpcMsg.BufferLength = stubMsg.BufferLength = VARIANT_UserSize(&umcb.Flags, 0, &v);
    ok(stubMsg.BufferLength == 20, "size %d\n", stubMsg.BufferLength);

    buffer = rpcMsg.Buffer = stubMsg.Buffer = stubMsg.BufferStart = alloc_aligned(stubMsg.BufferLength, &oldbuffer);
    stubMsg.BufferEnd = stubMsg.Buffer + stubMsg.BufferLength;
    next = VARIANT_UserMarshal(&umcb.Flags, buffer, &v);
    ok(next == buffer + stubMsg.BufferLength, "got %p expect %p\n", next, buffer + stubMsg.BufferLength);
    wirev = (DWORD*)buffer;

    check_variant_header(wirev, &v, stubMsg.BufferLength);
    if (VARIANT_UNMARSHAL_WORKS)
    {
        VariantInit(&v2);
        stubMsg.Buffer = buffer;
        next = VARIANT_UserUnmarshal(&umcb.Flags, buffer, &v2);
        ok(next == buffer + stubMsg.BufferLength, "got %p expect %p\n", next, buffer + stubMsg.BufferLength);
        ok(V_VT(&v) == V_VT(&v2), "got vt %d expect %d\n", V_VT(&v), V_VT(&v2));

        VARIANT_UserFree(&umcb.Flags, &v2);
    }
    HeapFree(GetProcessHeap(), 0, oldbuffer);

    /*** BSTR ***/
    b = SysAllocString(str);
    VariantInit(&v);
    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = b;

    rpcMsg.BufferLength = stubMsg.BufferLength = VARIANT_UserSize(&umcb.Flags, 0, &v);
    ok(stubMsg.BufferLength == 60, "size %d\n", stubMsg.BufferLength);
    buffer = rpcMsg.Buffer = stubMsg.Buffer = stubMsg.BufferStart = alloc_aligned(stubMsg.BufferLength, &oldbuffer);
    stubMsg.BufferEnd = stubMsg.Buffer + stubMsg.BufferLength;
    next = VARIANT_UserMarshal(&umcb.Flags, buffer, &v);
    ok(next == buffer + stubMsg.BufferLength, "got %p expect %p\n", next, buffer + stubMsg.BufferLength);
    wirev = (DWORD*)buffer;
    
    check_variant_header(wirev, &v, stubMsg.BufferLength);
    wirev += 5;
    ok(*wirev, "wv[5] %08x\n", *wirev); /* win2k: this is b. winxp: this is (char*)b + 1 */
    wirev++;
    check_bstr(wirev, V_BSTR(&v));
    if (VARIANT_UNMARSHAL_WORKS)
    {
        VariantInit(&v2);
        stubMsg.Buffer = buffer;
        next = VARIANT_UserUnmarshal(&umcb.Flags, buffer, &v2);
        ok(next == buffer + stubMsg.BufferLength, "got %p expect %p\n", next, buffer + stubMsg.BufferLength);
        ok(V_VT(&v) == V_VT(&v2), "got vt %d expect %d\n", V_VT(&v), V_VT(&v2));
        ok(SysStringByteLen(V_BSTR(&v)) == SysStringByteLen(V_BSTR(&v2)), "bstr string lens differ\n");
        ok(!memcmp(V_BSTR(&v), V_BSTR(&v2), SysStringByteLen(V_BSTR(&v))), "bstrs differ\n");

        VARIANT_UserFree(&umcb.Flags, &v2);
    }
    HeapFree(GetProcessHeap(), 0, oldbuffer);

    /*** BSTR BYREF ***/
    VariantInit(&v);
    V_VT(&v) = VT_BSTR | VT_BYREF;
    V_BSTRREF(&v) = &b;

    rpcMsg.BufferLength = stubMsg.BufferLength = VARIANT_UserSize(&umcb.Flags, 0, &v);
    ok(stubMsg.BufferLength == 64, "size %d\n", stubMsg.BufferLength);
    buffer = rpcMsg.Buffer = stubMsg.Buffer = stubMsg.BufferStart = alloc_aligned(stubMsg.BufferLength, &oldbuffer);
    stubMsg.BufferEnd = stubMsg.Buffer + stubMsg.BufferLength;
    next = VARIANT_UserMarshal(&umcb.Flags, buffer, &v);
    ok(next == buffer + stubMsg.BufferLength, "got %p expect %p\n", next, buffer + stubMsg.BufferLength);
    wirev = (DWORD*)buffer;
    
    check_variant_header(wirev, &v, stubMsg.BufferLength);
    wirev += 5;
    ok(*wirev == 0x4, "wv[5] %08x\n", *wirev);
    wirev++;
    ok(*wirev, "wv[6] %08x\n", *wirev); /* win2k: this is b. winxp: this is (char*)b + 1 */
    wirev++;
    check_bstr(wirev, b);
    if (VARIANT_UNMARSHAL_WORKS)
    {
        VariantInit(&v2);
        stubMsg.Buffer = buffer;
        next = VARIANT_UserUnmarshal(&umcb.Flags, buffer, &v2);
        ok(next == buffer + stubMsg.BufferLength, "got %p expect %p\n", next, buffer + stubMsg.BufferLength);
        ok(V_VT(&v) == V_VT(&v2), "got vt %d expect %d\n", V_VT(&v), V_VT(&v2));
        ok(SysStringByteLen(*V_BSTRREF(&v)) == SysStringByteLen(*V_BSTRREF(&v2)), "bstr string lens differ\n");
        ok(!memcmp(*V_BSTRREF(&v), *V_BSTRREF(&v2), SysStringByteLen(*V_BSTRREF(&v))), "bstrs differ\n");

        VARIANT_UserFree(&umcb.Flags, &v2);
    }
    HeapFree(GetProcessHeap(), 0, oldbuffer);
    SysFreeString(b);

    /*** ARRAY ***/
    sab.lLbound = 5;
    sab.cElements = 10;

    lpsa = SafeArrayCreate(VT_R8, 1, &sab);
    *(DWORD *)lpsa->pvData = 0xcafebabe;
    *((DWORD *)lpsa->pvData + 1) = 0xdeadbeef;

    VariantInit(&v);
    V_VT(&v) = VT_UI4 | VT_ARRAY;
    V_ARRAY(&v) = lpsa;

    rpcMsg.BufferLength = stubMsg.BufferLength = VARIANT_UserSize(&umcb.Flags, 0, &v);
    expected = 152;
    ok(stubMsg.BufferLength == expected || stubMsg.BufferLength == expected + 8, /* win64 */
       "size %u instead of %u\n", stubMsg.BufferLength, expected);
    buffer = rpcMsg.Buffer = stubMsg.Buffer = stubMsg.BufferStart = alloc_aligned(stubMsg.BufferLength, &oldbuffer);
    stubMsg.BufferEnd = stubMsg.Buffer + stubMsg.BufferLength;
    next = VARIANT_UserMarshal(&umcb.Flags, buffer, &v);
    ok(next == buffer + expected, "got %p expect %p\n", next, buffer + expected);
    wirev = (DWORD*)buffer;
    
    check_variant_header(wirev, &v, expected);
    wirev += 5;
    ok(*wirev, "wv[5] %08x\n", *wirev); /* win2k: this is lpsa. winxp: this is (char*)lpsa + 1 */
    wirev++;
    check_safearray(wirev, lpsa);
    if (VARIANT_UNMARSHAL_WORKS)
    {
        LONG bound, bound2;
        VARTYPE vt, vt2;
        VariantInit(&v2);
        stubMsg.Buffer = buffer;
        next = VARIANT_UserUnmarshal(&umcb.Flags, buffer, &v2);
        ok(next == buffer + expected, "got %p expect %p\n", next, buffer + expected);
        ok(V_VT(&v) == V_VT(&v2), "got vt %d expect %d\n", V_VT(&v), V_VT(&v2));
        ok(SafeArrayGetDim(V_ARRAY(&v)) == SafeArrayGetDim(V_ARRAY(&v)), "array dims differ\n");  
        SafeArrayGetLBound(V_ARRAY(&v), 1, &bound);
        SafeArrayGetLBound(V_ARRAY(&v2), 1, &bound2);
        ok(bound == bound2, "array lbounds differ\n");
        SafeArrayGetUBound(V_ARRAY(&v), 1, &bound);
        SafeArrayGetUBound(V_ARRAY(&v2), 1, &bound2);
        ok(bound == bound2, "array ubounds differ\n");
        SafeArrayGetVartype(V_ARRAY(&v), &vt);
        SafeArrayGetVartype(V_ARRAY(&v2), &vt2);
        ok(vt == vt2, "array vts differ %x %x\n", vt, vt2);
        VARIANT_UserFree(&umcb.Flags, &v2);
    }
    HeapFree(GetProcessHeap(), 0, oldbuffer);

    /*** ARRAY BYREF ***/
    VariantInit(&v);
    V_VT(&v) = VT_UI4 | VT_ARRAY | VT_BYREF;
    V_ARRAYREF(&v) = &lpsa;

    rpcMsg.BufferLength = stubMsg.BufferLength = VARIANT_UserSize(&umcb.Flags, 0, &v);
    expected = 152;
    ok(stubMsg.BufferLength == expected || stubMsg.BufferLength == expected + 16, /* win64 */
       "size %u instead of %u\n", stubMsg.BufferLength, expected);
    buffer = rpcMsg.Buffer = stubMsg.Buffer = stubMsg.BufferStart = alloc_aligned(stubMsg.BufferLength, &oldbuffer);
    stubMsg.BufferEnd = stubMsg.Buffer + stubMsg.BufferLength;
    next = VARIANT_UserMarshal(&umcb.Flags, buffer, &v);
    ok(next == buffer + expected, "got %p expect %p\n", next, buffer + expected);
    wirev = (DWORD*)buffer;

    check_variant_header(wirev, &v, expected);
    wirev += 5;
    ok(*wirev == 4, "wv[5] %08x\n", *wirev);
    wirev++;
    ok(*wirev, "wv[6] %08x\n", *wirev); /* win2k: this is lpsa. winxp: this is (char*)lpsa + 1 */
    wirev++;
    check_safearray(wirev, lpsa);
    if (VARIANT_UNMARSHAL_WORKS)
    {
        LONG bound, bound2;
        VARTYPE vt, vt2;
        VariantInit(&v2);
        stubMsg.Buffer = buffer;
        next = VARIANT_UserUnmarshal(&umcb.Flags, buffer, &v2);
        ok(next == buffer + expected, "got %p expect %p\n", next, buffer + expected);
        ok(V_VT(&v) == V_VT(&v2), "got vt %d expect %d\n", V_VT(&v), V_VT(&v2));
        ok(SafeArrayGetDim(*V_ARRAYREF(&v)) == SafeArrayGetDim(*V_ARRAYREF(&v)), "array dims differ\n");  
        SafeArrayGetLBound(*V_ARRAYREF(&v), 1, &bound);
        SafeArrayGetLBound(*V_ARRAYREF(&v2), 1, &bound2);
        ok(bound == bound2, "array lbounds differ\n");
        SafeArrayGetUBound(*V_ARRAYREF(&v), 1, &bound);
        SafeArrayGetUBound(*V_ARRAYREF(&v2), 1, &bound2);
        ok(bound == bound2, "array ubounds differ\n");
        SafeArrayGetVartype(*V_ARRAYREF(&v), &vt);
        SafeArrayGetVartype(*V_ARRAYREF(&v2), &vt2);
        ok(vt == vt2, "array vts differ %x %x\n", vt, vt2);
        VARIANT_UserFree(&umcb.Flags, &v2);
    }
    HeapFree(GetProcessHeap(), 0, oldbuffer);
    SafeArrayDestroy(lpsa);

    /*** VARIANT BYREF ***/
    VariantInit(&v);
    VariantInit(&v2);
    V_VT(&v2) = VT_R8;
    V_R8(&v2) = 3.1415;
    V_VT(&v) = VT_VARIANT | VT_BYREF;
    V_VARIANTREF(&v) = &v2;

    rpcMsg.BufferLength = stubMsg.BufferLength = VARIANT_UserSize(&umcb.Flags, 0, &v);
    ok(stubMsg.BufferLength == 64, "size %d\n", stubMsg.BufferLength);
    buffer = rpcMsg.Buffer = stubMsg.Buffer = stubMsg.BufferStart = alloc_aligned(stubMsg.BufferLength, &oldbuffer);
    stubMsg.BufferEnd = stubMsg.Buffer + stubMsg.BufferLength;
    memset(buffer, 0xcc, stubMsg.BufferLength);
    next = VARIANT_UserMarshal(&umcb.Flags, buffer, &v);
    ok(next == buffer + stubMsg.BufferLength, "got %p expect %p\n", next, buffer + stubMsg.BufferLength);
    wirev = (DWORD*)buffer;
    check_variant_header(wirev, &v, stubMsg.BufferLength);
    wirev += 5;

    ok(*wirev == sizeof(VARIANT), "wv[5] %08x\n", *wirev);
    wirev++;
    ok(*wirev == ('U' | 's' << 8 | 'e' << 16 | 'r' << 24), "wv[6] %08x\n", *wirev); /* 'User' */
    wirev++;
    ok(*wirev == 0xcccccccc, "wv[7] %08x\n", *wirev); /* pad */
    wirev++;
    check_variant_header(wirev, &v2, stubMsg.BufferLength - 32);
    wirev += 5;
    ok(*wirev == 0xcccccccc, "wv[13] %08x\n", *wirev); /* pad for VT_R8 */
    wirev++;
    ok(*(double*)wirev == V_R8(&v2), "wv[6] %08x wv[7] %08x\n", *wirev, *(wirev+1));
    if (VARIANT_UNMARSHAL_WORKS)
    {
        VARIANT v3;
        VariantInit(&v3);
        stubMsg.Buffer = buffer;
        next = VARIANT_UserUnmarshal(&umcb.Flags, buffer, &v3);
        ok(next == buffer + stubMsg.BufferLength, "got %p expect %p\n", next, buffer + stubMsg.BufferLength);
        ok(V_VT(&v) == V_VT(&v3), "got vt %d expect %d\n", V_VT(&v), V_VT(&v3));
        ok(V_VT(V_VARIANTREF(&v)) == V_VT(V_VARIANTREF(&v3)), "vts differ %x %x\n",
           V_VT(V_VARIANTREF(&v)), V_VT(V_VARIANTREF(&v3))); 
        ok(V_R8(V_VARIANTREF(&v)) == V_R8(V_VARIANTREF(&v3)), "r8s differ\n"); 
        VARIANT_UserFree(&umcb.Flags, &v3);
    }
    HeapFree(GetProcessHeap(), 0, oldbuffer);

    /*** UNKNOWN ***/
    heap_unknown = HeapAlloc(GetProcessHeap(), 0, sizeof(*heap_unknown));
    heap_unknown->lpVtbl = &HeapUnknown_Vtbl;
    heap_unknown->refs = 1;
    VariantInit(&v);
    VariantInit(&v2);
    V_VT(&v) = VT_UNKNOWN;
    V_UNKNOWN(&v) = (IUnknown *)heap_unknown;

    rpcMsg.BufferLength = stubMsg.BufferLength = VARIANT_UserSize(&umcb.Flags, 0, &v);
    ok(stubMsg.BufferLength > 32, "size %d\n", stubMsg.BufferLength);
    buffer = rpcMsg.Buffer = stubMsg.Buffer = stubMsg.BufferStart = alloc_aligned(stubMsg.BufferLength, &oldbuffer);
    stubMsg.BufferEnd = stubMsg.Buffer + stubMsg.BufferLength;
    memset(buffer, 0xcc, stubMsg.BufferLength);
    next = VARIANT_UserMarshal(&umcb.Flags, buffer, &v);
    wirev = (DWORD*)buffer;
    check_variant_header(wirev, &v, next - buffer);
    wirev += 5;

    todo_wine
    ok(*wirev == (DWORD_PTR)V_UNKNOWN(&v) /* Win9x */ ||
       *wirev == (DWORD_PTR)V_UNKNOWN(&v) + 1 /* NT */, "wv[5] %08x\n", *wirev);
    wirev++;
    todo_wine
    ok(*wirev == next - buffer - 0x20, "wv[6] %08x\n", *wirev);
    wirev++;
    todo_wine
    ok(*wirev == next - buffer - 0x20, "wv[7] %08x\n", *wirev);
    wirev++;
    todo_wine
    ok(*wirev == 0x574f454d, "wv[8] %08x\n", *wirev);
    if (VARIANT_UNMARSHAL_WORKS)
    {
        VARIANT v3;
        VariantInit(&v3);
        V_VT(&v3) = VT_UNKNOWN;
        V_UNKNOWN(&v3) = (IUnknown *)heap_unknown;
        IUnknown_AddRef(V_UNKNOWN(&v3));
        stubMsg.Buffer = buffer;
        next = VARIANT_UserUnmarshal(&umcb.Flags, buffer, &v3);
        ok(V_VT(&v) == V_VT(&v3), "got vt %d expect %d\n", V_VT(&v), V_VT(&v3));
        ok(V_UNKNOWN(&v) == V_UNKNOWN(&v3), "got %p expect %p\n", V_UNKNOWN(&v), V_UNKNOWN(&v3));
        VARIANT_UserFree(&umcb.Flags, &v3);
        ok(heap_unknown->refs == 1, "%d refcounts of IUnknown leaked\n", heap_unknown->refs - 1);
        IUnknown_Release((IUnknown *)heap_unknown);
    }
    HeapFree(GetProcessHeap(), 0, oldbuffer);

    /*** UNKNOWN BYREF ***/
    heap_unknown = HeapAlloc(GetProcessHeap(), 0, sizeof(*heap_unknown));
    heap_unknown->lpVtbl = &HeapUnknown_Vtbl;
    heap_unknown->refs = 1;
    VariantInit(&v);
    VariantInit(&v2);
    V_VT(&v) = VT_UNKNOWN | VT_BYREF;
    V_UNKNOWNREF(&v) = (IUnknown **)&heap_unknown;

    rpcMsg.BufferLength = stubMsg.BufferLength = VARIANT_UserSize(&umcb.Flags, 0, &v);
    ok(stubMsg.BufferLength > 36, "size %d\n", stubMsg.BufferLength);
    buffer = rpcMsg.Buffer = stubMsg.Buffer = stubMsg.BufferStart = alloc_aligned(stubMsg.BufferLength, &oldbuffer);
    stubMsg.BufferEnd = stubMsg.Buffer + stubMsg.BufferLength;
    memset(buffer, 0xcc, stubMsg.BufferLength);
    next = VARIANT_UserMarshal(&umcb.Flags, buffer, &v);
    wirev = (DWORD*)buffer;
    check_variant_header(wirev, &v, next - buffer);
    wirev += 5;

    ok(*wirev == 4, "wv[5] %08x\n", *wirev);
    wirev++;
    todo_wine
    ok(*wirev == (DWORD_PTR)heap_unknown /* Win9x, Win2000 */ ||
       *wirev == (DWORD_PTR)heap_unknown + 1 /* XP */, "wv[6] %08x\n", *wirev);
    wirev++;
    todo_wine
    ok(*wirev == next - buffer - 0x24, "wv[7] %08x\n", *wirev);
    wirev++;
    todo_wine
    ok(*wirev == next - buffer - 0x24, "wv[8] %08x\n", *wirev);
    wirev++;
    todo_wine
    ok(*wirev == 0x574f454d, "wv[9] %08x\n", *wirev);
    if (VARIANT_UNMARSHAL_WORKS)
    {
        VARIANT v3;
        VariantInit(&v3);
        V_VT(&v3) = VT_UNKNOWN;
        V_UNKNOWN(&v3) = (IUnknown *)heap_unknown;
        IUnknown_AddRef(V_UNKNOWN(&v3));
        stubMsg.Buffer = buffer;
        next = VARIANT_UserUnmarshal(&umcb.Flags, buffer, &v3);
        ok(V_VT(&v) == V_VT(&v3), "got vt %d expect %d\n", V_VT(&v), V_VT(&v3));
        ok(*V_UNKNOWNREF(&v) == *V_UNKNOWNREF(&v3), "got %p expect %p\n", *V_UNKNOWNREF(&v), *V_UNKNOWNREF(&v3));
        VARIANT_UserFree(&umcb.Flags, &v3);
        ok(heap_unknown->refs == 1, "%d refcounts of IUnknown leaked\n", heap_unknown->refs - 1);
        IUnknown_Release((IUnknown *)heap_unknown);
    }
    HeapFree(GetProcessHeap(), 0, oldbuffer);
}


START_TEST(usrmarshal)
{
    CoInitialize(NULL);

    test_marshal_LPSAFEARRAY();
    test_marshal_BSTR();
    test_marshal_VARIANT();

    CoUninitialize();
}
