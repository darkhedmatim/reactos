#define WIN32_LEAN_AND_MEAN
#define __ROS_LONG64__
#include <windows.h>

#define STANDALONE
#include "wine/test.h"

extern void func_AddFontResource(void);
extern void func_AddFontResourceEx(void);
extern void func_BeginPath(void);
extern void func_CombineTransform(void);
extern void func_CreateBitmapIndirect(void);
extern void func_CreateCompatibleDC(void);
extern void func_CreateFont(void);
extern void func_CreateFontIndirect(void);
extern void func_CreatePen(void);
extern void func_CreateRectRgn(void);
extern void func_DPtoLP(void);
extern void func_EngAcquireSemaphore(void);
extern void func_EngCreateSemaphore(void);
extern void func_EngDeleteSemaphore(void);
extern void func_EngReleaseSemaphore(void);
extern void func_ExtCreatePen(void);
extern void func_GdiConvertBitmap(void);
extern void func_GdiConvertBrush(void);
extern void func_GdiConvertDC(void);
extern void func_GdiConvertFont(void);
extern void func_GdiConvertPalette(void);
extern void func_GdiConvertRegion(void);
extern void func_GdiDeleteLocalDC(void);
extern void func_GdiGetCharDimensions(void);
extern void func_GdiGetLocalBrush(void);
extern void func_GdiGetLocalDC(void);
extern void func_GdiReleaseLocalDC(void);
extern void func_GdiSetAttrs(void);
extern void func_GetClipRgn(void);
extern void func_GetCurrentObject(void);
extern void func_GetDIBits(void);
extern void func_GetPixel(void);
extern void func_GetObject(void);
extern void func_GetStockObject(void);
extern void func_GetTextExtentExPoint(void);
extern void func_GetTextFace(void);
extern void func_MaskBlt(void);
extern void func_Rectangle(void);
extern void func_SelectObject(void);
extern void func_SetDCPenColor(void);
extern void func_SetDIBits(void);
extern void func_SetMapMode(void);
extern void func_SetSysColors(void);
extern void func_SetWindowExtEx(void);
extern void func_SetWorldTransform(void);

const struct test winetest_testlist[] =
{
    { "AddFontResource", func_AddFontResource },
    { "AddFontResourceEx", func_AddFontResourceEx },
    { "BeginPath", func_BeginPath },
    { "CombineTransform", func_CombineTransform },
    { "CreateBitmapIndirect", func_CreateBitmapIndirect },
    { "CreateCompatibleDC", func_CreateCompatibleDC },
    { "CreateFont", func_CreateFont },
    { "CreateFontIndirect", func_CreateFontIndirect },
    { "CreatePen", func_CreatePen },
    { "CreateRectRgn", func_CreateRectRgn },
    { "DPtoLP", func_DPtoLP },
    { "EngAcquireSemaphore", func_EngAcquireSemaphore },
    { "EngCreateSemaphore", func_EngCreateSemaphore },
    { "EngDeleteSemaphore", func_EngDeleteSemaphore },
    { "EngReleaseSemaphore", func_EngReleaseSemaphore },
    { "ExtCreatePen", func_ExtCreatePen },
    { "GdiConvertBitmap", func_GdiConvertBitmap },
    { "GdiConvertBrush", func_GdiConvertBrush },
    { "GdiConvertDC", func_GdiConvertDC },
    { "GdiConvertFont", func_GdiConvertFont },
    { "GdiConvertPalette", func_GdiConvertPalette },
    { "GdiConvertRegion", func_GdiConvertRegion },
    { "GdiDeleteLocalDC", func_GdiDeleteLocalDC },
    { "GdiGetCharDimensions", func_GdiGetCharDimensions },
    { "GdiGetLocalBrush", func_GdiGetLocalBrush },
    { "GdiGetLocalDC", func_GdiGetLocalDC },
    { "GdiReleaseLocalDC", func_GdiReleaseLocalDC },
    { "GdiSetAttrs", func_GdiSetAttrs },
    { "GetClipRgn", func_GetClipRgn },
    { "GetCurrentObject", func_GetCurrentObject },
    { "GetDIBits", func_GetDIBits },
    { "GetPixel", func_GetPixel },
    { "GetObject", func_GetObject },
    { "GetStockObject", func_GetStockObject },
    { "GetTextExtentExPoint", func_GetTextExtentExPoint },
    { "GetTextFace", func_GetTextFace },
    { "MaskBlt", func_MaskBlt },
    { "Rectangle", func_Rectangle },
    { "SelectObject", func_SelectObject },
    { "SetDCPenColor", func_SetDCPenColor },
    { "SetDIBits", func_SetDIBits },
    { "SetMapMode", func_SetMapMode },
    { "SetSysColors", func_SetSysColors },
    { "SetWindowExtEx", func_SetWindowExtEx },
    { "SetWorldTransform", func_SetWorldTransform },

    { 0, 0 }
};

