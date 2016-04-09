/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         COM interface test
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#pragma once

/* Define this if you're adding new classes - the test will auto-generate the
 * interface table entries for you ;) */
//#define GENERATE_TABLE_ENTRIES

/* Define this to make wrong interface offsets count as test failures...
 * we usually don't want to be that strict */
//#define FAIL_WRONG_OFFSET

#define COBJMACROS
#define WIN32_NO_STATUS
#include <wine/test.h>
#include <objbase.h>
#include <objsafe.h>
#include <oleacc.h>
#include <oaidl.h>
#include <netcon.h>
#include <netcfgx.h>
#include <netcfgn.h>
#include <mshtmhst.h>
#include <perhist.h>
#include <shlguid.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <shldisp.h>
#include <shdeprecated.h>
#include <shlobj_undoc.h>
#include <shlguid_undoc.h>
#include <shlwapi.h>
#include <ndk/rtlfuncs.h>
#include <initguid.h>

typedef IUnknown *PUNKNOWN;

typedef struct _CLASS_AND_INTERFACES
{
    const CLSID *clsid;
    PCSTR name;
    struct
    {
        LONG offset;
        const IID *iid;
    } ifaces[80];
    PCWSTR ThreadingModel;
} CLASS_AND_INTERFACES;
typedef const CLASS_AND_INTERFACES *PCCLASS_AND_INTERFACES;

#define ID_NAME(c) &c, #c

VOID
TestClasses(
    _In_ PCWSTR ModuleName,
    _In_ PCCLASS_AND_INTERFACES ExpectedInterfaces,
    _In_ INT ExpectedInterfaceCount);

/* Indicate that the interface is implemented in another (probably aggregate) object,
 * so its offset varies and is "far away" */
#define FARAWY (-65535)

// TODO: fix our headers... we really shouldn't need these here
DEFINE_GUID(CLSID_ActiveDesktop,           0x75048700, 0xef1f, 0x11d0, 0x98, 0x88, 0x00, 0x60, 0x97, 0xde, 0xac, 0xf9);
DEFINE_GUID(CLSID_AugmentedShellFolder,    0x91ea3f8b, 0xc99b, 0x11d0, 0x98, 0x15, 0x00, 0xc0, 0x4f, 0xd9, 0x19, 0x72);
DEFINE_GUID(CLSID_AugmentedShellFolder2,   0x6413ba2c, 0xb461, 0x11d1, 0xa1, 0x8a, 0x08, 0x00, 0x36, 0xb1, 0x1a, 0x03);
DEFINE_GUID(CLSID_CopyToMenu,              0xC2FBB630, 0x2971, 0x11D1, 0xA1, 0x8C, 0x00, 0xC0, 0x4F, 0xD7, 0x5D, 0x13);
DEFINE_GUID(CLSID_CDBurn,                  0xfbeb8a05, 0xbeee, 0x4442, 0x80, 0x4e, 0x40, 0x9d, 0x6c, 0x45, 0x15, 0xe9);
DEFINE_GUID(CLSID_DeskMovr,                0x72267F6A, 0xA6F9, 0x11D0, 0xBC, 0x94, 0x00, 0xC0, 0x4F, 0xB6, 0x78, 0x63);
DEFINE_GUID(CLSID_FadeTask,                0x7EB5FBE4, 0x2100, 0x49E6, 0x85, 0x93, 0x17, 0xE1, 0x30, 0x12, 0x2F, 0x91);
DEFINE_GUID(CLSID_FileSearchBand,          0xc4ee31f3, 0x4768, 0x11d2, 0xbe, 0x5c, 0x00, 0xa0, 0xc9, 0xa8, 0x3d, 0xa1);
DEFINE_GUID(CLSID_FolderItem,              0xfef10fa2, 0x355e, 0x4e06, 0x93, 0x81, 0x9b, 0x24, 0xd7, 0xf7, 0xcc, 0x88);
DEFINE_GUID(CLSID_FolderItemsFDF,          0x53c74826, 0xab99, 0x4d33, 0xac, 0xa4, 0x31, 0x17, 0xf5, 0x1d, 0x37, 0x88);
DEFINE_GUID(CLSID_FolderViewHost,          0x20b1cb23, 0x6968, 0x4eb9, 0xb7, 0xd4, 0xa6, 0x6d, 0x00, 0xd0, 0x7c, 0xee);
DEFINE_GUID(CLSID_ISFBand,                 0xD82BE2B0, 0x5764, 0x11D0, 0xA9, 0x6E, 0x00, 0xC0, 0x4F, 0xD7, 0x05, 0xA2);
DEFINE_GUID(CLSID_MergedFolder,            0x26fdc864, 0xbe88, 0x46e7, 0x92, 0x35, 0x03, 0x2d, 0x8e, 0xa5, 0x16, 0x2e);
DEFINE_GUID(CLSID_MenuDeskBar,             0xECD4FC4F, 0x521C, 0x11D0, 0xB7, 0x92, 0x00, 0xA0, 0xC9, 0x03, 0x12, 0xE1);
DEFINE_GUID(CLSID_MenuToolbarBase,         0x40b96610, 0xb522, 0x11d1, 0xb3, 0xb4, 0x00, 0xaa, 0x00, 0x6e, 0xfd, 0xe7);
DEFINE_GUID(CLSID_MoveToMenu,              0xC2FBB631, 0x2971, 0x11D1, 0xA1, 0x8C, 0x00, 0xC0, 0x4F, 0xD7, 0x5D, 0x13);
DEFINE_GUID(CLSID_MruLongList,             0x53BD6B4E, 0x3780, 0x4693, 0xAF, 0xC3, 0x71, 0x61, 0xC2, 0xF3, 0xEE, 0x9C);
DEFINE_GUID(CLSID_QuickLinks,              0x0E5CBF21, 0xD15F, 0x11D0, 0x83, 0x01, 0x00, 0xAA, 0x00, 0x5B, 0x43, 0x83);
DEFINE_GUID(CLSID_SendToMenu,              0x7BA4C740, 0x9E81, 0x11CF, 0x99, 0xD3, 0x00, 0xAA, 0x00, 0x4A, 0xE8, 0x37);
DEFINE_GUID(CLSID_ShellFolderView,         0x62112aa1, 0xebe4, 0x11cf, 0xa5, 0xfb, 0x00, 0x20, 0xaf, 0xe7, 0x29, 0x2d);
DEFINE_GUID(CLSID_ShellImageDataFactory,   0x66e4e4fb, 0xf385, 0x4dd0, 0x8d, 0x74, 0xa2, 0xef, 0xd1, 0xbc, 0x61, 0x78);
DEFINE_GUID(CLSID_ShellLinkObject,         0x11219420, 0x1768, 0x11d1, 0x95, 0xbe, 0x00, 0x60, 0x97, 0x97, 0xea, 0x4f);
DEFINE_GUID(CLSID_StartMenuPin,            0xa2a9545d, 0xa0c2, 0x42b4, 0x97, 0x08, 0xa0, 0xb2, 0xba, 0xdd, 0x77, 0xc8);
DEFINE_GUID(CLSID_TrackShellMenu,          0x8278F931, 0x2A3E, 0x11d2, 0x83, 0x8F, 0x00, 0xC0, 0x4F, 0xD9, 0x18, 0xD0);

DEFINE_GUID(IID_IADesktopP2,               0xb22754e2, 0x4574, 0x11d1, 0x98, 0x88, 0x00, 0x60, 0x97, 0xde, 0xac, 0xf9);
DEFINE_GUID(IID_IAccessControl,            0xeedd23e0, 0x8410, 0x11ce, 0xa1, 0xc3, 0x08, 0x00, 0x2b, 0x2b, 0x8d, 0x8f);
DEFINE_GUID(IID_IAccessor,                 0x0c733a8c, 0x2a1c, 0x11ce, 0xad, 0xe5, 0x00, 0xaa, 0x00, 0x44, 0x77, 0x3d);
DEFINE_GUID(IID_IActionProgress,           0x49ff1173, 0xeadc, 0x446d, 0x92, 0x85, 0x15, 0x64, 0x53, 0xa6, 0x43, 0x1c);
DEFINE_GUID(IID_IActionProgressDialog,     0x49ff1172, 0xeadc, 0x446d, 0x92, 0x85, 0x15, 0x64, 0x53, 0xa6, 0x43, 0x1c);
DEFINE_GUID(IID_IActiveDesktop,            0xf490eb00, 0x1240, 0x11d1, 0x98, 0x88, 0x00, 0x60, 0x97, 0xde, 0xac, 0xf9);
DEFINE_GUID(IID_IActiveDesktopP,           0x52502ee0, 0xec80, 0x11d0, 0x89, 0xab, 0x00, 0xc0, 0x4f, 0xc2, 0x97, 0x2d);
DEFINE_GUID(IID_IAddressBarParser,         0xc9d81948, 0x443a, 0x40c7, 0x94, 0x5c, 0x5e, 0x17, 0x1b, 0x8c, 0x66, 0xb4);
DEFINE_GUID(IID_IAugmentedShellFolder,     0x91ea3f8c, 0xc99b, 0x11d0, 0x98, 0x15, 0x00, 0xc0, 0x4f, 0xd9, 0x19, 0x72);
DEFINE_GUID(IID_IAugmentedShellFolder2,    0x8db3b3f4, 0x6cfe, 0x11d1, 0x8a, 0xe9, 0x00, 0xc0, 0x4f, 0xd9, 0x18, 0xd0);
DEFINE_GUID(IID_IBandHost,                 0xb9075c7c, 0xd48e, 0x403f, 0xab, 0x99, 0xd6, 0xc7, 0x7a, 0x10, 0x84, 0xac);
DEFINE_GUID(IID_IBrowserFrameOptions,      0x10df43c8, 0x1dbe, 0x11d3, 0x8b, 0x34, 0x00, 0x60, 0x97, 0xdf, 0x5b, 0xd4);
DEFINE_GUID(IID_IComputerInfoChangeNotify, 0x0df60d92, 0x6818, 0x46d6, 0xb3, 0x58, 0xd6, 0x61, 0x70, 0xdd, 0xe4, 0x66);
DEFINE_OLEGUID(IID_IContext,               0x000001c0, 0x0000, 0x0000);
DEFINE_OLEGUID(IID_ICopyHookA,             0x000214ef, 0x0000, 0x0000);
DEFINE_OLEGUID(IID_ICopyHookW,             0x000214fc, 0x0000, 0x0000);
DEFINE_GUID(IID_ICDBurn,                   0x3d73a659, 0xe5d0, 0x4d42, 0xaf, 0xc0, 0x51, 0x21, 0xba, 0x42, 0x5c, 0x8d);
DEFINE_GUID(IID_ICDBurnExt,                0x2271dcca, 0x74fc, 0x4414, 0x8f, 0xb7, 0xc5, 0x6b, 0x05, 0xac, 0xe2, 0xd7);
DEFINE_GUID(IID_IDVGetEnum,                0x70f55181, 0x5fea, 0x4900, 0xb6, 0xb8, 0x73, 0x43, 0xcb, 0x0a, 0x34, 0x8c);
DEFINE_GUID(IID_IDefViewID,                0x985f64f0, 0xd410, 0x4e02, 0xbe, 0x22, 0xda, 0x07, 0xf2, 0xb5, 0xc5, 0xe1);
DEFINE_GUID(IID_IDefViewFrame,             0x710EB7A0, 0x45ED, 0x11D0, 0x92, 0x4A, 0x00, 0x20, 0xAF, 0xC7, 0xAC, 0x4D);
DEFINE_GUID(IID_IDefViewFrame3,            0x985f64f0, 0xd410, 0x4e02, 0xbe, 0x22, 0xda, 0x07, 0xf2, 0xb5, 0xc5, 0xe1);
DEFINE_GUID(IID_IDefViewFrameGroup,        0xb4bf6da0, 0xbfba, 0x4cbf, 0xb0, 0x3a, 0x6c, 0x55, 0xfa, 0x20, 0xaf, 0x65);
DEFINE_GUID(IID_IDefViewSafety,            0x9a93b3fb, 0x4e75, 0x4c74, 0x87, 0x1a, 0x2c, 0xda, 0x66, 0x7f, 0x39, 0xa5);
DEFINE_GUID(IID_IDefViewScript,            0x4706b500, 0x09ef, 0x11d0, 0x99, 0xcb, 0x00, 0xc0, 0x4f, 0xd6, 0x44, 0x97);
DEFINE_GUID(IID_IDeskBandEx,               0x5dd6b79a, 0x3ab7, 0x49c0, 0xab, 0x82, 0x6b, 0x2d, 0xa7, 0xd7, 0x8d, 0x75);
DEFINE_GUID(IID_IDeskMovr,                 0x72267f69, 0xa6f9, 0x11d0, 0xbc, 0x94, 0x00, 0xc0, 0x4f, 0xb6, 0x78, 0x63);
DEFINE_GUID(IID_IDocViewSite,              0x87D605E0, 0xC511, 0x11CF, 0x89, 0xA9, 0x00, 0xA0, 0xC9, 0x05, 0x41, 0x29);
DEFINE_GUID(IID_IExtractImage,             0xbb2e617c, 0x0920, 0x11d1, 0x9a, 0x0b, 0x00, 0xc0, 0x4f, 0xc2, 0xd6, 0xc1);
DEFINE_GUID(IID_IExtractImage2,            0x953bb1ee, 0x93b4, 0x11d1, 0x98, 0xa3, 0x00, 0xc0, 0x4f, 0xb6, 0x87, 0xda);
DEFINE_GUID(IID_IFileSearchBand,           0x2d91eea1, 0x9932, 0x11d2, 0xbe, 0x86, 0x00, 0xa0, 0xc9, 0xa8, 0x3d, 0xa1);
DEFINE_GUID(IID_IFolderBandPriv,           0x47c01f95, 0xe185, 0x412c, 0xb5, 0xc5, 0x4f, 0x27, 0xdf, 0x96, 0x5a, 0xea);
DEFINE_GUID(IID_IFolderFilter,             0x9cc22886, 0xdc8e, 0x11d2, 0xb1, 0xd0, 0x00, 0xc0, 0x4f, 0x8e, 0xeb, 0x3e);
DEFINE_GUID(IID_IFolderFilterSite,         0xc0a651f5, 0xb48b, 0x11d2, 0xb5, 0xed, 0x00, 0x60, 0x97, 0xc6, 0x86, 0xf6);
DEFINE_GUID(IID_IFolderViewHost,           0x1ea58f02, 0xd55a, 0x411d, 0xb0, 0x9e, 0x9e, 0x65, 0xac, 0x21, 0x60, 0x5b);
DEFINE_GUID(IID_IFolderViewSettings,       0xae8c987d, 0x8797, 0x4ed3, 0xbe, 0x72, 0x2a, 0x47, 0xdd, 0x93, 0x8d, 0xb0);
DEFINE_GUID(IID_IGetNameSpaceExtensionPointer,0x287d4a71, 0x439f, 0x43a4, 0x8d, 0x5b, 0x0e, 0x0a, 0xe7, 0x1e, 0x84, 0xa9);
DEFINE_GUID(IID_IHWEventHandler,           0xc1fb73d0, 0xec3a, 0x4ba2, 0xb5, 0x12, 0x8c, 0xdb, 0x91, 0x87, 0xb6, 0xd1);
DEFINE_GUID(IID_IHWEventHandler2,          0xcfcc809f, 0x295d, 0x42e8, 0x9f, 0xfc, 0x42, 0x4b, 0x33, 0xc4, 0x87, 0xe6);
DEFINE_GUID(IID_IMultiMonitorDockingSite,  0x03879de0, 0xa205, 0x11d0, 0x99, 0xcb, 0x00, 0xc0, 0x4f, 0xd6, 0x55, 0xe1);
DEFINE_GUID(IID_INamespaceWalk,            0x57ced8a7, 0x3f4a, 0x432c, 0x93, 0x50, 0x30, 0xf2, 0x44, 0x83, 0xf7, 0x4f);
DEFINE_GUID(IID_INamespaceWalkCB,          0xd92995f8, 0xcf5e, 0x4a76, 0xbf, 0x59, 0xea, 0xd3, 0x9e, 0xa2, 0xb9, 0x7e);
DEFINE_GUID(IID_INamespaceWalkCB2,         0x7ac7492b, 0xc38e, 0x438a, 0x87, 0xdb, 0x68, 0x73, 0x78, 0x44, 0xff, 0x70);
DEFINE_GUID(IID_IPersistFreeThreadedObject,0xc7264bf0, 0xedb6, 0x11d1, 0x85, 0x46, 0x00, 0x60, 0x08, 0x05, 0x93, 0x68);
DEFINE_GUID(IID_IPreviewHandler,           0x8895b1c6, 0xb41f, 0x4c1c, 0xa5, 0x62, 0x0d, 0x56, 0x42, 0x50, 0x83, 0x6f);
DEFINE_GUID(IID_IPreviewHandlerFrame,      0xfec87aaf, 0x35f9, 0x447a, 0xad, 0xb7, 0x20, 0x23, 0x44, 0x91, 0x40, 0x1a);
DEFINE_GUID(IID_IPreviewHandlerVisuals,    0x196bf9a5, 0xb346, 0x4ef0, 0xaa, 0x1e, 0x5d, 0xcd, 0xb7, 0x67, 0x68, 0xb1);
DEFINE_GUID(IID_IShellApp,                 0xa3e14960, 0x935f, 0x11d1, 0xb8, 0xb8, 0x00, 0x60, 0x08, 0x05, 0x93, 0x82);
DEFINE_GUID(IID_IShellDispatch3,           0x177160ca, 0xbb5a, 0x411c, 0x84, 0x1d, 0xbd, 0x38, 0xfa, 0xcd, 0xea, 0xa0);
DEFINE_GUID(IID_IShellDispatch4,           0xefd84b2d, 0x4bcf, 0x4298, 0xbe, 0x25, 0xeb, 0x54, 0x2a, 0x59, 0xfb, 0xda);
DEFINE_GUID(IID_IShellDispatch5,           0x866738b9, 0x6cf2, 0x4de8, 0x87, 0x67, 0xf7, 0x94, 0xeb, 0xe7, 0x4f, 0x4e);
DEFINE_GUID(IID_IShellFolderSearchable,    0x4e1ae66c, 0x204b, 0x11d2, 0x8d, 0xb3, 0x00, 0x00, 0xf8, 0x7a, 0x55, 0x6c);
DEFINE_GUID(IID_IShellFolderSearchableCallback,0xf98d8294, 0x2bbc, 0x11d2, 0x8d, 0xbd, 0x00, 0x00, 0xf8, 0x7a, 0x55, 0x6c);
DEFINE_GUID(IID_IShellFolderViewType,      0x49422c1e, 0x1c03, 0x11d2, 0x8d, 0xab, 0x00, 0x00, 0xf8, 0x7a, 0x55, 0x6c);
DEFINE_GUID(IID_IShellIconOverlay,         0x7d688a70, 0xc613, 0x11d0, 0x99, 0x9b, 0x00, 0xc0, 0x4f, 0xd6, 0x55, 0xe1);
DEFINE_GUID(IID_IContextMenuSite,          0x0811aebe, 0x0b87, 0x4c54, 0x9e, 0x72, 0x54, 0x8c, 0xf6, 0x49, 0x01, 0x6b);
DEFINE_GUID(IID_IShellDesktopTray,         0x213e2df9, 0x9a14, 0x4328, 0x99, 0xb1, 0x69, 0x61, 0xf9, 0x14, 0x3c, 0xe9);
DEFINE_GUID(IID_IShellFolderBand,          0x7fe80cc8, 0xc247, 0x11d0, 0xb9, 0x3a, 0x00, 0xa0, 0xc9, 0x03, 0x12, 0xe1);
DEFINE_GUID(IID_IShellFolderViewDual,      0xe7a1af80, 0x4d96, 0x11cf, 0x96, 0x0c, 0x00, 0x80, 0xc7, 0xf4, 0xee, 0x85);
DEFINE_GUID(IID_IShellFolderViewDual2,     0x31c147b6, 0x0ade, 0x4a3c, 0xb5, 0x14, 0xdd, 0xf9, 0x32, 0xef, 0x6d, 0x17);
DEFINE_GUID(IID_IShellImageData,           0xbfdeec12, 0x8040, 0x4403, 0xa5, 0xea, 0x9e, 0x07, 0xda, 0xfc, 0xf5, 0x30);
DEFINE_GUID(IID_IShellImageDataAbort,      0x53fb8e58, 0x50c0, 0x4003, 0xb4, 0xaa, 0x0c, 0x8d, 0xf2, 0x8e, 0x7f, 0x3a);
DEFINE_GUID(IID_IShellImageDataFactory,    0x9be8ed5c, 0xedab, 0x4d75, 0x90, 0xf3, 0xbd, 0x5b, 0xdb, 0xb2, 0x1c, 0x82);
DEFINE_GUID(IID_IShellLinkDual,            0x88a05c00, 0xf000, 0x11ce, 0x83, 0x50, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00);
DEFINE_GUID(IID_IShellLinkDual2,           0x317ee249, 0xf12e, 0x11d2, 0xb1, 0xe4, 0x00, 0xc0, 0x4f, 0x8e, 0xeb, 0x3e);
DEFINE_GUID(IID_IShellView3,               0xec39fa88, 0xf8af, 0x41c5, 0x84, 0x21, 0x38, 0xbe, 0xd2, 0x8f, 0x46, 0x73);
DEFINE_GUID(IID_ITranslateShellChangeNotify,0xb4df2675, 0xba23, 0x11d2, 0xb5, 0xee, 0x00, 0x60, 0x97, 0xc6, 0x86, 0xf6);
DEFINE_GUID(IID_IViewObjectEx,             0x3af24292, 0x0c96, 0x11ce, 0xa0, 0xcf, 0x00, 0xaa, 0x00, 0x60, 0x0a, 0xb8);

DEFINE_GUID(IID_DFConstraint,              0x403df050, 0x23bd, 0x11d2, 0x93, 0x9f, 0x00, 0xa0, 0xc9, 0x1e, 0xed, 0xba);

DEFINE_GUID(IID_CDefView,                  0x4434ff80, 0xef4c, 0x11ce, 0xae, 0x65, 0x08, 0x00, 0x2b, 0xe2, 0x12, 0x62);
DEFINE_GUID(IID_FolderItems2,              0xc94f0ad0, 0xf363, 0x11d2, 0xa3, 0x27, 0x00, 0xc0, 0x4f, 0x8e, 0xec, 0x7f);
DEFINE_GUID(IID_FolderItems3,              0xeaa7c309, 0xbbec, 0x49d5, 0x82, 0x1d, 0x64, 0xd9, 0x66, 0xcb, 0x66, 0x7f);
