/*
 * Copyright (C) the Wine project
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __WINE_DDRAW_H
#define __WINE_DDRAW_H

#include <windows.h> /* LARGE_INTEGER ... */

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

#ifndef __DDRAW_GUID_DEFINED__
	DEFINE_GUID( CLSID_DirectDraw,		0xD7B70EE0,0x4340,0x11CF,0xB0,0x63,0x00,0x20,0xAF,0xC2,0xCD,0x35 );

#endif

#define DDENUMRET_CANCEL	0
#define DDENUMRET_OK		1

#define DD_OK			0

#define _FACDD		0x876
#define MAKE_DDHRESULT( code )  MAKE_HRESULT( 1, _FACDD, code )

#define DDERR_ALREADYINITIALIZED		MAKE_DDHRESULT( 5 )
#define DDERR_CANNOTATTACHSURFACE		MAKE_DDHRESULT( 10 )
#define DDERR_CANNOTDETACHSURFACE		MAKE_DDHRESULT( 20 )
#define DDERR_CURRENTLYNOTAVAIL			MAKE_DDHRESULT( 40 )
#define DDERR_EXCEPTION				MAKE_DDHRESULT( 55 )
#define DDERR_GENERIC				E_FAIL
#define DDERR_HEIGHTALIGN			MAKE_DDHRESULT( 90 )
#define DDERR_INCOMPATIBLEPRIMARY		MAKE_DDHRESULT( 95 )
#define DDERR_INVALIDCAPS			MAKE_DDHRESULT( 100 )
#define DDERR_INVALIDCLIPLIST			MAKE_DDHRESULT( 110 )
#define DDERR_INVALIDMODE			MAKE_DDHRESULT( 120 )
#define DDERR_INVALIDOBJECT			MAKE_DDHRESULT( 130 )
#define DDERR_INVALIDPARAMS			E_INVALIDARG
#define DDERR_INVALIDPIXELFORMAT		MAKE_DDHRESULT( 145 )
#define DDERR_INVALIDRECT			MAKE_DDHRESULT( 150 )
#define DDERR_LOCKEDSURFACES			MAKE_DDHRESULT( 160 )
#define DDERR_NO3D				MAKE_DDHRESULT( 170 )
#define DDERR_NOALPHAHW				MAKE_DDHRESULT( 180 )
#define DDERR_NOSTEREOHARDWARE          	MAKE_DDHRESULT( 181 )
#define DDERR_NOSURFACELEFT                     MAKE_DDHRESULT( 182 )
#define DDERR_NOCLIPLIST			MAKE_DDHRESULT( 205 )
#define DDERR_NOCOLORCONVHW			MAKE_DDHRESULT( 210 )
#define DDERR_NOCOOPERATIVELEVELSET		MAKE_DDHRESULT( 212 )
#define DDERR_NOCOLORKEY			MAKE_DDHRESULT( 215 )
#define DDERR_NOCOLORKEYHW			MAKE_DDHRESULT( 220 )
#define DDERR_NODIRECTDRAWSUPPORT		MAKE_DDHRESULT( 222 )
#define DDERR_NOEXCLUSIVEMODE			MAKE_DDHRESULT( 225 )
#define DDERR_NOFLIPHW				MAKE_DDHRESULT( 230 )
#define DDERR_NOGDI				MAKE_DDHRESULT( 240 )
#define DDERR_NOMIRRORHW			MAKE_DDHRESULT( 250 )
#define DDERR_NOTFOUND				MAKE_DDHRESULT( 255 )
#define DDERR_NOOVERLAYHW			MAKE_DDHRESULT( 260 )
#define DDERR_OVERLAPPINGRECTS                  MAKE_DDHRESULT( 270 )
#define DDERR_NORASTEROPHW			MAKE_DDHRESULT( 280 )
#define DDERR_NOROTATIONHW			MAKE_DDHRESULT( 290 )
#define DDERR_NOSTRETCHHW			MAKE_DDHRESULT( 310 )
#define DDERR_NOT4BITCOLOR			MAKE_DDHRESULT( 316 )
#define DDERR_NOT4BITCOLORINDEX			MAKE_DDHRESULT( 317 )
#define DDERR_NOT8BITCOLOR			MAKE_DDHRESULT( 320 )
#define DDERR_NOTEXTUREHW			MAKE_DDHRESULT( 330 )
#define DDERR_NOVSYNCHW				MAKE_DDHRESULT( 335 )
#define DDERR_NOZBUFFERHW			MAKE_DDHRESULT( 340 )
#define DDERR_NOZOVERLAYHW			MAKE_DDHRESULT( 350 )
#define DDERR_OUTOFCAPS				MAKE_DDHRESULT( 360 )
#define DDERR_OUTOFMEMORY			E_OUTOFMEMORY
#define DDERR_OUTOFVIDEOMEMORY			MAKE_DDHRESULT( 380 )
#define DDERR_OVERLAYCANTCLIP			MAKE_DDHRESULT( 382 )
#define DDERR_OVERLAYCOLORKEYONLYONEACTIVE	MAKE_DDHRESULT( 384 )
#define DDERR_PALETTEBUSY			MAKE_DDHRESULT( 387 )
#define DDERR_COLORKEYNOTSET			MAKE_DDHRESULT( 400 )
#define DDERR_SURFACEALREADYATTACHED		MAKE_DDHRESULT( 410 )
#define DDERR_SURFACEALREADYDEPENDENT		MAKE_DDHRESULT( 420 )
#define DDERR_SURFACEBUSY			MAKE_DDHRESULT( 430 )
#define DDERR_CANTLOCKSURFACE			MAKE_DDHRESULT( 435 )
#define DDERR_SURFACEISOBSCURED			MAKE_DDHRESULT( 440 )
#define DDERR_SURFACELOST			MAKE_DDHRESULT( 450 )
#define DDERR_SURFACENOTATTACHED		MAKE_DDHRESULT( 460 )
#define DDERR_TOOBIGHEIGHT			MAKE_DDHRESULT( 470 )
#define DDERR_TOOBIGSIZE			MAKE_DDHRESULT( 480 )
#define DDERR_TOOBIGWIDTH			MAKE_DDHRESULT( 490 )
#define DDERR_UNSUPPORTED			E_NOTIMPL
#define DDERR_UNSUPPORTEDFORMAT			MAKE_DDHRESULT( 510 )
#define DDERR_UNSUPPORTEDMASK			MAKE_DDHRESULT( 520 )
#define DDERR_INVALIDSTREAM                     MAKE_DDHRESULT( 521 )
#define DDERR_VERTICALBLANKINPROGRESS		MAKE_DDHRESULT( 537 )
#define DDERR_WASSTILLDRAWING			MAKE_DDHRESULT( 540 )
#define DDERR_DDSCAPSCOMPLEXREQUIRED            MAKE_DDHRESULT( 542 )
#define DDERR_XALIGN				MAKE_DDHRESULT( 560 )
#define DDERR_INVALIDDIRECTDRAWGUID		MAKE_DDHRESULT( 561 )
#define DDERR_DIRECTDRAWALREADYCREATED		MAKE_DDHRESULT( 562 )
#define DDERR_NODIRECTDRAWHW			MAKE_DDHRESULT( 563 )
#define DDERR_PRIMARYSURFACEALREADYEXISTS	MAKE_DDHRESULT( 564 )
#define DDERR_NOEMULATION			MAKE_DDHRESULT( 565 )
#define DDERR_REGIONTOOSMALL			MAKE_DDHRESULT( 566 )
#define DDERR_CLIPPERISUSINGHWND		MAKE_DDHRESULT( 567 )
#define DDERR_NOCLIPPERATTACHED			MAKE_DDHRESULT( 568 )
#define DDERR_NOHWND				MAKE_DDHRESULT( 569 )
#define DDERR_HWNDSUBCLASSED			MAKE_DDHRESULT( 570 )
#define DDERR_HWNDALREADYSET			MAKE_DDHRESULT( 571 )
#define DDERR_NOPALETTEATTACHED			MAKE_DDHRESULT( 572 )
#define DDERR_NOPALETTEHW			MAKE_DDHRESULT( 573 )
#define DDERR_BLTFASTCANTCLIP			MAKE_DDHRESULT( 574 )
#define DDERR_NOBLTHW				MAKE_DDHRESULT( 575 )
#define DDERR_NODDROPSHW			MAKE_DDHRESULT( 576 )
#define DDERR_OVERLAYNOTVISIBLE			MAKE_DDHRESULT( 577 )
#define DDERR_NOOVERLAYDEST			MAKE_DDHRESULT( 578 )
#define DDERR_INVALIDPOSITION			MAKE_DDHRESULT( 579 )
#define DDERR_NOTAOVERLAYSURFACE		MAKE_DDHRESULT( 580 )
#define DDERR_EXCLUSIVEMODEALREADYSET		MAKE_DDHRESULT( 581 )
#define DDERR_NOTFLIPPABLE			MAKE_DDHRESULT( 582 )
#define DDERR_CANTDUPLICATE			MAKE_DDHRESULT( 583 )
#define DDERR_NOTLOCKED				MAKE_DDHRESULT( 584 )
#define DDERR_CANTCREATEDC			MAKE_DDHRESULT( 585 )
#define DDERR_NODC				MAKE_DDHRESULT( 586 )
#define DDERR_WRONGMODE				MAKE_DDHRESULT( 587 )
#define DDERR_IMPLICITLYCREATED			MAKE_DDHRESULT( 588 )
#define DDERR_NOTPALETTIZED			MAKE_DDHRESULT( 589 )
#define DDERR_UNSUPPORTEDMODE			MAKE_DDHRESULT( 590 )
#define DDERR_NOMIPMAPHW			MAKE_DDHRESULT( 591 )
#define DDERR_INVALIDSURFACETYPE		MAKE_DDHRESULT( 592 )
#define DDERR_NOOPTIMIZEHW			MAKE_DDHRESULT( 600 )
#define DDERR_NOTLOADED				MAKE_DDHRESULT( 601 )
#define DDERR_NOFOCUSWINDOW			MAKE_DDHRESULT( 602 )
#define DDERR_NOTONMIPMAPSUBLEVEL               MAKE_DDHRESULT( 603 )
#define DDERR_DCALREADYCREATED			MAKE_DDHRESULT( 620 )
#define DDERR_NONONLOCALVIDMEM			MAKE_DDHRESULT( 630 )
#define DDERR_CANTPAGELOCK			MAKE_DDHRESULT( 640 )
#define DDERR_CANTPAGEUNLOCK			MAKE_DDHRESULT( 660 )
#define DDERR_NOTPAGELOCKED			MAKE_DDHRESULT( 680 )
#define DDERR_MOREDATA				MAKE_DDHRESULT( 690 )
#define DDERR_EXPIRED                           MAKE_DDHRESULT( 691 )
#define DDERR_TESTFINISHED                      MAKE_DDHRESULT( 692 )
#define DDERR_NEWMODE                           MAKE_DDHRESULT( 693 )
#define DDERR_D3DNOTINITIALIZED                 MAKE_DDHRESULT( 694 )
#define DDERR_VIDEONOTACTIVE			MAKE_DDHRESULT( 695 )
#define DDERR_NOMONITORINFORMATION              MAKE_DDHRESULT( 696 )
#define DDERR_NODRIVERSUPPORT                   MAKE_DDHRESULT( 697 )
#define DDERR_DEVICEDOESNTOWNSURFACE		MAKE_DDHRESULT( 699 )
#define DDERR_NOTINITIALIZED			CO_E_NOTINITIALIZED

/* dwFlags for Blt* */
#define DDBLT_ALPHADEST				0x00000001
#define DDBLT_ALPHADESTCONSTOVERRIDE		0x00000002
#define DDBLT_ALPHADESTNEG			0x00000004
#define DDBLT_ALPHADESTSURFACEOVERRIDE		0x00000008
#define DDBLT_ALPHAEDGEBLEND			0x00000010
#define DDBLT_ALPHASRC				0x00000020
#define DDBLT_ALPHASRCCONSTOVERRIDE		0x00000040
#define DDBLT_ALPHASRCNEG			0x00000080
#define DDBLT_ALPHASRCSURFACEOVERRIDE		0x00000100
#define DDBLT_ASYNC				0x00000200
#define DDBLT_COLORFILL				0x00000400
#define DDBLT_DDFX				0x00000800
#define DDBLT_DDROPS				0x00001000
#define DDBLT_KEYDEST				0x00002000
#define DDBLT_KEYDESTOVERRIDE			0x00004000
#define DDBLT_KEYSRC				0x00008000
#define DDBLT_KEYSRCOVERRIDE			0x00010000
#define DDBLT_ROP				0x00020000
#define DDBLT_ROTATIONANGLE			0x00040000
#define DDBLT_ZBUFFER				0x00080000
#define DDBLT_ZBUFFERDESTCONSTOVERRIDE		0x00100000
#define DDBLT_ZBUFFERDESTOVERRIDE		0x00200000
#define DDBLT_ZBUFFERSRCCONSTOVERRIDE		0x00400000
#define DDBLT_ZBUFFERSRCOVERRIDE		0x00800000
#define DDBLT_WAIT				0x01000000
#define DDBLT_DEPTHFILL				0x02000000
#define DDBLT_DONOTWAIT                         0x08000000

/* dwTrans for BltFast */
#define DDBLTFAST_NOCOLORKEY			0x00000000
#define DDBLTFAST_SRCCOLORKEY			0x00000001
#define DDBLTFAST_DESTCOLORKEY			0x00000002
#define DDBLTFAST_WAIT				0x00000010
#define DDBLTFAST_DONOTWAIT                     0x00000020

/* dwFlags for Flip */
#define DDFLIP_WAIT		0x00000001
#define DDFLIP_EVEN		0x00000002 /* only valid for overlay */
#define DDFLIP_ODD		0x00000004 /* only valid for overlay */
#define DDFLIP_NOVSYNC		0x00000008
#define DDFLIP_STEREO		0x00000010
#define DDFLIP_DONOTWAIT	0x00000020

/* dwFlags for GetBltStatus */
#define DDGBS_CANBLT				0x00000001
#define DDGBS_ISBLTDONE				0x00000002

/* dwFlags for IDirectDrawSurface7::GetFlipStatus */
#define DDGFS_CANFLIP		1L
#define DDGFS_ISFLIPDONE	2L

/* dwFlags for IDirectDrawSurface7::SetPrivateData */
#define DDSPD_IUNKNOWNPTR	1L
#define DDSPD_VOLATILE		2L

/* DDSCAPS.dwCaps */
/* reserved1, was 3d capable */
#define DDSCAPS_RESERVED1		0x00000001
/* surface contains alpha information */
#define DDSCAPS_ALPHA			0x00000002
/* this surface is a backbuffer */
#define DDSCAPS_BACKBUFFER		0x00000004
/* complex surface structure */
#define DDSCAPS_COMPLEX			0x00000008
/* part of surface flipping structure */
#define DDSCAPS_FLIP			0x00000010
/* this surface is the frontbuffer surface */
#define DDSCAPS_FRONTBUFFER		0x00000020
/* this is a plain offscreen surface */
#define DDSCAPS_OFFSCREENPLAIN		0x00000040
/* overlay */
#define DDSCAPS_OVERLAY			0x00000080
/* palette objects can be created and attached to us */
#define DDSCAPS_PALETTE			0x00000100
/* primary surface (the one the user looks at currently)(right eye)*/
#define DDSCAPS_PRIMARYSURFACE		0x00000200
/* primary surface for left eye */
#define DDSCAPS_PRIMARYSURFACELEFT	0x00000400
/* surface exists in systemmemory */
#define DDSCAPS_SYSTEMMEMORY		0x00000800
/* surface can be used as a texture */
#define DDSCAPS_TEXTURE		        0x00001000
/* surface may be destination for 3d rendering */
#define DDSCAPS_3DDEVICE		0x00002000
/* surface exists in videomemory */
#define DDSCAPS_VIDEOMEMORY		0x00004000
/* surface changes immediately visible */
#define DDSCAPS_VISIBLE			0x00008000
/* write only surface */
#define DDSCAPS_WRITEONLY		0x00010000
/* zbuffer surface */
#define DDSCAPS_ZBUFFER			0x00020000
/* has its own DC */
#define DDSCAPS_OWNDC			0x00040000
/* surface should be able to receive live video */
#define DDSCAPS_LIVEVIDEO		0x00080000
/* should be able to have a hw codec decompress stuff into it */
#define DDSCAPS_HWCODEC			0x00100000
/* mode X (320x200 or 320x240) surface */
#define DDSCAPS_MODEX			0x00200000
/* one mipmap surface (1 level) */
#define DDSCAPS_MIPMAP			0x00400000
#define DDSCAPS_RESERVED2		0x00800000
/* memory allocation delayed until Load() */
#define DDSCAPS_ALLOCONLOAD		0x04000000
/* Indicates that the surface will receive data from a video port */
#define DDSCAPS_VIDEOPORT		0x08000000
/* surface is in local videomemory */
#define DDSCAPS_LOCALVIDMEM		0x10000000
/* surface is in nonlocal videomemory */
#define DDSCAPS_NONLOCALVIDMEM		0x20000000
/* surface is a standard VGA mode surface (NOT ModeX) */
#define DDSCAPS_STANDARDVGAMODE		0x40000000
/* optimized? surface */
#define DDSCAPS_OPTIMIZED		0x80000000

typedef struct _DDSCAPS {
	DWORD	dwCaps;	/* capabilities of surface wanted */
} DDSCAPS,*LPDDSCAPS;

/* DDSCAPS2.dwCaps2 */
/* indicates the surface will receive data from a video port using
   deinterlacing hardware. */
#define DDSCAPS2_HARDWAREDEINTERLACE	0x00000002
/* indicates the surface will be locked very frequently. */
#define DDSCAPS2_HINTDYNAMIC		0x00000004
/* indicates surface can be re-ordered or retiled on load() */
#define DDSCAPS2_HINTSTATIC             0x00000008
/* indicates surface to be managed by directdraw/direct3D */
#define DDSCAPS2_TEXTUREMANAGE          0x00000010
/* reserved bits */
#define DDSCAPS2_RESERVED1              0x00000020
#define DDSCAPS2_RESERVED2              0x00000040
/* indicates surface will never be locked again */
#define DDSCAPS2_OPAQUE                 0x00000080
/* set at CreateSurface() time to indicate antialising will be used */
#define DDSCAPS2_HINTANTIALIASING       0x00000100
/* set at CreateSurface() time to indicate cubic environment map */
#define DDSCAPS2_CUBEMAP                0x00000200
/* face flags for cube maps */
#define DDSCAPS2_CUBEMAP_POSITIVEX      0x00000400
#define DDSCAPS2_CUBEMAP_NEGATIVEX      0x00000800
#define DDSCAPS2_CUBEMAP_POSITIVEY      0x00001000
#define DDSCAPS2_CUBEMAP_NEGATIVEY      0x00002000
#define DDSCAPS2_CUBEMAP_POSITIVEZ      0x00004000
#define DDSCAPS2_CUBEMAP_NEGATIVEZ      0x00008000
/* specifies all faces of a cube for CreateSurface() */
#define DDSCAPS2_CUBEMAP_ALLFACES ( DDSCAPS2_CUBEMAP_POSITIVEX |\
                                    DDSCAPS2_CUBEMAP_NEGATIVEX |\
                                    DDSCAPS2_CUBEMAP_POSITIVEY |\
                                    DDSCAPS2_CUBEMAP_NEGATIVEY |\
                                    DDSCAPS2_CUBEMAP_POSITIVEZ |\
                                    DDSCAPS2_CUBEMAP_NEGATIVEZ )
/* set for mipmap sublevels on DirectX7 and later.  ignored by CreateSurface() */
#define DDSCAPS2_MIPMAPSUBLEVEL         0x00010000
/* indicates texture surface to be managed by Direct3D *only* */
#define DDSCAPS2_D3DTEXTUREMANAGE       0x00020000
/* indicates managed surface that can safely be lost */
#define DDSCAPS2_DONOTPERSIST           0x00040000
/* indicates surface is part of a stereo flipping chain */
#define DDSCAPS2_STEREOSURFACELEFT      0x00080000

typedef struct _DDSCAPS2 {
	DWORD	dwCaps;	/* capabilities of surface wanted */
	DWORD   dwCaps2; /* additional capabilities */
	DWORD   dwCaps3; /* reserved capabilities */
	DWORD   dwCaps4; /* more reserved capabilities */
} DDSCAPS2,*LPDDSCAPS2;

typedef struct _DDSCAPSEX {
	DWORD   dwCaps2; /* additional capabilities */
	DWORD   dwCaps3; /* reserved capabilities */
	DWORD   dwCaps4; /* more reserved capabilities */
} DDSCAPSEX,*LPDDSCAPSEX;

#define	DD_ROP_SPACE	(256/32)	/* space required to store ROP array */

typedef struct DDCAPS_DX7		/* DirectX 7 version of caps struct */
{
    DWORD	dwSize;                 /* size of the DDDRIVERCAPS structure */
    DWORD	dwCaps;                 /* driver specific capabilities */
    DWORD	dwCaps2;                /* more driver specific capabilites */
    DWORD	dwCKeyCaps;             /* color key capabilities of the surface */
    DWORD	dwFXCaps;               /* driver specific stretching and effects capabilites */
    DWORD	dwFXAlphaCaps;          /* alpha driver specific capabilities */
    DWORD	dwPalCaps;              /* palette capabilities */
    DWORD	dwSVCaps;               /* stereo vision capabilities */
    DWORD	dwAlphaBltConstBitDepths;       /* DDBD_2,4,8 */
    DWORD	dwAlphaBltPixelBitDepths;       /* DDBD_1,2,4,8 */
    DWORD	dwAlphaBltSurfaceBitDepths;     /* DDBD_1,2,4,8 */
    DWORD	dwAlphaOverlayConstBitDepths;   /* DDBD_2,4,8 */
    DWORD	dwAlphaOverlayPixelBitDepths;   /* DDBD_1,2,4,8 */
    DWORD	dwAlphaOverlaySurfaceBitDepths; /* DDBD_1,2,4,8 */
    DWORD	dwZBufferBitDepths;             /* DDBD_8,16,24,32 */
    DWORD	dwVidMemTotal;          /* total amount of video memory */
    DWORD	dwVidMemFree;           /* amount of free video memory */
    DWORD	dwMaxVisibleOverlays;   /* maximum number of visible overlays */
    DWORD	dwCurrVisibleOverlays;  /* current number of visible overlays */
    DWORD	dwNumFourCCCodes;       /* number of four cc codes */
    DWORD	dwAlignBoundarySrc;     /* source rectangle alignment */
    DWORD	dwAlignSizeSrc;         /* source rectangle byte size */
    DWORD	dwAlignBoundaryDest;    /* dest rectangle alignment */
    DWORD	dwAlignSizeDest;        /* dest rectangle byte size */
    DWORD	dwAlignStrideAlign;     /* stride alignment */
    DWORD	dwRops[DD_ROP_SPACE];   /* ROPS supported */
    DDSCAPS	ddsOldCaps;             /* old DDSCAPS - superseded for DirectX6+ */
    DWORD	dwMinOverlayStretch;    /* minimum overlay stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3 */
    DWORD	dwMaxOverlayStretch;    /* maximum overlay stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3 */
    DWORD	dwMinLiveVideoStretch;  /* minimum live video stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3 */
    DWORD	dwMaxLiveVideoStretch;  /* maximum live video stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3 */
    DWORD	dwMinHwCodecStretch;    /* minimum hardware codec stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3 */
    DWORD	dwMaxHwCodecStretch;    /* maximum hardware codec stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3 */
    DWORD	dwReserved1;
    DWORD	dwReserved2;
    DWORD	dwReserved3;
    DWORD	dwSVBCaps;              /* driver specific capabilities for System->Vmem blts */
    DWORD	dwSVBCKeyCaps;          /* driver color key capabilities for System->Vmem blts */
    DWORD	dwSVBFXCaps;            /* driver FX capabilities for System->Vmem blts */
    DWORD	dwSVBRops[DD_ROP_SPACE];/* ROPS supported for System->Vmem blts */
    DWORD	dwVSBCaps;              /* driver specific capabilities for Vmem->System blts */
    DWORD	dwVSBCKeyCaps;          /* driver color key capabilities for Vmem->System blts */
    DWORD	dwVSBFXCaps;            /* driver FX capabilities for Vmem->System blts */
    DWORD	dwVSBRops[DD_ROP_SPACE];/* ROPS supported for Vmem->System blts */
    DWORD	dwSSBCaps;              /* driver specific capabilities for System->System blts */
    DWORD	dwSSBCKeyCaps;          /* driver color key capabilities for System->System blts */
    DWORD	dwSSBFXCaps;            /* driver FX capabilities for System->System blts */
    DWORD	dwSSBRops[DD_ROP_SPACE];/* ROPS supported for System->System blts */
    DWORD       dwMaxVideoPorts;        /* maximum number of usable video ports */
    DWORD   	dwCurrVideoPorts;       /* current number of video ports used */
    DWORD   	dwSVBCaps2;             /* more driver specific capabilities for System->Vmem blts */
    DWORD   	dwNLVBCaps;             /* driver specific capabilities for non-local->local vidmem blts */
    DWORD   	dwNLVBCaps2;            /* more driver specific capabilities non-local->local vidmem blts */
    DWORD   	dwNLVBCKeyCaps;         /* driver color key capabilities for non-local->local vidmem blts */
    DWORD   	dwNLVBFXCaps;           /* driver FX capabilities for non-local->local blts */
    DWORD   	dwNLVBRops[DD_ROP_SPACE]; /* ROPS supported for non-local->local blts */
    DDSCAPS2    ddsCaps;		/* surface capabilities */
} DDCAPS_DX7,*LPDDCAPS_DX7;

typedef DDCAPS_DX7 DDCAPS;
typedef DDCAPS *LPDDCAPS;

/* DDCAPS.dwCaps */
#define DDCAPS_3D			0x00000001
#define DDCAPS_ALIGNBOUNDARYDEST	0x00000002
#define DDCAPS_ALIGNSIZEDEST		0x00000004
#define DDCAPS_ALIGNBOUNDARYSRC		0x00000008
#define DDCAPS_ALIGNSIZESRC		0x00000010
#define DDCAPS_ALIGNSTRIDE		0x00000020
#define DDCAPS_BLT			0x00000040
#define DDCAPS_BLTQUEUE			0x00000080
#define DDCAPS_BLTFOURCC		0x00000100
#define DDCAPS_BLTSTRETCH		0x00000200
#define DDCAPS_GDI			0x00000400
#define DDCAPS_OVERLAY			0x00000800
#define DDCAPS_OVERLAYCANTCLIP		0x00001000
#define DDCAPS_OVERLAYFOURCC		0x00002000
#define DDCAPS_OVERLAYSTRETCH		0x00004000
#define DDCAPS_PALETTE			0x00008000
#define DDCAPS_PALETTEVSYNC		0x00010000
#define DDCAPS_READSCANLINE		0x00020000
#define DDCAPS_STEREOVIEW		0x00040000
#define DDCAPS_VBI			0x00080000
#define DDCAPS_ZBLTS			0x00100000
#define DDCAPS_ZOVERLAYS		0x00200000
#define DDCAPS_COLORKEY			0x00400000
#define DDCAPS_ALPHA			0x00800000
#define DDCAPS_COLORKEYHWASSIST		0x01000000
#define DDCAPS_NOHARDWARE		0x02000000
#define DDCAPS_BLTCOLORFILL		0x04000000
#define DDCAPS_BANKSWITCHED		0x08000000
#define DDCAPS_BLTDEPTHFILL		0x10000000
#define DDCAPS_CANCLIP			0x20000000
#define DDCAPS_CANCLIPSTRETCHED		0x40000000
#define DDCAPS_CANBLTSYSMEM		0x80000000

/* DDCAPS.dwCaps2 */
#define DDCAPS2_CERTIFIED		0x00000001
#define DDCAPS2_NO2DDURING3DSCENE       0x00000002
#define DDCAPS2_VIDEOPORT		0x00000004
#define DDCAPS2_AUTOFLIPOVERLAY		0x00000008
#define DDCAPS2_CANBOBINTERLEAVED	0x00000010
#define DDCAPS2_CANBOBNONINTERLEAVED	0x00000020
#define DDCAPS2_COLORCONTROLOVERLAY	0x00000040
#define DDCAPS2_COLORCONTROLPRIMARY	0x00000080
#define DDCAPS2_CANDROPZ16BIT		0x00000100
#define DDCAPS2_NONLOCALVIDMEM		0x00000200
#define DDCAPS2_NONLOCALVIDMEMCAPS	0x00000400
#define DDCAPS2_NOPAGELOCKREQUIRED	0x00000800
#define DDCAPS2_WIDESURFACES		0x00001000
#define DDCAPS2_CANFLIPODDEVEN		0x00002000
#define DDCAPS2_CANBOBHARDWARE		0x00004000
#define DDCAPS2_COPYFOURCC              0x00008000
#define DDCAPS2_PRIMARYGAMMA            0x00020000
#define DDCAPS2_CANRENDERWINDOWED       0x00080000
#define DDCAPS2_CANCALIBRATEGAMMA       0x00100000
#define DDCAPS2_FLIPINTERVAL            0x00200000
#define DDCAPS2_FLIPNOVSYNC             0x00400000
#define DDCAPS2_CANMANAGETEXTURE        0x00800000
#define DDCAPS2_TEXMANINNONLOCALVIDMEM  0x01000000
#define DDCAPS2_STEREO                  0x02000000
#define DDCAPS2_SYSTONONLOCAL_AS_SYSTOLOCAL   0x04000000


/* Set/Get Colour Key Flags */
#define DDCKEY_COLORSPACE  0x00000001  /* Struct is single colour space */
#define DDCKEY_DESTBLT     0x00000002  /* To be used as dest for blt */
#define DDCKEY_DESTOVERLAY 0x00000004  /* To be used as dest for CK overlays */
#define DDCKEY_SRCBLT      0x00000008  /* To be used as src for blt */
#define DDCKEY_SRCOVERLAY  0x00000010  /* To be used as src for CK overlays */

typedef struct _DDCOLORKEY
{
	DWORD	dwColorSpaceLowValue;/* low boundary of color space that is to
                                      * be treated as Color Key, inclusive
				      */
	DWORD	dwColorSpaceHighValue;/* high boundary of color space that is
                                       * to be treated as Color Key, inclusive
				       */
} DDCOLORKEY,*LPDDCOLORKEY;

/* ddCKEYCAPS bits */
#define DDCKEYCAPS_DESTBLT			0x00000001
#define DDCKEYCAPS_DESTBLTCLRSPACE		0x00000002
#define DDCKEYCAPS_DESTBLTCLRSPACEYUV		0x00000004
#define DDCKEYCAPS_DESTBLTYUV			0x00000008
#define DDCKEYCAPS_DESTOVERLAY			0x00000010
#define DDCKEYCAPS_DESTOVERLAYCLRSPACE		0x00000020
#define DDCKEYCAPS_DESTOVERLAYCLRSPACEYUV	0x00000040
#define DDCKEYCAPS_DESTOVERLAYONEACTIVE		0x00000080
#define DDCKEYCAPS_DESTOVERLAYYUV		0x00000100
#define DDCKEYCAPS_SRCBLT			0x00000200
#define DDCKEYCAPS_SRCBLTCLRSPACE		0x00000400
#define DDCKEYCAPS_SRCBLTCLRSPACEYUV		0x00000800
#define DDCKEYCAPS_SRCBLTYUV			0x00001000
#define DDCKEYCAPS_SRCOVERLAY			0x00002000
#define DDCKEYCAPS_SRCOVERLAYCLRSPACE		0x00004000
#define DDCKEYCAPS_SRCOVERLAYCLRSPACEYUV	0x00008000
#define DDCKEYCAPS_SRCOVERLAYONEACTIVE		0x00010000
#define DDCKEYCAPS_SRCOVERLAYYUV		0x00020000
#define DDCKEYCAPS_NOCOSTOVERLAY		0x00040000

typedef struct _DDPIXELFORMAT {
    DWORD	dwSize;                 /* 0: size of structure */
    DWORD	dwFlags;                /* 4: pixel format flags */
    DWORD	dwFourCC;               /* 8: (FOURCC code) */
    union {
	DWORD	dwRGBBitCount;          /* C: how many bits per pixel */
	DWORD	dwYUVBitCount;          /* C: how many bits per pixel */
	DWORD	dwZBufferBitDepth;      /* C: how many bits for z buffers */
	DWORD	dwAlphaBitDepth;        /* C: how many bits for alpha channels*/
	DWORD	dwLuminanceBitCount;
	DWORD	dwBumpBitCount;
    } DUMMYUNIONNAME1;
    union {
	DWORD	dwRBitMask;             /* 10: mask for red bit*/
	DWORD	dwYBitMask;             /* 10: mask for Y bits*/
	DWORD	dwStencilBitDepth;
	DWORD	dwLuminanceBitMask;
	DWORD	dwBumpDuBitMask;
    } DUMMYUNIONNAME2;
    union {
	DWORD	dwGBitMask;             /* 14: mask for green bits*/
	DWORD	dwUBitMask;             /* 14: mask for U bits*/
	DWORD	dwZBitMask;
	DWORD	dwBumpDvBitMask;
    } DUMMYUNIONNAME3;
    union {
	DWORD   dwBBitMask;             /* 18: mask for blue bits*/
	DWORD   dwVBitMask;             /* 18: mask for V bits*/
	DWORD	dwStencilBitMask;
	DWORD	dwBumpLuminanceBitMask;
    } DUMMYUNIONNAME4;
    union {
    	DWORD	dwRGBAlphaBitMask;	/* 1C: mask for alpha channel */
    	DWORD	dwYUVAlphaBitMask;	/* 1C: mask for alpha channel */
	DWORD	dwLuminanceAlphaBitMask;
	DWORD	dwRGBZBitMask;		/* 1C: mask for Z channel */
	DWORD	dwYUVZBitMask;		/* 1C: mask for Z channel */
    } DUMMYUNIONNAME5;
    					/* 20: next structure */
} DDPIXELFORMAT,*LPDDPIXELFORMAT;

/* DDCAPS.dwFXCaps */
#define DDFXCAPS_BLTALPHA               0x00000001
#define DDFXCAPS_OVERLAYALPHA           0x00000004
#define DDFXCAPS_BLTARITHSTRETCHYN	0x00000010
#define DDFXCAPS_BLTARITHSTRETCHY	0x00000020
#define DDFXCAPS_BLTMIRRORLEFTRIGHT	0x00000040
#define DDFXCAPS_BLTMIRRORUPDOWN	0x00000080
#define DDFXCAPS_BLTROTATION		0x00000100
#define DDFXCAPS_BLTROTATION90		0x00000200
#define DDFXCAPS_BLTSHRINKX		0x00000400
#define DDFXCAPS_BLTSHRINKXN		0x00000800
#define DDFXCAPS_BLTSHRINKY		0x00001000
#define DDFXCAPS_BLTSHRINKYN		0x00002000
#define DDFXCAPS_BLTSTRETCHX		0x00004000
#define DDFXCAPS_BLTSTRETCHXN		0x00008000
#define DDFXCAPS_BLTSTRETCHY		0x00010000
#define DDFXCAPS_BLTSTRETCHYN		0x00020000
#define DDFXCAPS_OVERLAYARITHSTRETCHY	0x00040000
#define DDFXCAPS_OVERLAYARITHSTRETCHYN	0x00000008
#define DDFXCAPS_OVERLAYSHRINKX		0x00080000
#define DDFXCAPS_OVERLAYSHRINKXN	0x00100000
#define DDFXCAPS_OVERLAYSHRINKY		0x00200000
#define DDFXCAPS_OVERLAYSHRINKYN	0x00400000
#define DDFXCAPS_OVERLAYSTRETCHX	0x00800000
#define DDFXCAPS_OVERLAYSTRETCHXN	0x01000000
#define DDFXCAPS_OVERLAYSTRETCHY	0x02000000
#define DDFXCAPS_OVERLAYSTRETCHYN	0x04000000
#define DDFXCAPS_OVERLAYMIRRORLEFTRIGHT	0x08000000
#define DDFXCAPS_OVERLAYMIRRORUPDOWN	0x10000000

#define DDFXCAPS_OVERLAYFILTER          DDFXCAPS_OVERLAYARITHSTRETCHY

/* DDCAPS.dwFXAlphaCaps */
#define DDFXALPHACAPS_BLTALPHAEDGEBLEND		0x00000001
#define DDFXALPHACAPS_BLTALPHAPIXELS		0x00000002
#define DDFXALPHACAPS_BLTALPHAPIXELSNEG		0x00000004
#define DDFXALPHACAPS_BLTALPHASURFACES		0x00000008
#define DDFXALPHACAPS_BLTALPHASURFACESNEG	0x00000010
#define DDFXALPHACAPS_OVERLAYALPHAEDGEBLEND	0x00000020
#define DDFXALPHACAPS_OVERLAYALPHAPIXELS	0x00000040
#define DDFXALPHACAPS_OVERLAYALPHAPIXELSNEG	0x00000080
#define DDFXALPHACAPS_OVERLAYALPHASURFACES	0x00000100
#define DDFXALPHACAPS_OVERLAYALPHASURFACESNEG	0x00000200

/* DDCAPS.dwPalCaps */
#define DDPCAPS_4BIT			0x00000001
#define DDPCAPS_8BITENTRIES		0x00000002
#define DDPCAPS_8BIT			0x00000004
#define DDPCAPS_INITIALIZE		0x00000008
#define DDPCAPS_PRIMARYSURFACE		0x00000010
#define DDPCAPS_PRIMARYSURFACELEFT	0x00000020
#define DDPCAPS_ALLOW256		0x00000040
#define DDPCAPS_VSYNC			0x00000080
#define DDPCAPS_1BIT			0x00000100
#define DDPCAPS_2BIT			0x00000200
#define DDPCAPS_ALPHA                   0x00000400

/* DDCAPS.dwSVCaps */
/* the first 4 of these are now obsolete */
#if DIRECTDRAW_VERSION >= 0x700	/* FIXME: I'm not sure when this switch occurred */
#define DDSVCAPS_RESERVED1		0x00000001
#define DDSVCAPS_RESERVED2		0x00000002
#define DDSVCAPS_RESERVED3		0x00000004
#define DDSVCAPS_RESERVED4		0x00000008
#else
#define DDSVCAPS_ENIGMA			0x00000001
#define DDSVCAPS_FLICKER		0x00000002
#define DDSVCAPS_REDBLUE		0x00000004
#define DDSVCAPS_SPLIT			0x00000008
#endif
#define DDSVCAPS_STEREOSEQUENTIAL       0x00000010

/* BitDepths */
#define DDBD_1				0x00004000
#define DDBD_2				0x00002000
#define DDBD_4				0x00001000
#define DDBD_8				0x00000800
#define DDBD_16				0x00000400
#define DDBD_24				0x00000200
#define DDBD_32				0x00000100

/* DDOVERLAYFX.dwDDFX */
#define DDOVERFX_ARITHSTRETCHY		0x00000001
#define DDOVERFX_MIRRORLEFTRIGHT	0x00000002
#define DDOVERFX_MIRRORUPDOWN		0x00000004

/* UpdateOverlay flags */
#define DDOVER_ALPHADEST                        0x00000001
#define DDOVER_ALPHADESTCONSTOVERRIDE           0x00000002
#define DDOVER_ALPHADESTNEG                     0x00000004
#define DDOVER_ALPHADESTSURFACEOVERRIDE         0x00000008
#define DDOVER_ALPHAEDGEBLEND                   0x00000010
#define DDOVER_ALPHASRC                         0x00000020
#define DDOVER_ALPHASRCCONSTOVERRIDE            0x00000040
#define DDOVER_ALPHASRCNEG                      0x00000080
#define DDOVER_ALPHASRCSURFACEOVERRIDE          0x00000100
#define DDOVER_HIDE                             0x00000200
#define DDOVER_KEYDEST                          0x00000400
#define DDOVER_KEYDESTOVERRIDE                  0x00000800
#define DDOVER_KEYSRC                           0x00001000
#define DDOVER_KEYSRCOVERRIDE                   0x00002000
#define DDOVER_SHOW                             0x00004000
#define DDOVER_ADDDIRTYRECT                     0x00008000
#define DDOVER_REFRESHDIRTYRECTS                0x00010000
#define DDOVER_REFRESHALL                       0x00020000
#define DDOVER_DDFX                             0x00080000
#define DDOVER_AUTOFLIP                         0x00100000
#define DDOVER_BOB                              0x00200000
#define DDOVER_OVERRIDEBOBWEAVE                 0x00400000
#define DDOVER_INTERLEAVED                      0x00800000

/* DDCOLORKEY.dwFlags */
#define DDPF_ALPHAPIXELS		0x00000001
#define DDPF_ALPHA			0x00000002
#define DDPF_FOURCC			0x00000004
#define DDPF_PALETTEINDEXED4		0x00000008
#define DDPF_PALETTEINDEXEDTO8		0x00000010
#define DDPF_PALETTEINDEXED8		0x00000020
#define DDPF_RGB			0x00000040
#define DDPF_COMPRESSED			0x00000080
#define DDPF_RGBTOYUV			0x00000100
#define DDPF_YUV			0x00000200
#define DDPF_ZBUFFER			0x00000400
#define DDPF_PALETTEINDEXED1		0x00000800
#define DDPF_PALETTEINDEXED2		0x00001000
#define DDPF_ZPIXELS			0x00002000
#define DDPF_STENCILBUFFER              0x00004000
#define DDPF_ALPHAPREMULT               0x00008000
#define DDPF_LUMINANCE                  0x00020000
#define DDPF_BUMPLUMINANCE              0x00040000
#define DDPF_BUMPDUDV                   0x00080000

/* SetCooperativeLevel dwFlags */
#define DDSCL_FULLSCREEN		0x00000001
#define DDSCL_ALLOWREBOOT		0x00000002
#define DDSCL_NOWINDOWCHANGES		0x00000004
#define DDSCL_NORMAL			0x00000008
#define DDSCL_EXCLUSIVE			0x00000010
#define DDSCL_ALLOWMODEX		0x00000040
#define DDSCL_SETFOCUSWINDOW		0x00000080
#define DDSCL_SETDEVICEWINDOW		0x00000100
#define DDSCL_CREATEDEVICEWINDOW	0x00000200
#define DDSCL_MULTITHREADED             0x00000400
#define DDSCL_FPUSETUP                  0x00000800
#define DDSCL_FPUPRESERVE               0x00001000


/* DDSURFACEDESC.dwFlags */
#define	DDSD_CAPS		0x00000001
#define	DDSD_HEIGHT		0x00000002
#define	DDSD_WIDTH		0x00000004
#define	DDSD_PITCH		0x00000008
#define	DDSD_BACKBUFFERCOUNT	0x00000020
#define	DDSD_ZBUFFERBITDEPTH	0x00000040
#define	DDSD_ALPHABITDEPTH	0x00000080
#define	DDSD_LPSURFACE		0x00000800
#define	DDSD_PIXELFORMAT	0x00001000
#define	DDSD_CKDESTOVERLAY	0x00002000
#define	DDSD_CKDESTBLT		0x00004000
#define	DDSD_CKSRCOVERLAY	0x00008000
#define	DDSD_CKSRCBLT		0x00010000
#define	DDSD_MIPMAPCOUNT	0x00020000
#define	DDSD_REFRESHRATE	0x00040000
#define	DDSD_LINEARSIZE		0x00080000
#define DDSD_TEXTURESTAGE       0x00100000
#define DDSD_FVF                0x00200000
#define DDSD_SRCVBHANDLE        0x00400000
#define	DDSD_ALL		0x007ff9ee

/* EnumSurfaces flags */
#define DDENUMSURFACES_ALL          0x00000001
#define DDENUMSURFACES_MATCH        0x00000002
#define DDENUMSURFACES_NOMATCH      0x00000004
#define DDENUMSURFACES_CANBECREATED 0x00000008
#define DDENUMSURFACES_DOESEXIST    0x00000010

/* SetDisplayMode flags */
#define DDSDM_STANDARDVGAMODE	0x00000001

/* EnumDisplayModes flags */
#define DDEDM_REFRESHRATES	0x00000001
#define DDEDM_STANDARDVGAMODES	0x00000002

/* WaitForVerticalDisplay flags */

#define DDWAITVB_BLOCKBEGIN		0x00000001
#define DDWAITVB_BLOCKBEGINEVENT	0x00000002
#define DDWAITVB_BLOCKEND		0x00000004

typedef struct _DDSURFACEDESC
{
	DWORD	dwSize;		/* 0: size of the DDSURFACEDESC structure*/
	DWORD	dwFlags;	/* 4: determines what fields are valid*/
	DWORD	dwHeight;	/* 8: height of surface to be created*/
	DWORD	dwWidth;	/* C: width of input surface*/
	union {
		LONG	lPitch;	/* 10: distance to start of next line (return value only)*/
		DWORD	dwLinearSize;
	} DUMMYUNIONNAME1;
	DWORD	dwBackBufferCount;/* 14: number of back buffers requested*/
	union {
		DWORD	dwMipMapCount;/* 18:number of mip-map levels requested*/
		DWORD	dwZBufferBitDepth;/*18: depth of Z buffer requested*/
		DWORD	dwRefreshRate;/* 18:refresh rate (used when display mode is described)*/
	} DUMMYUNIONNAME2;
	DWORD	dwAlphaBitDepth;/* 1C:depth of alpha buffer requested*/
	DWORD	dwReserved;	/* 20:reserved*/
	LPVOID	lpSurface;	/* 24:pointer to the associated surface memory*/
	DDCOLORKEY	ddckCKDestOverlay;/* 28: CK for dest overlay use*/
	DDCOLORKEY	ddckCKDestBlt;	/* 30: CK for destination blt use*/
	DDCOLORKEY	ddckCKSrcOverlay;/* 38: CK for source overlay use*/
	DDCOLORKEY	ddckCKSrcBlt;	/* 40: CK for source blt use*/
	DDPIXELFORMAT	ddpfPixelFormat;/* 48: pixel format description of the surface*/
	DDSCAPS		ddsCaps;	/* 68: direct draw surface caps */
} DDSURFACEDESC,*LPDDSURFACEDESC;

typedef struct _DDSURFACEDESC2
{
	DWORD	dwSize;		/* 0: size of the DDSURFACEDESC2 structure*/
	DWORD	dwFlags;	/* 4: determines what fields are valid*/
	DWORD	dwHeight;	/* 8: height of surface to be created*/
	DWORD	dwWidth;	/* C: width of input surface*/
	union {
		LONG	lPitch;	      /*10: distance to start of next line (return value only)*/
		DWORD   dwLinearSize; /*10: formless late-allocated optimized surface size */
	} DUMMYUNIONNAME1;
	DWORD	dwBackBufferCount;/* 14: number of back buffers requested*/
	union {
		DWORD	dwMipMapCount;/* 18:number of mip-map levels requested*/
		DWORD	dwRefreshRate;/* 18:refresh rate (used when display mode is described)*/
		DWORD   dwSrcVBHandle;/* 18:source used in VB::Optimize */
	} DUMMYUNIONNAME2;
	DWORD	dwAlphaBitDepth;/* 1C:depth of alpha buffer requested*/
	DWORD	dwReserved;	/* 20:reserved*/
	LPVOID	lpSurface;	/* 24:pointer to the associated surface memory*/
	union {
		DDCOLORKEY	ddckCKDestOverlay; /* 28: CK for dest overlay use*/
		DWORD 		dwEmptyFaceColor;  /* 28: color for empty cubemap faces */
	} DUMMYUNIONNAME3;
	DDCOLORKEY	ddckCKDestBlt;	/* 30: CK for destination blt use*/
	DDCOLORKEY	ddckCKSrcOverlay;/* 38: CK for source overlay use*/
	DDCOLORKEY	ddckCKSrcBlt;	/* 40: CK for source blt use*/

	union {
		DDPIXELFORMAT	ddpfPixelFormat;/* 48: pixel format description of the surface*/
		DWORD 		dwFVF;	/* 48: vertex format description of vertex buffers */
	} DUMMYUNIONNAME4;
	DDSCAPS2	ddsCaps;  /* 68: DDraw surface caps */
	DWORD		dwTextureStage; /* 78: stage in multitexture cascade */
} DDSURFACEDESC2,*LPDDSURFACEDESC2;

/* DDCOLORCONTROL.dwFlags */
#define DDCOLOR_BRIGHTNESS	0x00000001
#define DDCOLOR_CONTRAST	0x00000002
#define DDCOLOR_HUE		0x00000004
#define DDCOLOR_SATURATION	0x00000008
#define DDCOLOR_SHARPNESS	0x00000010
#define DDCOLOR_GAMMA		0x00000020
#define DDCOLOR_COLORENABLE	0x00000040

typedef struct {
	DWORD	dwSize;
	DWORD	dwFlags;
	LONG	lBrightness;
	LONG	lContrast;
	LONG	lHue;
	LONG	lSaturation;
	LONG	lSharpness;
	LONG	lGamma;
	LONG	lColorEnable;
	DWORD	dwReserved1;
} DDCOLORCONTROL,*LPDDCOLORCONTROL;

typedef struct {
	WORD	red[256];
	WORD	green[256];
	WORD	blue[256];
} DDGAMMARAMP,*LPDDGAMMARAMP;

/* flags for DirectDrawEnumerateEx */
#define DDENUM_ATTACHEDSECONDARYDEVICES	0x00000001
#define DDENUM_DETACHEDSECONDARYDEVICES	0x00000002
#define DDENUM_NONDISPLAYDEVICES	0x00000004

/* flags for DirectDrawCreate or IDirectDraw::Initialize */
#define DDCREATE_HARDWAREONLY	1L
#define DDCREATE_EMULATIONONLY	2L

/* dwDDFX */
/* arithmetic stretching along y axis */
#define DDBLTFX_ARITHSTRETCHY			0x00000001
/* mirror on y axis */
#define DDBLTFX_MIRRORLEFTRIGHT			0x00000002
/* mirror on x axis */
#define DDBLTFX_MIRRORUPDOWN			0x00000004
/* do not tear */
#define DDBLTFX_NOTEARING			0x00000008
/* 180 degrees clockwise rotation */
#define DDBLTFX_ROTATE180			0x00000010
/* 270 degrees clockwise rotation */
#define DDBLTFX_ROTATE270			0x00000020
/* 90 degrees clockwise rotation */
#define DDBLTFX_ROTATE90			0x00000040
/* dwZBufferLow and dwZBufferHigh specify limits to the copied Z values */
#define DDBLTFX_ZBUFFERRANGE			0x00000080
/* add dwZBufferBaseDest to every source z value before compare */
#define DDBLTFX_ZBUFFERBASEDEST			0x00000100

typedef DWORD IDirectDrawSurface; /* FIXME: implement proper definition */
typedef IDirectDrawSurface *LPDIRECTDRAWSURFACE;
typedef struct _DDBLTFX
{
    DWORD       dwSize;                         /* size of structure */
    DWORD       dwDDFX;                         /* FX operations */
    DWORD       dwROP;                          /* Win32 raster operations */
    DWORD       dwDDROP;                        /* Raster operations new for DirectDraw */
    DWORD       dwRotationAngle;                /* Rotation angle for blt */
    DWORD       dwZBufferOpCode;                /* ZBuffer compares */
    DWORD       dwZBufferLow;                   /* Low limit of Z buffer */
    DWORD       dwZBufferHigh;                  /* High limit of Z buffer */
    DWORD       dwZBufferBaseDest;              /* Destination base value */
    DWORD       dwZDestConstBitDepth;           /* Bit depth used to specify Z constant for destination */
    union
    {
        DWORD   dwZDestConst;                   /* Constant to use as Z buffer for dest */
        LPDIRECTDRAWSURFACE lpDDSZBufferDest;   /* Surface to use as Z buffer for dest */
    } DUMMYUNIONNAME1;
    DWORD       dwZSrcConstBitDepth;            /* Bit depth used to specify Z constant for source */
    union
    {
        DWORD   dwZSrcConst;                    /* Constant to use as Z buffer for src */
        LPDIRECTDRAWSURFACE lpDDSZBufferSrc;    /* Surface to use as Z buffer for src */
    } DUMMYUNIONNAME2;
    DWORD       dwAlphaEdgeBlendBitDepth;       /* Bit depth used to specify constant for alpha edge blend */
    DWORD       dwAlphaEdgeBlend;               /* Alpha for edge blending */
    DWORD       dwReserved;
    DWORD       dwAlphaDestConstBitDepth;       /* Bit depth used to specify alpha constant for destination */
    union
    {
        DWORD   dwAlphaDestConst;               /* Constant to use as Alpha Channel */
        LPDIRECTDRAWSURFACE lpDDSAlphaDest;     /* Surface to use as Alpha Channel */
    } DUMMYUNIONNAME3;
    DWORD       dwAlphaSrcConstBitDepth;        /* Bit depth used to specify alpha constant for source */
    union
    {
        DWORD   dwAlphaSrcConst;                /* Constant to use as Alpha Channel */
        LPDIRECTDRAWSURFACE lpDDSAlphaSrc;      /* Surface to use as Alpha Channel */
    } DUMMYUNIONNAME4;
    union
    {
        DWORD   dwFillColor;                    /* color in RGB or Palettized */
        DWORD   dwFillDepth;                    /* depth value for z-buffer */
	DWORD   dwFillPixel;			/* pixel val for RGBA or RGBZ */
        LPDIRECTDRAWSURFACE lpDDSPattern;       /* Surface to use as pattern */
    } DUMMYUNIONNAME5;
    DDCOLORKEY  ddckDestColorkey;               /* DestColorkey override */
    DDCOLORKEY  ddckSrcColorkey;                /* SrcColorkey override */
} DDBLTFX,*LPDDBLTFX;
typedef struct _DDOVERLAYFX
{
    DWORD       dwSize;                         /* size of structure */
    DWORD       dwAlphaEdgeBlendBitDepth;       /* Bit depth used to specify constant for alpha edge blend */
    DWORD       dwAlphaEdgeBlend;               /* Constant to use as alpha for edge blend */
    DWORD       dwReserved;
    DWORD       dwAlphaDestConstBitDepth;       /* Bit depth used to specify alpha constant for destination */
    union
    {
        DWORD   dwAlphaDestConst;               /* Constant to use as alpha channel for dest */
        LPDIRECTDRAWSURFACE lpDDSAlphaDest;     /* Surface to use as alpha channel for dest */
    } DUMMYUNIONNAME1;
    DWORD       dwAlphaSrcConstBitDepth;        /* Bit depth used to specify alpha constant for source */
    union
    {
        DWORD   dwAlphaSrcConst;                /* Constant to use as alpha channel for src */
        LPDIRECTDRAWSURFACE lpDDSAlphaSrc;      /* Surface to use as alpha channel for src */
    } DUMMYUNIONNAME2;
    DDCOLORKEY  dckDestColorkey;                /* DestColorkey override */
    DDCOLORKEY  dckSrcColorkey;                 /* DestColorkey override */
    DWORD       dwDDFX;                         /* Overlay FX */
    DWORD       dwFlags;                        /* flags */
} DDOVERLAYFX,*LPDDOVERLAYFX;

typedef struct _DDBLTBATCH
{
    LPRECT		lprDest;
    LPDIRECTDRAWSURFACE	lpDDSSrc;
    LPRECT		lprSrc;
    DWORD		dwFlags;
    LPDDBLTFX		lpDDBltFx;
} DDBLTBATCH,*LPDDBLTBATCH;

#define MAX_DDDEVICEID_STRING          512

typedef struct tagDDDEVICEIDENTIFIER {
  char    szDriver[MAX_DDDEVICEID_STRING];
  char    szDescription[MAX_DDDEVICEID_STRING];
  LARGE_INTEGER  liDriverVersion;
  DWORD   dwVendorId;
  DWORD   dwDeviceId;
  DWORD   dwSubSysId;
  DWORD   dwRevision;
  GUID    guidDeviceIdentifier;
} DDDEVICEIDENTIFIER, * LPDDDEVICEIDENTIFIER;

typedef struct tagDDDEVICEIDENTIFIER2 {
  char    szDriver[MAX_DDDEVICEID_STRING];	/* user readable driver name */
  char    szDescription[MAX_DDDEVICEID_STRING]; /* user readable description */
  LARGE_INTEGER  liDriverVersion;		/* driver version */
  DWORD   dwVendorId;				/* vendor ID, zero if unknown */
  DWORD   dwDeviceId;				/* chipset ID, zero if unknown */
  DWORD   dwSubSysId;				/* board ID, zero if unknown */
  DWORD   dwRevision;				/* chipset version, zero if unknown */
  GUID    guidDeviceIdentifier;			/* unique ID for this driver/chipset combination */
  DWORD   dwWHQLLevel;				/* Windows Hardware Quality Lab certification level */
} DDDEVICEIDENTIFIER2, * LPDDDEVICEIDENTIFIER2;

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* __WINE_DDRAW_H */
