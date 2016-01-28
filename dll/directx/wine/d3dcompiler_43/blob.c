/*
 * Direct3D blob file
 *
 * Copyright 2010 Rico Schüller
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
 *
 */

#include "d3dcompiler_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3dcompiler);

struct d3dcompiler_blob
{
    ID3DBlob ID3DBlob_iface;
    LONG refcount;

    SIZE_T size;
    void *data;
};

static inline struct d3dcompiler_blob *impl_from_ID3DBlob(ID3DBlob *iface)
{
    return CONTAINING_RECORD(iface, struct d3dcompiler_blob, ID3DBlob_iface);
}

/* IUnknown methods */

static HRESULT STDMETHODCALLTYPE d3dcompiler_blob_QueryInterface(ID3DBlob *iface, REFIID riid, void **object)
{
    TRACE("iface %p, riid %s, object %p\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_ID3D10Blob)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        IUnknown_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE\n", debugstr_guid(riid));

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d3dcompiler_blob_AddRef(ID3DBlob *iface)
{
    struct d3dcompiler_blob *blob = impl_from_ID3DBlob(iface);
    ULONG refcount = InterlockedIncrement(&blob->refcount);

    TRACE("%p increasing refcount to %u\n", blob, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d3dcompiler_blob_Release(ID3DBlob *iface)
{
    struct d3dcompiler_blob *blob = impl_from_ID3DBlob(iface);
    ULONG refcount = InterlockedDecrement(&blob->refcount);

    TRACE("%p decreasing refcount to %u\n", blob, refcount);

    if (!refcount)
    {
        HeapFree(GetProcessHeap(), 0, blob->data);
        HeapFree(GetProcessHeap(), 0, blob);
    }

    return refcount;
}

/* ID3DBlob methods */

static void * STDMETHODCALLTYPE d3dcompiler_blob_GetBufferPointer(ID3DBlob *iface)
{
    struct d3dcompiler_blob *blob = impl_from_ID3DBlob(iface);

    TRACE("iface %p\n", iface);

    return blob->data;
}

static SIZE_T STDMETHODCALLTYPE d3dcompiler_blob_GetBufferSize(ID3DBlob *iface)
{
    struct d3dcompiler_blob *blob = impl_from_ID3DBlob(iface);

    TRACE("iface %p\n", iface);

    return blob->size;
}

static const struct ID3D10BlobVtbl d3dcompiler_blob_vtbl =
{
    /* IUnknown methods */
    d3dcompiler_blob_QueryInterface,
    d3dcompiler_blob_AddRef,
    d3dcompiler_blob_Release,
    /* ID3DBlob methods */
    d3dcompiler_blob_GetBufferPointer,
    d3dcompiler_blob_GetBufferSize,
};

static HRESULT d3dcompiler_blob_init(struct d3dcompiler_blob *blob, SIZE_T data_size)
{
    blob->ID3DBlob_iface.lpVtbl = &d3dcompiler_blob_vtbl;
    blob->refcount = 1;
    blob->size = data_size;

    blob->data = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, data_size);
    if (!blob->data)
    {
        ERR("Failed to allocate D3D blob data memory\n");
        return E_OUTOFMEMORY;
    }

    return S_OK;
}

HRESULT WINAPI D3DCreateBlob(SIZE_T data_size, ID3DBlob **blob)
{
    struct d3dcompiler_blob *object;
    HRESULT hr;

    TRACE("data_size %lu, blob %p\n", data_size, blob);

    if (!blob)
    {
        WARN("Invalid blob specified.\n");
        return D3DERR_INVALIDCALL;
    }

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    hr = d3dcompiler_blob_init(object, data_size);
    if (FAILED(hr))
    {
        WARN("Failed to initialize blob, hr %#x.\n", hr);
        HeapFree(GetProcessHeap(), 0, object);
        return hr;
    }

    *blob = &object->ID3DBlob_iface;

    TRACE("Created ID3DBlob %p\n", *blob);

    return S_OK;
}

static BOOL check_blob_part(DWORD tag, D3D_BLOB_PART part)
{
    BOOL add = FALSE;

    switch(part)
    {
        case D3D_BLOB_INPUT_SIGNATURE_BLOB:
            if (tag == TAG_ISGN) add = TRUE;
            break;

        case D3D_BLOB_OUTPUT_SIGNATURE_BLOB:
            if (tag == TAG_OSGN || tag == TAG_OSG5) add = TRUE;
            break;

        case D3D_BLOB_INPUT_AND_OUTPUT_SIGNATURE_BLOB:
            if (tag == TAG_ISGN || tag == TAG_OSGN || tag == TAG_OSG5) add = TRUE;
            break;

        case D3D_BLOB_PATCH_CONSTANT_SIGNATURE_BLOB:
            if (tag == TAG_PCSG) add = TRUE;
            break;

        case D3D_BLOB_ALL_SIGNATURE_BLOB:
            if (tag == TAG_ISGN || tag == TAG_OSGN || tag == TAG_OSG5 || tag == TAG_PCSG) add = TRUE;
            break;

        case D3D_BLOB_DEBUG_INFO:
            if (tag == TAG_SDBG) add = TRUE;
            break;

        case D3D_BLOB_LEGACY_SHADER:
            if (tag == TAG_Aon9) add = TRUE;
            break;

        case D3D_BLOB_XNA_PREPASS_SHADER:
            if (tag == TAG_XNAP) add = TRUE;
            break;

        case D3D_BLOB_XNA_SHADER:
            if (tag == TAG_XNAS) add = TRUE;
            break;

        default:
            FIXME("Unhandled D3D_BLOB_PART %s.\n", debug_d3dcompiler_d3d_blob_part(part));
            break;
    }

    TRACE("%s tag %s\n", add ? "Add" : "Skip", debugstr_an((const char *)&tag, 4));

    return add;
}

static HRESULT d3dcompiler_get_blob_part(const void *data, SIZE_T data_size, D3D_BLOB_PART part, UINT flags, ID3DBlob **blob)
{
    struct dxbc src_dxbc, dst_dxbc;
    HRESULT hr;
    unsigned int i, count;

    if (!data || !data_size || flags || !blob)
    {
        WARN("Invalid arguments: data %p, data_size %lu, flags %#x, blob %p\n", data, data_size, flags, blob);
        return D3DERR_INVALIDCALL;
    }

    if (part > D3D_BLOB_TEST_COMPILE_PERF
            || (part < D3D_BLOB_TEST_ALTERNATE_SHADER && part > D3D_BLOB_XNA_SHADER))
    {
        WARN("Invalid D3D_BLOB_PART: part %s\n", debug_d3dcompiler_d3d_blob_part(part));
        return D3DERR_INVALIDCALL;
    }

    hr = dxbc_parse(data, data_size, &src_dxbc);
    if (FAILED(hr))
    {
        WARN("Failed to parse blob part\n");
        return hr;
    }

    hr = dxbc_init(&dst_dxbc, 0);
    if (FAILED(hr))
    {
        dxbc_destroy(&src_dxbc);
        WARN("Failed to init dxbc\n");
        return hr;
    }

    for (i = 0; i < src_dxbc.count; ++i)
    {
        struct dxbc_section *section = &src_dxbc.sections[i];

        if (check_blob_part(section->tag, part))
        {
            hr = dxbc_add_section(&dst_dxbc, section->tag, section->data, section->data_size);
            if (FAILED(hr))
            {
                dxbc_destroy(&src_dxbc);
                dxbc_destroy(&dst_dxbc);
                WARN("Failed to add section to dxbc\n");
                return hr;
            }
        }
    }

    count = dst_dxbc.count;

    switch(part)
    {
        case D3D_BLOB_INPUT_SIGNATURE_BLOB:
        case D3D_BLOB_OUTPUT_SIGNATURE_BLOB:
        case D3D_BLOB_PATCH_CONSTANT_SIGNATURE_BLOB:
        case D3D_BLOB_DEBUG_INFO:
        case D3D_BLOB_LEGACY_SHADER:
        case D3D_BLOB_XNA_PREPASS_SHADER:
        case D3D_BLOB_XNA_SHADER:
            if (count != 1) count = 0;
            break;

        case D3D_BLOB_INPUT_AND_OUTPUT_SIGNATURE_BLOB:
            if (count != 2) count = 0;
            break;

        case D3D_BLOB_ALL_SIGNATURE_BLOB:
            if (count != 3) count = 0;
            break;

        default:
            FIXME("Unhandled D3D_BLOB_PART %s.\n", debug_d3dcompiler_d3d_blob_part(part));
            break;
    }

    if (count == 0)
    {
        dxbc_destroy(&src_dxbc);
        dxbc_destroy(&dst_dxbc);
        WARN("Nothing to write into the blob (count = 0)\n");
        return E_FAIL;
    }

    /* some parts aren't full DXBCs, they contain only the data */
    if (count == 1 && (part == D3D_BLOB_DEBUG_INFO || part == D3D_BLOB_LEGACY_SHADER || part == D3D_BLOB_XNA_PREPASS_SHADER
            || part == D3D_BLOB_XNA_SHADER))
    {
        hr = D3DCreateBlob(dst_dxbc.sections[0].data_size, blob);
        if (SUCCEEDED(hr))
        {
            memcpy(ID3D10Blob_GetBufferPointer(*blob), dst_dxbc.sections[0].data, dst_dxbc.sections[0].data_size);
        }
        else
        {
            WARN("Could not create blob\n");
        }
    }
    else
    {
        hr = dxbc_write_blob(&dst_dxbc, blob);
        if (FAILED(hr))
        {
            WARN("Failed to write blob part\n");
        }
    }

    dxbc_destroy(&src_dxbc);
    dxbc_destroy(&dst_dxbc);

    return hr;
}

static BOOL check_blob_strip(DWORD tag, UINT flags)
{
    BOOL add = TRUE;

    if (flags & D3DCOMPILER_STRIP_TEST_BLOBS) FIXME("Unhandled flag D3DCOMPILER_STRIP_TEST_BLOBS.\n");

    switch(tag)
    {
        case TAG_RDEF:
        case TAG_STAT:
            if (flags & D3DCOMPILER_STRIP_REFLECTION_DATA) add = FALSE;
            break;

        case TAG_SDBG:
            if (flags & D3DCOMPILER_STRIP_DEBUG_INFO) add = FALSE;
            break;

        default:
            break;
    }

    TRACE("%s tag %s\n", add ? "Add" : "Skip", debugstr_an((const char *)&tag, 4));

    return add;
}

static HRESULT d3dcompiler_strip_shader(const void *data, SIZE_T data_size, UINT flags, ID3DBlob **blob)
{
    struct dxbc src_dxbc, dst_dxbc;
    HRESULT hr;
    unsigned int i;

    if (!blob)
    {
        WARN("NULL for blob specified\n");
        return E_FAIL;
    }

    if (!data || !data_size)
    {
        WARN("Invalid arguments: data %p, data_size %lu\n", data, data_size);
        return D3DERR_INVALIDCALL;
    }

    hr = dxbc_parse(data, data_size, &src_dxbc);
    if (FAILED(hr))
    {
        WARN("Failed to parse blob part\n");
        return hr;
    }

    /* src_dxbc.count >= dst_dxbc.count */
    hr = dxbc_init(&dst_dxbc, src_dxbc.count);
    if (FAILED(hr))
    {
        dxbc_destroy(&src_dxbc);
        WARN("Failed to init dxbc\n");
        return hr;
    }

    for (i = 0; i < src_dxbc.count; ++i)
    {
        struct dxbc_section *section = &src_dxbc.sections[i];

        if (check_blob_strip(section->tag, flags))
        {
            hr = dxbc_add_section(&dst_dxbc, section->tag, section->data, section->data_size);
            if (FAILED(hr))
            {
                dxbc_destroy(&src_dxbc);
                dxbc_destroy(&dst_dxbc);
                WARN("Failed to add section to dxbc\n");
                return hr;
            }
        }
    }

    hr = dxbc_write_blob(&dst_dxbc, blob);
    if (FAILED(hr))
    {
        WARN("Failed to write blob part\n");
    }

    dxbc_destroy(&src_dxbc);
    dxbc_destroy(&dst_dxbc);

    return hr;
}

HRESULT WINAPI D3DGetBlobPart(const void *data, SIZE_T data_size, D3D_BLOB_PART part, UINT flags, ID3DBlob **blob)
{
    TRACE("data %p, data_size %lu, part %s, flags %#x, blob %p\n", data,
           data_size, debug_d3dcompiler_d3d_blob_part(part), flags, blob);

    return d3dcompiler_get_blob_part(data, data_size, part, flags, blob);
}

HRESULT WINAPI D3DGetInputSignatureBlob(const void *data, SIZE_T data_size, ID3DBlob **blob)
{
    TRACE("data %p, data_size %lu, blob %p\n", data, data_size, blob);

    return d3dcompiler_get_blob_part(data, data_size, D3D_BLOB_INPUT_SIGNATURE_BLOB, 0, blob);
}

HRESULT WINAPI D3DGetOutputSignatureBlob(const void *data, SIZE_T data_size, ID3DBlob **blob)
{
    TRACE("data %p, data_size %lu, blob %p\n", data, data_size, blob);

    return d3dcompiler_get_blob_part(data, data_size, D3D_BLOB_OUTPUT_SIGNATURE_BLOB, 0, blob);
}

HRESULT WINAPI D3DGetInputAndOutputSignatureBlob(const void *data, SIZE_T data_size, ID3DBlob **blob)
{
    TRACE("data %p, data_size %lu, blob %p\n", data, data_size, blob);

    return d3dcompiler_get_blob_part(data, data_size, D3D_BLOB_INPUT_AND_OUTPUT_SIGNATURE_BLOB, 0, blob);
}

HRESULT WINAPI D3DGetDebugInfo(const void *data, SIZE_T data_size, ID3DBlob **blob)
{
    TRACE("data %p, data_size %lu, blob %p\n", data, data_size, blob);

    return d3dcompiler_get_blob_part(data, data_size, D3D_BLOB_DEBUG_INFO, 0, blob);
}

HRESULT WINAPI D3DStripShader(const void *data, SIZE_T data_size, UINT flags, ID3D10Blob **blob)
{
    TRACE("data %p, data_size %lu, flags %#x, blob %p\n", data, data_size, flags, blob);

    return d3dcompiler_strip_shader(data, data_size, flags, blob);
}

HRESULT WINAPI D3DReadFileToBlob(const WCHAR *filename, ID3DBlob **contents)
{
    FIXME("filename %s, contents %p\n", debugstr_w(filename), contents);

    return E_NOTIMPL;
}

HRESULT WINAPI D3DWriteBlobToFile(ID3DBlob* blob, const WCHAR *filename, BOOL overwrite)
{
    FIXME("blob %p, filename %s, overwrite %d\n", blob, debugstr_w(filename), overwrite);

    return E_NOTIMPL;
}
