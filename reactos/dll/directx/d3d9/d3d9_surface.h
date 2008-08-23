#ifndef _D3D9_SURFACE_H_
#define _D3D9_SURFACE_H_

#include "d3d9_baseobject.h"

typedef struct _D3D9BaseSurface
{
/* 0x0000 */    LPDWORD lpVtbl;
/* 0x0024 */    D3DFORMAT DisplayFormat2;   // Back buffer format?
/* 0x0028 */    DWORD dwUnknown0028;    // Constant/ref count? (1)
/* 0x002c */    DWORD dwUnknown002c;    // Refresh rate?
/* 0x0030 */    D3DPOOL SurfacePool;
/* 0x0034 */    DWORD dwUnknown0034;
/* 0x0038 */    DWORD dwUnknown0038;
/* 0x003c */    DWORD dwWidth;
/* 0x0040 */    DWORD dwHeight;
/* 0x0044 */    D3DPOOL ResourcePool;
/* 0x0048 */    D3DFORMAT DisplayFormat;
/* 0x004c */    DWORD dwUnknown004c;
/* 0x0050 */    DWORD dwUnknown0050;
/* 0x0054 */    DWORD dwUnknown0054;
/* 0x0058 */    DWORD dwBpp;
} D3D9BaseSurface;

typedef struct _D3D9Surface
{
/* 0x0000 */    D3D9BaseObject BaseObject;
/* 0x0020 */    D3D9BaseSurface BaseSurface;
} D3D9Surface;

typedef struct _D3D9DriverSurface
{
/* 0x0000 */    D3D9Surface BaseD3D9Surface;
/* 0x005c */    DWORD dwUnknown5c;
/* 0x0060 */    DWORD dwUnknown60;
/* 0x0064 */    struct _D3D9DriverSurface* pPreviousDriverSurface;
/* 0x0068 */    struct _D3D9DriverSurface* pNextDriverSurface;
/* 0x006c */    DWORD dwUnknown6c[8];
} D3D9DriverSurface;

#endif // _D3D9_SURFACE_H_
