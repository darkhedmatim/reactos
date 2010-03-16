/*
 * User Marshaling Tests
 *
 * Copyright 2004-2006 Robert Shearman for CodeWeavers
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
#define CONST_VTABLE
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "objidl.h"

#include "wine/test.h"

ULONG __RPC_USER HMETAFILE_UserSize(ULONG *, unsigned long, HMETAFILE *);
unsigned char * __RPC_USER HMETAFILE_UserMarshal(ULONG *, unsigned char *, HMETAFILE *);
unsigned char * __RPC_USER HMETAFILE_UserUnmarshal(ULONG *, unsigned char *, HMETAFILE *);
void __RPC_USER HMETAFILE_UserFree(ULONG *, HMETAFILE *);

ULONG __RPC_USER HENHMETAFILE_UserSize(ULONG *, ULONG, HENHMETAFILE *);
unsigned char * __RPC_USER HENHMETAFILE_UserMarshal  (ULONG *, unsigned char *, HENHMETAFILE *);
unsigned char * __RPC_USER HENHMETAFILE_UserUnmarshal(ULONG *, unsigned char *, HENHMETAFILE *);
void  __RPC_USER HENHMETAFILE_UserFree(ULONG *, HENHMETAFILE *);

ULONG __RPC_USER HMETAFILEPICT_UserSize(ULONG *, ULONG, HMETAFILEPICT *);
unsigned char * __RPC_USER HMETAFILEPICT_UserMarshal  (ULONG *, unsigned char *, HMETAFILEPICT *);
unsigned char * __RPC_USER HMETAFILEPICT_UserUnmarshal(ULONG *, unsigned char *, HMETAFILEPICT *);
void __RPC_USER HMETAFILEPICT_UserFree(ULONG *, HMETAFILEPICT *);

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

static const char cf_marshaled[] =
{
    0x9, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0,
    0x9, 0x0, 0x0, 0x0,
    'M', 0x0, 'y', 0x0,
    'F', 0x0, 'o', 0x0,
    'r', 0x0, 'm', 0x0,
    'a', 0x0, 't', 0x0,
    0x0, 0x0
};

static void test_marshal_CLIPFORMAT(void)
{
    USER_MARSHAL_CB umcb;
    MIDL_STUB_MESSAGE stub_msg;
    RPC_MESSAGE rpc_msg;
    unsigned char *buffer;
    ULONG i, size;
    CLIPFORMAT cf = RegisterClipboardFormatA("MyFormat");
    CLIPFORMAT cf2;

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
    size = CLIPFORMAT_UserSize(&umcb.Flags, 0, &cf);
    ok(size == 8 + sizeof(cf_marshaled) ||
       broken(size == 12 + sizeof(cf_marshaled)) ||  /* win64 adds 4 extra (unused) bytes */
       broken(size == 8 + sizeof(cf_marshaled) - 2), /* win9x and winnt don't include the '\0' */
              "CLIPFORMAT: Wrong size %d\n", size);

    buffer = HeapAlloc(GetProcessHeap(), 0, size);
    memset( buffer, 0xcc, size );
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_DIFFERENTMACHINE);
    CLIPFORMAT_UserMarshal(&umcb.Flags, buffer, &cf);
    ok(*(LONG *)(buffer + 0) == WDT_REMOTE_CALL, "CLIPFORMAT: Context should be WDT_REMOTE_CALL instead of 0x%08x\n", *(LONG *)(buffer + 0));
    ok(*(DWORD *)(buffer + 4) == cf, "CLIPFORMAT: Marshaled value should be 0x%04x instead of 0x%04x\n", cf, *(DWORD *)(buffer + 4));
    ok(!memcmp(buffer + 8, cf_marshaled, min( sizeof(cf_marshaled), size-8 )), "Marshaled data differs\n");
    if (size > sizeof(cf_marshaled) + 8)  /* make sure the extra bytes are not used */
        for (i = sizeof(cf_marshaled) + 8; i < size; i++)
            ok( buffer[i] == 0xcc, "buffer offset %u has been set to %x\n", i, buffer[i] );

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_DIFFERENTMACHINE);
    CLIPFORMAT_UserUnmarshal(&umcb.Flags, buffer, &cf2);
    ok(cf == cf2, "CLIPFORMAT: Didn't unmarshal properly\n");
    HeapFree(GetProcessHeap(), 0, buffer);

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
    CLIPFORMAT_UserFree(&umcb.Flags, &cf2);
}

static void test_marshal_HWND(void)
{
    USER_MARSHAL_CB umcb;
    MIDL_STUB_MESSAGE stub_msg;
    RPC_MESSAGE rpc_msg;
    unsigned char *buffer;
    ULONG size;
    HWND hwnd = GetDesktopWindow();
    HWND hwnd2;
    wireHWND wirehwnd;

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_LOCAL);
    size = HWND_UserSize(&umcb.Flags, 0, &hwnd);
    ok(size == sizeof(*wirehwnd), "Wrong size %d\n", size);

    buffer = HeapAlloc(GetProcessHeap(), 0, size);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_LOCAL);
    HWND_UserMarshal(&umcb.Flags, buffer, &hwnd);
    wirehwnd = (wireHWND)buffer;
    ok(wirehwnd->fContext == WDT_INPROC_CALL, "Context should be WDT_INPROC_CALL instead of 0x%08x\n", wirehwnd->fContext);
    ok(wirehwnd->u.hInproc == (LONG_PTR)hwnd, "Marshaled value should be %p instead of %p\n", hwnd, (HANDLE)wirehwnd->u.hRemote);

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_LOCAL);
    HWND_UserUnmarshal(&umcb.Flags, buffer, &hwnd2);
    ok(hwnd == hwnd2, "Didn't unmarshal properly\n");
    HeapFree(GetProcessHeap(), 0, buffer);

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_LOCAL);
    HWND_UserFree(&umcb.Flags, &hwnd2);
}

static void test_marshal_HGLOBAL(void)
{
    USER_MARSHAL_CB umcb;
    MIDL_STUB_MESSAGE stub_msg;
    RPC_MESSAGE rpc_msg;
    unsigned char *buffer;
    ULONG size;
    HGLOBAL hglobal;
    HGLOBAL hglobal2;
    unsigned char *wirehglobal;
    int i;

    hglobal = NULL;
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_LOCAL);
    size = HGLOBAL_UserSize(&umcb.Flags, 0, &hglobal);
    /* native is poorly programmed and allocates 4/8 bytes more than it needs to
     * here - Wine doesn't have to emulate that */
    ok((size == 8) || broken(size == 12) || broken(size == 16), "Size should be 8, instead of %d\n", size);
    buffer = HeapAlloc(GetProcessHeap(), 0, size);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_LOCAL);
    HGLOBAL_UserMarshal(&umcb.Flags, buffer, &hglobal);
    wirehglobal = buffer;
    ok(*(ULONG *)wirehglobal == WDT_REMOTE_CALL, "Context should be WDT_REMOTE_CALL instead of 0x%08x\n", *(ULONG *)wirehglobal);
    wirehglobal += sizeof(ULONG);
    ok(*(ULONG *)wirehglobal == 0, "buffer+4 should be HGLOBAL\n");
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_LOCAL);
    HGLOBAL_UserUnmarshal(&umcb.Flags, buffer, &hglobal2);
    ok(hglobal2 == hglobal, "Didn't unmarshal properly\n");
    HeapFree(GetProcessHeap(), 0, buffer);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_LOCAL);
    HGLOBAL_UserFree(&umcb.Flags, &hglobal2);

    hglobal = GlobalAlloc(0, 4);
    buffer = GlobalLock(hglobal);
    for (i = 0; i < 4; i++)
        buffer[i] = i;
    GlobalUnlock(hglobal);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_LOCAL);
    size = HGLOBAL_UserSize(&umcb.Flags, 0, &hglobal);
    /* native is poorly programmed and allocates 4/8 bytes more than it needs to
     * here - Wine doesn't have to emulate that */
    ok((size == 24) || broken(size == 28) || broken(size == 32), "Size should be 24, instead of %d\n", size);
    buffer = HeapAlloc(GetProcessHeap(), 0, size);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_LOCAL);
    HGLOBAL_UserMarshal(&umcb.Flags, buffer, &hglobal);
    wirehglobal = buffer;
    ok(*(ULONG *)wirehglobal == WDT_REMOTE_CALL, "Context should be WDT_REMOTE_CALL instead of 0x%08x\n", *(ULONG *)wirehglobal);
    wirehglobal += sizeof(ULONG);
    ok(*(ULONG *)wirehglobal == (ULONG)(ULONG_PTR)hglobal, "buffer+0x4 should be HGLOBAL\n");
    wirehglobal += sizeof(ULONG);
    ok(*(ULONG *)wirehglobal == 4, "buffer+0x8 should be size of HGLOBAL instead of %d\n", *(ULONG *)wirehglobal);
    wirehglobal += sizeof(ULONG);
    ok(*(ULONG *)wirehglobal == (ULONG)(ULONG_PTR)hglobal, "buffer+0xc should be HGLOBAL\n");
    wirehglobal += sizeof(ULONG);
    ok(*(ULONG *)wirehglobal == 4, "buffer+0x10 should be size of HGLOBAL instead of %d\n", *(ULONG *)wirehglobal);
    wirehglobal += sizeof(ULONG);
    for (i = 0; i < 4; i++)
        ok(wirehglobal[i] == i, "buffer+0x%x should be %d\n", 0x10 + i, i);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_LOCAL);
    HGLOBAL_UserUnmarshal(&umcb.Flags, buffer, &hglobal2);
    ok(hglobal2 != NULL, "Didn't unmarshal properly\n");
    HeapFree(GetProcessHeap(), 0, buffer);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_LOCAL);
    HGLOBAL_UserFree(&umcb.Flags, &hglobal2);
    GlobalFree(hglobal);
}

static HENHMETAFILE create_emf(void)
{
    const RECT rect = {0, 0, 100, 100};
    HDC hdc = CreateEnhMetaFile(NULL, NULL, &rect, "HENHMETAFILE Marshaling Test\0Test\0\0");
    ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rect, "Test String", strlen("Test String"), NULL);
    return CloseEnhMetaFile(hdc);
}

static void test_marshal_HENHMETAFILE(void)
{
    USER_MARSHAL_CB umcb;
    MIDL_STUB_MESSAGE stub_msg;
    RPC_MESSAGE rpc_msg;
    unsigned char *buffer;
    ULONG size;
    HENHMETAFILE hemf;
    HENHMETAFILE hemf2 = NULL;
    unsigned char *wirehemf;

    hemf = create_emf();

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
    size = HENHMETAFILE_UserSize(&umcb.Flags, 0, &hemf);
    ok(size > 20, "size should be at least 20 bytes, not %d\n", size);
    buffer = HeapAlloc(GetProcessHeap(), 0, size);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_DIFFERENTMACHINE);
    HENHMETAFILE_UserMarshal(&umcb.Flags, buffer, &hemf);
    wirehemf = buffer;
    ok(*(DWORD *)wirehemf == WDT_REMOTE_CALL, "wirestgm + 0x0 should be WDT_REMOTE_CALL instead of 0x%08x\n", *(DWORD *)wirehemf);
    wirehemf += sizeof(DWORD);
    ok(*(DWORD *)wirehemf == (DWORD)(DWORD_PTR)hemf, "wirestgm + 0x4 should be hemf instead of 0x%08x\n", *(DWORD *)wirehemf);
    wirehemf += sizeof(DWORD);
    ok(*(DWORD *)wirehemf == (size - 0x10), "wirestgm + 0x8 should be size - 0x10 instead of 0x%08x\n", *(DWORD *)wirehemf);
    wirehemf += sizeof(DWORD);
    ok(*(DWORD *)wirehemf == (size - 0x10), "wirestgm + 0xc should be size - 0x10 instead of 0x%08x\n", *(DWORD *)wirehemf);
    wirehemf += sizeof(DWORD);
    ok(*(DWORD *)wirehemf == EMR_HEADER, "wirestgm + 0x10 should be EMR_HEADER instead of %d\n", *(DWORD *)wirehemf);
    wirehemf += sizeof(DWORD);
    /* ... rest of data not tested - refer to tests for GetEnhMetaFileBits
     * at this point */

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_DIFFERENTMACHINE);
    HENHMETAFILE_UserUnmarshal(&umcb.Flags, buffer, &hemf2);
    ok(hemf2 != NULL, "HENHMETAFILE didn't unmarshal\n");
    HeapFree(GetProcessHeap(), 0, buffer);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
    HENHMETAFILE_UserFree(&umcb.Flags, &hemf2);
    DeleteEnhMetaFile(hemf);

    /* test NULL emf */
    hemf = NULL;

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
    size = HENHMETAFILE_UserSize(&umcb.Flags, 0, &hemf);
    ok(size == 8, "size should be 8 bytes, not %d\n", size);
    buffer = HeapAlloc(GetProcessHeap(), 0, size);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_DIFFERENTMACHINE);
    HENHMETAFILE_UserMarshal(&umcb.Flags, buffer, &hemf);
    wirehemf = buffer;
    ok(*(DWORD *)wirehemf == WDT_REMOTE_CALL, "wirestgm + 0x0 should be WDT_REMOTE_CALL instead of 0x%08x\n", *(DWORD *)wirehemf);
    wirehemf += sizeof(DWORD);
    ok(*(DWORD *)wirehemf == (DWORD)(DWORD_PTR)hemf, "wirestgm + 0x4 should be hemf instead of 0x%08x\n", *(DWORD *)wirehemf);
    wirehemf += sizeof(DWORD);

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_DIFFERENTMACHINE);
    HENHMETAFILE_UserUnmarshal(&umcb.Flags, buffer, &hemf2);
    ok(hemf2 == NULL, "NULL HENHMETAFILE didn't unmarshal\n");
    HeapFree(GetProcessHeap(), 0, buffer);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
    HENHMETAFILE_UserFree(&umcb.Flags, &hemf2);
}

static HMETAFILE create_mf(void)
{
    RECT rect = {0, 0, 100, 100};
    HDC hdc = CreateMetaFile(NULL);
    ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rect, "Test String", strlen("Test String"), NULL);
    return CloseMetaFile(hdc);
}

static void test_marshal_HMETAFILE(void)
{
    USER_MARSHAL_CB umcb;
    MIDL_STUB_MESSAGE stub_msg;
    RPC_MESSAGE rpc_msg;
    unsigned char *buffer;
    ULONG size;
    HMETAFILE hmf;
    HMETAFILE hmf2 = NULL;
    unsigned char *wirehmf;

    hmf = create_mf();

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
    size = HMETAFILE_UserSize(&umcb.Flags, 0, &hmf);
    ok(size > 20, "size should be at least 20 bytes, not %d\n", size);
    buffer = HeapAlloc(GetProcessHeap(), 0, size);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_DIFFERENTMACHINE);
    HMETAFILE_UserMarshal(&umcb.Flags, buffer, &hmf);
    wirehmf = buffer;
    ok(*(DWORD *)wirehmf == WDT_REMOTE_CALL, "wirestgm + 0x0 should be WDT_REMOTE_CALL instead of 0x%08x\n", *(DWORD *)wirehmf);
    wirehmf += sizeof(DWORD);
    ok(*(DWORD *)wirehmf == (DWORD)(DWORD_PTR)hmf, "wirestgm + 0x4 should be hmf instead of 0x%08x\n", *(DWORD *)wirehmf);
    wirehmf += sizeof(DWORD);
    ok(*(DWORD *)wirehmf == (size - 0x10), "wirestgm + 0x8 should be size - 0x10 instead of 0x%08x\n", *(DWORD *)wirehmf);
    wirehmf += sizeof(DWORD);
    ok(*(DWORD *)wirehmf == (size - 0x10), "wirestgm + 0xc should be size - 0x10 instead of 0x%08x\n", *(DWORD *)wirehmf);
    wirehmf += sizeof(DWORD);
    ok(*(WORD *)wirehmf == 1, "wirestgm + 0x10 should be 1 instead of 0x%08x\n", *(DWORD *)wirehmf);
    wirehmf += sizeof(DWORD);
    /* ... rest of data not tested - refer to tests for GetMetaFileBits
     * at this point */

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_DIFFERENTMACHINE);
    HMETAFILE_UserUnmarshal(&umcb.Flags, buffer, &hmf2);
    ok(hmf2 != NULL, "HMETAFILE didn't unmarshal\n");
    HeapFree(GetProcessHeap(), 0, buffer);
    HMETAFILE_UserFree(&umcb.Flags, &hmf2);
    DeleteMetaFile(hmf);

    /* test NULL emf */
    hmf = NULL;

    size = HMETAFILE_UserSize(&umcb.Flags, 0, &hmf);
    ok(size == 8, "size should be 8 bytes, not %d\n", size);
    buffer = HeapAlloc(GetProcessHeap(), 0, size);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_DIFFERENTMACHINE);
    HMETAFILE_UserMarshal(&umcb.Flags, buffer, &hmf);
    wirehmf = buffer;
    ok(*(DWORD *)wirehmf == WDT_REMOTE_CALL, "wirestgm + 0x0 should be WDT_REMOTE_CALL instead of 0x%08x\n", *(DWORD *)wirehmf);
    wirehmf += sizeof(DWORD);
    ok(*(DWORD *)wirehmf == (DWORD)(DWORD_PTR)hmf, "wirestgm + 0x4 should be hmf instead of 0x%08x\n", *(DWORD *)wirehmf);
    wirehmf += sizeof(DWORD);

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_DIFFERENTMACHINE);
    HMETAFILE_UserUnmarshal(&umcb.Flags, buffer, &hmf2);
    ok(hmf2 == NULL, "NULL HMETAFILE didn't unmarshal\n");
    HeapFree(GetProcessHeap(), 0, buffer);
    HMETAFILE_UserFree(&umcb.Flags, &hmf2);
}

#define USER_MARSHAL_PTR_PREFIX \
  ( (DWORD)'U'         | ( (DWORD)'s' << 8 ) | \
  ( (DWORD)'e' << 16 ) | ( (DWORD)'r' << 24 ) )

static void test_marshal_HMETAFILEPICT(void)
{
    USER_MARSHAL_CB umcb;
    MIDL_STUB_MESSAGE stub_msg;
    RPC_MESSAGE rpc_msg;
    unsigned char *buffer, *buffer_end;
    ULONG size;
    HMETAFILEPICT hmfp;
    HMETAFILEPICT hmfp2 = NULL;
    METAFILEPICT *pmfp;
    unsigned char *wirehmfp;

    hmfp = GlobalAlloc(GMEM_MOVEABLE, sizeof(*pmfp));
    pmfp = GlobalLock(hmfp);
    pmfp->mm = MM_ISOTROPIC;
    pmfp->xExt = 1;
    pmfp->yExt = 2;
    pmfp->hMF = create_mf();
    GlobalUnlock(hmfp);

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
    size = HMETAFILEPICT_UserSize(&umcb.Flags, 0, &hmfp);
    ok(size > 20, "size should be at least 20 bytes, not %d\n", size);
    trace("size is %d\n", size);
    buffer = HeapAlloc(GetProcessHeap(), 0, size);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_DIFFERENTMACHINE);
    buffer_end = HMETAFILEPICT_UserMarshal(&umcb.Flags, buffer, &hmfp);
    wirehmfp = buffer;
    ok(*(DWORD *)wirehmfp == WDT_REMOTE_CALL, "wirestgm + 0x0 should be WDT_REMOTE_CALL instead of 0x%08x\n", *(DWORD *)wirehmfp);
    wirehmfp += sizeof(DWORD);
    ok(*(DWORD *)wirehmfp == (DWORD)(DWORD_PTR)hmfp, "wirestgm + 0x4 should be hmf instead of 0x%08x\n", *(DWORD *)wirehmfp);
    wirehmfp += sizeof(DWORD);
    ok(*(DWORD *)wirehmfp == MM_ISOTROPIC, "wirestgm + 0x8 should be MM_ISOTROPIC instead of 0x%08x\n", *(DWORD *)wirehmfp);
    wirehmfp += sizeof(DWORD);
    ok(*(DWORD *)wirehmfp == 1, "wirestgm + 0xc should be 1 instead of 0x%08x\n", *(DWORD *)wirehmfp);
    wirehmfp += sizeof(DWORD);
    ok(*(DWORD *)wirehmfp == 2, "wirestgm + 0x10 should be 2 instead of 0x%08x\n", *(DWORD *)wirehmfp);
    wirehmfp += sizeof(DWORD);
    ok(*(DWORD *)wirehmfp == USER_MARSHAL_PTR_PREFIX, "wirestgm + 0x14 should be \"User\" instead of 0x%08x\n", *(DWORD *)wirehmfp);
    wirehmfp += sizeof(DWORD);
    ok(*(DWORD *)wirehmfp == WDT_REMOTE_CALL, "wirestgm + 0x18 should be WDT_REMOTE_CALL instead of 0x%08x\n", *(DWORD *)wirehmfp);
    wirehmfp += sizeof(DWORD);
    pmfp = GlobalLock(hmfp);
    ok(*(DWORD *)wirehmfp == (DWORD)(DWORD_PTR)pmfp->hMF, "wirestgm + 0x1c should be pmfp->hMF instead of 0x%08x\n", *(DWORD *)wirehmfp);
    GlobalUnlock(hmfp);
    wirehmfp += sizeof(DWORD);
    /* Note use (buffer_end - buffer) instead of size here, because size is an
     * overestimate with native */
    ok(*(DWORD *)wirehmfp == (buffer_end - buffer - 0x28), "wirestgm + 0x20 should be size - 0x34 instead of 0x%08x\n", *(DWORD *)wirehmfp);
    wirehmfp += sizeof(DWORD);
    ok(*(DWORD *)wirehmfp == (buffer_end - buffer - 0x28), "wirestgm + 0x24 should be size - 0x34 instead of 0x%08x\n", *(DWORD *)wirehmfp);
    wirehmfp += sizeof(DWORD);
    ok(*(WORD *)wirehmfp == 1, "wirehmfp + 0x28 should be 1 instead of 0x%08x\n", *(DWORD *)wirehmfp);
    wirehmfp += sizeof(DWORD);
    /* ... rest of data not tested - refer to tests for GetMetaFileBits
     * at this point */

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_DIFFERENTMACHINE);
    HMETAFILEPICT_UserUnmarshal(&umcb.Flags, buffer, &hmfp2);
    ok(hmfp2 != NULL, "HMETAFILEPICT didn't unmarshal\n");
    HeapFree(GetProcessHeap(), 0, buffer);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
    HMETAFILEPICT_UserFree(&umcb.Flags, &hmfp2);
    pmfp = GlobalLock(hmfp);
    DeleteMetaFile(pmfp->hMF);
    GlobalUnlock(hmfp);
    GlobalFree(hmfp);

    /* test NULL emf */
    hmfp = NULL;

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
    size = HMETAFILEPICT_UserSize(&umcb.Flags, 0, &hmfp);
    ok(size == 8, "size should be 8 bytes, not %d\n", size);
    buffer = HeapAlloc(GetProcessHeap(), 0, size);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_DIFFERENTMACHINE);
    HMETAFILEPICT_UserMarshal(&umcb.Flags, buffer, &hmfp);
    wirehmfp = buffer;
    ok(*(DWORD *)wirehmfp == WDT_REMOTE_CALL, "wirestgm + 0x0 should be WDT_REMOTE_CALL instead of 0x%08x\n", *(DWORD *)wirehmfp);
    wirehmfp += sizeof(DWORD);
    ok(*(DWORD *)wirehmfp == (DWORD)(DWORD_PTR)hmfp, "wirestgm + 0x4 should be hmf instead of 0x%08x\n", *(DWORD *)wirehmfp);
    wirehmfp += sizeof(DWORD);

    hmfp2 = NULL;
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_DIFFERENTMACHINE);
    HMETAFILEPICT_UserUnmarshal(&umcb.Flags, buffer, &hmfp2);
    ok(hmfp2 == NULL, "NULL HMETAFILE didn't unmarshal\n");
    HeapFree(GetProcessHeap(), 0, buffer);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
    HMETAFILEPICT_UserFree(&umcb.Flags, &hmfp2);
}

static HRESULT WINAPI Test_IUnknown_QueryInterface(
                                                   LPUNKNOWN iface,
                                                   REFIID riid,
                                                   LPVOID *ppvObj)
{
    if (ppvObj == NULL) return E_POINTER;

    if (IsEqualGUID(riid, &IID_IUnknown))
    {
        *ppvObj = iface;
        IUnknown_AddRef(iface);
        return S_OK;
    }

    *ppvObj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI Test_IUnknown_AddRef(LPUNKNOWN iface)
{
    return 2; /* non-heap-based object */
}

static ULONG WINAPI Test_IUnknown_Release(LPUNKNOWN iface)
{
    return 1; /* non-heap-based object */
}

static const IUnknownVtbl TestUnknown_Vtbl =
{
    Test_IUnknown_QueryInterface,
    Test_IUnknown_AddRef,
    Test_IUnknown_Release,
};

static IUnknown Test_Unknown = { &TestUnknown_Vtbl };

ULONG __RPC_USER WdtpInterfacePointer_UserSize(ULONG *, ULONG, ULONG, IUnknown *, REFIID);
unsigned char * __RPC_USER WdtpInterfacePointer_UserMarshal(ULONG *, ULONG, unsigned char *, IUnknown *, REFIID);
unsigned char * __RPC_USER WdtpInterfacePointer_UserUnmarshal(ULONG *, unsigned char *, IUnknown **, REFIID);
void __RPC_USER WdtpInterfacePointer_UserFree(IUnknown *);

static void test_marshal_WdtpInterfacePointer(void)
{
    USER_MARSHAL_CB umcb;
    MIDL_STUB_MESSAGE stub_msg;
    RPC_MESSAGE rpc_msg;
    unsigned char *buffer, *buffer_end;
    ULONG size;
    IUnknown *unk;
    IUnknown *unk2;
    unsigned char *wireip;
    const IID *iid;

    /* shows that the WdtpInterfacePointer functions don't marshal anything for
     * NULL pointers, so code using these functions must handle that case
     * itself */
    unk = NULL;
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_INPROC);
    size = WdtpInterfacePointer_UserSize(&umcb.Flags, umcb.Flags, 0, unk, &IID_IUnknown);
    ok(size == 0, "size should be 0 bytes, not %d\n", size);
    buffer = HeapAlloc(GetProcessHeap(), 0, size);
    buffer_end = WdtpInterfacePointer_UserMarshal(&umcb.Flags, umcb.Flags, buffer, unk, &IID_IUnknown);
    wireip = buffer;
    HeapFree(GetProcessHeap(), 0, buffer);

    unk = &Test_Unknown;
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_INPROC);
    size = WdtpInterfacePointer_UserSize(&umcb.Flags, umcb.Flags, 0, unk, &IID_IUnknown);
    todo_wine
    ok(size > 28, "size should be > 28 bytes, not %d\n", size);
    trace("WdtpInterfacePointer_UserSize returned %d\n", size);
    buffer = HeapAlloc(GetProcessHeap(), 0, size);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_INPROC);
    buffer_end = WdtpInterfacePointer_UserMarshal(&umcb.Flags, umcb.Flags, buffer, unk, &IID_IUnknown);
    wireip = buffer;
    if (size >= 28)
    {
        ok(*(DWORD *)wireip == 0x44, "wireip + 0x0 should be 0x44 instead of 0x%08x\n", *(DWORD *)wireip);
        wireip += sizeof(DWORD);
        ok(*(DWORD *)wireip == 0x44, "wireip + 0x4 should be 0x44 instead of 0x%08x\n", *(DWORD *)wireip);
        wireip += sizeof(DWORD);
        ok(*(DWORD *)wireip == 0x574f454d /* 'MEOW' */, "wireip + 0x8 should be 0x574f454d instead of 0x%08x\n", *(DWORD *)wireip);
        wireip += sizeof(DWORD);
        ok(*(DWORD *)wireip == 0x1, "wireip + 0xc should be 0x1 instead of 0x%08x\n", *(DWORD *)wireip);
        wireip += sizeof(DWORD);
        iid = (const IID *)wireip;
        ok(IsEqualIID(iid, &IID_IUnknown),
           "wireip + 0x10 should be IID_IUnknown instead of {%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}\n",
           iid->Data1, iid->Data2, iid->Data3,
           iid->Data4[0], iid->Data4[1], iid->Data4[2], iid->Data4[3],
           iid->Data4[4], iid->Data4[5], iid->Data4[6], iid->Data4[7]);
        wireip += sizeof(IID);
        ok(*(DWORD *)wireip == 0, "wireip + 0x1c should be 0 instead of 0x%08x\n", *(DWORD *)wireip);
        wireip += sizeof(DWORD);
        ok(*(DWORD *)wireip == 5, "wireip + 0x20 should be 5 instead of %d\n", *(DWORD *)wireip);
        wireip += sizeof(DWORD);
        /* the rest is dynamic so can't really be tested */
    }

    unk2 = NULL;
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_INPROC);
    WdtpInterfacePointer_UserUnmarshal(&umcb.Flags, buffer, &unk2, &IID_IUnknown);
    todo_wine
    ok(unk2 != NULL, "IUnknown object didn't unmarshal properly\n");
    HeapFree(GetProcessHeap(), 0, buffer);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_INPROC);
    WdtpInterfacePointer_UserFree(unk2);
}

START_TEST(usrmarshal)
{
    CoInitialize(NULL);

    test_marshal_CLIPFORMAT();
    test_marshal_HWND();
    test_marshal_HGLOBAL();
    test_marshal_HENHMETAFILE();
    test_marshal_HMETAFILE();
    test_marshal_HMETAFILEPICT();
    test_marshal_WdtpInterfacePointer();

    CoUninitialize();
}
