
#ifndef __WIN32K_DRIVER_H
#define __WIN32K_DRIVER_H

#include <ddk/winddi.h>

typedef BOOL STDCALL_FUNC (*PGD_ENABLEDRIVER)(ULONG, ULONG, PDRVENABLEDATA);
typedef DHPDEV STDCALL_FUNC (*PGD_ENABLEPDEV)(DEVMODEW  *,
                                 LPWSTR,
                                 ULONG,
                                 HSURF  *,
                                 ULONG,
                                 ULONG  *,
                                 ULONG,
                                 DEVINFO  *,
                                 LPWSTR,
                                 LPWSTR,
                                 HANDLE);
typedef VOID STDCALL_FUNC (*PGD_COMPLETEPDEV)(DHPDEV, HDEV);
typedef VOID STDCALL_FUNC (*PGD_DISABLEPDEV)(DHPDEV); 
typedef HSURF STDCALL_FUNC (*PGD_ENABLESURFACE)(DHPDEV);
typedef VOID STDCALL_FUNC (*PGD_DISABLESURFACE)(DHPDEV);
typedef BOOL STDCALL_FUNC (*PGD_ASSERTMODE)(DHPDEV, BOOL);
typedef BOOL STDCALL_FUNC (*PGD_RESETPDEV)(DHPDEV, DHPDEV);
typedef HBITMAP STDCALL_FUNC (*PGD_CREATEDEVICEBITMAP)(DHPDEV, SIZEL, ULONG); 
typedef VOID STDCALL_FUNC (*PGD_DELETEDEVICEBITMAP)(DHSURF); 
typedef BOOL STDCALL_FUNC (*PGD_REALIZEBRUSH)(BRUSHOBJ*, SURFOBJ*, SURFOBJ*, SURFOBJ*,
                                 XLATEOBJ*, ULONG); 
typedef ULONG STDCALL_FUNC (*PGD_DITHERCOLOR)(DHPDEV, ULONG, ULONG, PULONG); 
typedef BOOL STDCALL_FUNC (*PGD_STROKEPATH)(SURFOBJ*, PATHOBJ*, CLIPOBJ*, XFORMOBJ*,
                               BRUSHOBJ*, POINTL*, PLINEATTRS, MIX); 
typedef BOOL STDCALL_FUNC (*PGD_FILLPATH)(SURFOBJ*, PATHOBJ*, CLIPOBJ*, BRUSHOBJ*,
                             POINTL*, MIX, ULONG); 
typedef BOOL STDCALL_FUNC (*PGD_STROKEANDFILLPATH)(SURFOBJ*, PATHOBJ*, CLIPOBJ*,
                                      XFORMOBJ*, BRUSHOBJ*, PLINEATTRS,
                                      BRUSHOBJ*, POINTL*, MIX, ULONG); 
typedef BOOL STDCALL_FUNC (*PGD_PAINT)(SURFOBJ*, CLIPOBJ*, BRUSHOBJ*, POINTL*, MIX); 
typedef BOOL STDCALL_FUNC (*PGD_BITBLT)(SURFOBJ*, SURFOBJ*, SURFOBJ*, CLIPOBJ*,
                           XLATEOBJ*, RECTL*, POINTL*, POINTL*, BRUSHOBJ*,
                           POINTL*, ROP4); 
typedef BOOL STDCALL_FUNC (*PGD_TRANSPARENTBLT)(SURFOBJ*, SURFOBJ*, CLIPOBJ*, XLATEOBJ*, RECTL*, RECTL*, ULONG, ULONG);
typedef BOOL STDCALL_FUNC (*PGD_COPYBITS)(SURFOBJ*, SURFOBJ*, CLIPOBJ*, XLATEOBJ*,
                             RECTL*, POINTL*); 
typedef BOOL STDCALL_FUNC (*PGD_STRETCHBLT)(SURFOBJ*, SURFOBJ*, SURFOBJ*, CLIPOBJ*,
                               XLATEOBJ*, COLORADJUSTMENT*, POINTL*,
                               RECTL*, RECTL*, POINTL*, ULONG); 
typedef BOOL STDCALL_FUNC (*PGD_SETPALETTE)(DHPDEV, PALOBJ*, ULONG, ULONG, ULONG); 
typedef BOOL STDCALL_FUNC (*PGD_TEXTOUT)(SURFOBJ*, STROBJ*, FONTOBJ*, CLIPOBJ*, RECTL*,
                            RECTL*, BRUSHOBJ*, BRUSHOBJ*, POINTL*, MIX); 
typedef ULONG STDCALL_FUNC (*PGD_ESCAPE)(SURFOBJ*, ULONG, ULONG, PVOID *, ULONG, PVOID *); 
typedef ULONG STDCALL_FUNC (*PGD_DRAWESCAPE)(SURFOBJ*, ULONG, CLIPOBJ*, RECTL*, ULONG, 
                                PVOID *); 
typedef PIFIMETRICS STDCALL_FUNC (*PGD_QUERYFONT)(DHPDEV, ULONG, ULONG, PULONG); 
typedef PVOID STDCALL_FUNC (*PGD_QUERYFONTTREE)(DHPDEV, ULONG, ULONG, ULONG, PULONG); 
typedef LONG STDCALL_FUNC (*PGD_QUERYFONTDATA)(DHPDEV, FONTOBJ*, ULONG, HGLYPH, GLYPHDATA*,
                                  PVOID, ULONG); 
typedef ULONG STDCALL_FUNC (*PGD_SETPOINTERSHAPE)(SURFOBJ*, SURFOBJ*, SURFOBJ*, XLATEOBJ*,
                                     LONG, LONG, LONG, LONG, RECTL*, ULONG); 
typedef VOID STDCALL_FUNC (*PGD_MOVEPOINTER)(SURFOBJ*, LONG, LONG, RECTL*); 
typedef BOOL STDCALL_FUNC (*PGD_LINETO)(SURFOBJ*, CLIPOBJ*, BRUSHOBJ*, LONG, LONG, LONG,
                           LONG, RECTL*, MIX);
typedef BOOL STDCALL_FUNC (*PGD_SENDPAGE)(SURFOBJ*);
typedef BOOL STDCALL_FUNC (*PGD_STARTPAGE)(SURFOBJ*);
typedef BOOL STDCALL_FUNC (*PGD_ENDDOC)(SURFOBJ*, ULONG);
typedef BOOL STDCALL_FUNC (*PGD_STARTDOC)(SURFOBJ*, PWSTR, DWORD);
typedef ULONG STDCALL_FUNC (*PGD_GETGLYPHMODE)(DHPDEV, FONTOBJ*);
typedef VOID STDCALL_FUNC (*PGD_SYNCHRONIZE)(DHPDEV, RECTL*);
typedef ULONG STDCALL_FUNC (*PGD_SAVESCREENBITS)(SURFOBJ*, ULONG, ULONG, RECTL*);
typedef ULONG STDCALL_FUNC (*PGD_GETMODES)(HANDLE, ULONG, PDEVMODEW);
typedef VOID STDCALL_FUNC (*PGD_FREE)(PVOID, ULONG);
typedef VOID STDCALL_FUNC (*PGD_DESTROYFONT)(FONTOBJ*);
typedef LONG STDCALL_FUNC (*PGD_QUERYFONTCAPS)(ULONG, PULONG);
typedef ULONG STDCALL_FUNC (*PGD_LOADFONTFILE)(ULONG, PVOID, ULONG, ULONG);
typedef BOOL STDCALL_FUNC (*PGD_UNLOADFONTFILE)(ULONG);
typedef ULONG STDCALL_FUNC (*PGD_FONTMANAGEMENT)(SURFOBJ*, FONTOBJ*, ULONG, ULONG, PVOID,
                                    ULONG, PVOID);
typedef LONG STDCALL_FUNC (*PGD_QUERYTRUETYPETABLE)(ULONG, ULONG, ULONG, PTRDIFF, ULONG,
                                       PBYTE);
typedef LONG STDCALL_FUNC (*PGD_QUERYTRUETYPEOUTLINE)(DHPDEV, FONTOBJ*, HGLYPH, BOOL,
                                         GLYPHDATA*, ULONG, TTPOLYGONHEADER*);
typedef PVOID STDCALL_FUNC (*PGD_GETTRUETYPEFILE)(ULONG, PULONG);
typedef LONG STDCALL_FUNC (*PGD_QUERYFONTFILE)(ULONG, ULONG, ULONG, PULONG);
typedef BOOL STDCALL_FUNC (*PGD_QUERYADVANCEWIDTHS)(DHPDEV, FONTOBJ*, ULONG, HGLYPH *,
                                       PVOID *, ULONG);
typedef BOOL STDCALL_FUNC (*PGD_SETPIXELFORMAT)(SURFOBJ*, LONG, ULONG);
typedef LONG STDCALL_FUNC (*PGD_DESCRIBEPIXELFORMAT)(DHPDEV, LONG, ULONG,
                                        PPIXELFORMATDESCRIPTOR);
typedef BOOL STDCALL_FUNC (*PGD_SWAPBUFFERS)(SURFOBJ*, PWNDOBJ);
typedef BOOL STDCALL_FUNC (*PGD_STARTBANDING)(SURFOBJ*, POINTL*);
typedef BOOL STDCALL_FUNC (*PGD_NEXTBAND)(SURFOBJ*, POINTL*);

typedef BOOL STDCALL_FUNC (*PGD_GETDIRECTDRAWINFO)(DHPDEV, PDD_HALINFO, PDWORD, VIDEOMEMORY*, PDWORD, PDWORD);
typedef BOOL STDCALL_FUNC (*PGD_ENABLEDIRECTDRAW)(DHPDEV, PDD_CALLBACKS, PDD_SURFACECALLBACKS, PDD_PALETTECALLBACKS);
typedef VOID STDCALL_FUNC (*PGD_DISABLEDIRECTDRAW)(DHPDEV);

typedef LONG STDCALL_FUNC (*PGD_QUERYSPOOLTYPE)(DHPDEV, LPWSTR);

typedef BOOL STDCALL_FUNC (*PGD_GRADIENTFILL)(SURFOBJ*, CLIPOBJ*, XLATEOBJ*, TRIVERTEX*, ULONG, PVOID, ULONG, RECTL*, POINTL*, ULONG);

typedef struct _DRIVER_FUNCTIONS
{
  PGD_ENABLEDRIVER  EnableDriver;
  PGD_ENABLEPDEV  EnablePDev;
  PGD_COMPLETEPDEV  CompletePDev;
  PGD_DISABLEPDEV  DisablePDev;
  PGD_ENABLESURFACE  EnableSurface;
  PGD_DISABLESURFACE  DisableSurface;
  PGD_ASSERTMODE  AssertMode;
  PGD_RESETPDEV  ResetPDev;
  PGD_CREATEDEVICEBITMAP  CreateDeviceBitmap;
  PGD_DELETEDEVICEBITMAP  DeleteDeviceBitmap;
  PGD_REALIZEBRUSH  RealizeBrush;
  PGD_DITHERCOLOR  DitherColor;
  PGD_STROKEPATH  StrokePath;
  PGD_FILLPATH  FillPath;
  PGD_STROKEANDFILLPATH  StrokeAndFillPath;
  PGD_PAINT  Paint;
  PGD_BITBLT  BitBlt;
  PGD_TRANSPARENTBLT TransparentBlt;
  PGD_COPYBITS  CopyBits;
  PGD_STRETCHBLT  StretchBlt;
  PGD_SETPALETTE  SetPalette;
  PGD_TEXTOUT  TextOut;
  PGD_ESCAPE  Escape;
  PGD_DRAWESCAPE  DrawEscape;
  PGD_QUERYFONT  QueryFont;
  PGD_QUERYFONTTREE  QueryFontTree;
  PGD_QUERYFONTDATA  QueryFontData;
  PGD_SETPOINTERSHAPE  SetPointerShape;
  PGD_MOVEPOINTER  MovePointer;
  PGD_LINETO  LineTo;
  PGD_SENDPAGE  SendPage;
  PGD_STARTPAGE  StartPage;
  PGD_ENDDOC  EndDoc;
  PGD_STARTDOC  StartDoc;
  PGD_GETGLYPHMODE  GetGlyphMode;
  PGD_SYNCHRONIZE  Synchronize;
  PGD_SAVESCREENBITS  SaveScreenBits;
  PGD_GETMODES  GetModes;
  PGD_FREE  Free;
  PGD_DESTROYFONT  DestroyFont;
  PGD_QUERYFONTCAPS  QueryFontCaps;
  PGD_LOADFONTFILE  LoadFontFile;
  PGD_UNLOADFONTFILE  UnloadFontFile;
  PGD_FONTMANAGEMENT  FontManagement;
  PGD_QUERYTRUETYPETABLE  QueryTrueTypeTable;
  PGD_QUERYTRUETYPEOUTLINE  QueryTrueTypeOutline;
  PGD_GETTRUETYPEFILE  GetTrueTypeFile;
  PGD_QUERYFONTFILE  QueryFontFile;
  PGD_QUERYADVANCEWIDTHS  QueryAdvanceWidths;
  PGD_SETPIXELFORMAT  SetPixelFormat;
  PGD_DESCRIBEPIXELFORMAT  DescribePixelFormat;
  PGD_SWAPBUFFERS  SwapBuffers;
  PGD_STARTBANDING  StartBanding;
  PGD_NEXTBAND  NextBand;
  PGD_GETDIRECTDRAWINFO  GetDirectDrawInfo;
  PGD_ENABLEDIRECTDRAW  EnableDirectDraw;
  PGD_DISABLEDIRECTDRAW  DisableDirectDraw;
  PGD_QUERYSPOOLTYPE  QuerySpoolType;
  PGD_GRADIENTFILL  GradientFill;
} DRIVER_FUNCTIONS, *PDRIVER_FUNCTIONS;

BOOL  DRIVER_RegisterDriver(LPCWSTR  Name, PGD_ENABLEDRIVER  EnableDriver);
PGD_ENABLEDRIVER  DRIVER_FindDDIDriver(LPCWSTR  Name);
PFILE_OBJECT DRIVER_FindMPDriver(ULONG  DisplayNumber);
BOOL  DRIVER_BuildDDIFunctions(PDRVENABLEDATA  DED, 
                               PDRIVER_FUNCTIONS  DF);
BOOL  DRIVER_UnregisterDriver(LPCWSTR  Name);
INT  DRIVER_ReferenceDriver (LPCWSTR  Name);
INT  DRIVER_UnreferenceDriver (LPCWSTR  Name);

#endif

