
/* Note : OsThunkDdQueryDirectDrawObject is the usermode name of NtGdiDdQueryDirectDrawObject
 *        it lives in d3d8thk.dll and in windows 2000 it doing syscall direcly to win32k.sus
 *        in windows xp and higher it call to gdi32.dll to DdEntry41 and it doing the syscall
 */
INT
Test_NtGdiDdQueryDirectDrawObject(PTESTINFO pti)
{
    HANDLE  hDirectDraw = NULL;
    DD_HALINFO *pHalInfo = NULL;
    DWORD *pCallBackFlags = NULL;
    LPD3DNTHAL_CALLBACKS puD3dCallbacks = NULL;
    LPD3DNTHAL_GLOBALDRIVERDATA puD3dDriverData = NULL;
    PDD_D3DBUFCALLBACKS puD3dBufferCallbacks = NULL;
    LPDDSURFACEDESC puD3dTextureFormats = NULL;
    DWORD *puNumHeaps = NULL;
    VIDEOMEMORY *puvmList = NULL;
    DWORD *puNumFourCC = NULL;
    DWORD *puFourCC = NULL;

    DD_HALINFO HalInfo;
    DD_HALINFO oldHalInfo;
    DWORD CallBackFlags[4];

    D3DNTHAL_CALLBACKS D3dCallbacks;
    D3DNTHAL_CALLBACKS oldD3dCallbacks;

    D3DNTHAL_GLOBALDRIVERDATA D3dDriverData;
    DD_D3DBUFCALLBACKS D3dBufferCallbacks;
    DDSURFACEDESC2 D3dTextureFormats[100];
    //DWORD NumHeaps = 0;
    VIDEOMEMORY vmList;
    //DWORD NumFourCC = 0;
    //DWORD FourCC = 0;
    DEVMODE devmode;
    HDC hdc;

    /* clear data */
    memset(&vmList,0,sizeof(VIDEOMEMORY));
    memset(&D3dTextureFormats,0,sizeof(DDSURFACEDESC));
    memset(&D3dBufferCallbacks,0,sizeof(DD_D3DBUFCALLBACKS));
    memset(&D3dDriverData,0,sizeof(D3DNTHAL_GLOBALDRIVERDATA));
    memset(&D3dCallbacks,0,sizeof(D3DNTHAL_CALLBACKS));
    memset(&HalInfo,0,sizeof(DD_HALINFO));
    memset(CallBackFlags,0,sizeof(DWORD)*3);

    /* Get currenet display mode */
    EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devmode);

    /* Create hdc that we can use */
    hdc = CreateDCW(L"DISPLAY",NULL,NULL,NULL);
    ASSERT(hdc != NULL);


    /* Create ReactX handle */
    hDirectDraw = (HANDLE) NtGdiDdCreateDirectDrawObject(hdc);
    ASSERT(hDirectDraw != NULL);

    /* Start Test ReactX NtGdiDdQueryDirectDrawObject function */

    /* testing  OsThunkDdQueryDirectDrawObject( NULL, ....  */
    RTEST(NtGdiDdQueryDirectDrawObject( NULL, pHalInfo,
                                        pCallBackFlags, puD3dCallbacks,
                                        puD3dDriverData, puD3dBufferCallbacks,
                                        puD3dTextureFormats, puNumHeaps,
                                        puvmList, puNumFourCC,
                                        puFourCC) == FALSE);

    RTEST(pHalInfo == NULL);
    RTEST(pCallBackFlags == NULL);
    RTEST(puD3dCallbacks == NULL);
    RTEST(puD3dDriverData == NULL);
    RTEST(puD3dBufferCallbacks == NULL);
    RTEST(puD3dTextureFormats == NULL);
    RTEST(puNumFourCC == NULL);
    RTEST(puFourCC == NULL);
    RTEST(puNumHeaps == NULL);
    RTEST(puvmList == NULL);

    /* testing  NtGdiDdQueryDirectDrawObject( hDirectDrawLocal, NULL, ....  */
    RTEST(NtGdiDdQueryDirectDrawObject( hDirectDraw, pHalInfo,
                                        pCallBackFlags, puD3dCallbacks,
                                        puD3dDriverData, puD3dBufferCallbacks,
                                        puD3dTextureFormats, puNumHeaps,
                                        puvmList, puNumFourCC,
                                        puFourCC) == FALSE);

    RTEST(pHalInfo == NULL);
    RTEST(pCallBackFlags == NULL);
    RTEST(puD3dCallbacks == NULL);
    RTEST(puD3dDriverData == NULL);
    RTEST(puD3dBufferCallbacks == NULL);
    RTEST(puD3dTextureFormats == NULL);
    RTEST(puNumFourCC == NULL);
    RTEST(puFourCC == NULL);
    RTEST(puNumHeaps == NULL);
    RTEST(puvmList == NULL);

    /* testing  NtGdiDdQueryDirectDrawObject( hDirectDrawLocal, pHalInfo, NULL, ....  */
    pHalInfo = &HalInfo;
    RTEST(NtGdiDdQueryDirectDrawObject( hDirectDraw, pHalInfo,
                                        pCallBackFlags, puD3dCallbacks,
                                        puD3dDriverData, puD3dBufferCallbacks,
                                        puD3dTextureFormats, puNumHeaps,
                                        puvmList, puNumFourCC,
                                        puFourCC)== FALSE);

    RTEST(pHalInfo != NULL);
    ASSERT(pHalInfo != NULL);

    RTEST(pCallBackFlags == NULL);
    RTEST(puD3dCallbacks == NULL);
    RTEST(puD3dDriverData == NULL);
    RTEST(puD3dBufferCallbacks == NULL);
    RTEST(puD3dTextureFormats == NULL);
    RTEST(puNumFourCC == NULL);
    RTEST(puFourCC == NULL);	
    RTEST(puNumHeaps == NULL);
    RTEST(puvmList == NULL);

    if ((pHalInfo->dwSize != sizeof(DD_HALINFO)) &&
        (pHalInfo->dwSize != sizeof(DD_HALINFO_V4)))
    {
        RTEST(pHalInfo->dwSize != sizeof(DD_HALINFO));
        ASSERT(pHalInfo->dwSize != sizeof(DD_HALINFO));
    }

    if (pHalInfo->dwSize == sizeof(DD_HALINFO))
    {
        /*the offset, in bytes, to primary surface in the display memory  */
        /* some graphic card like  sis 760 GX, Nvida GF7900GS does not set any offset at all */
        // RTEST(pHalInfo->vmiData.fpPrimary != 0 );

        /* unsuse always 0 */
        RTEST(pHalInfo->vmiData.dwFlags == 0 );

        /* Check the res */
        RTEST(pHalInfo->vmiData.dwDisplayWidth == devmode.dmPelsWidth );
        RTEST(pHalInfo->vmiData.dwDisplayHeight == devmode.dmPelsHeight );

        /*  This can never be test for it is who big the line is after it been align displayPitch */
        RTEST(pHalInfo->vmiData.lDisplayPitch != 0);

        RTEST(pHalInfo->vmiData.ddpfDisplay.dwSize == sizeof(DDPIXELFORMAT) );
        ASSERT(pHalInfo->vmiData.ddpfDisplay.dwSize == sizeof(DDPIXELFORMAT));

        /* We can not check if it DDPF_RGB flags been set for primary surface 
         * for it can be DDPF_PALETTEINDEXED1,DDPF_PALETTEINDEXED2,DDPF_PALETTEINDEXED4,DDPF_PALETTEINDEXED8, DDPF_PALETTEINDEXEDTO8, DDPF_RGB, DDPF_YUV
         */
        RTEST( (pHalInfo->vmiData.ddpfDisplay.dwFlags & (DDPF_PALETTEINDEXED1 | DDPF_PALETTEINDEXED2 | DDPF_PALETTEINDEXED4 | 
                                                         DDPF_PALETTEINDEXED8 | DDPF_PALETTEINDEXEDTO8 | DDPF_RGB | DDPF_YUV)) != 0);


        /* No fourcc are use on primary screen */
        RTEST(pHalInfo->vmiData.ddpfDisplay.dwFourCC == 0 );

        /* Count RGB Bits 8/16/24/32 */
        RTEST(pHalInfo->vmiData.ddpfDisplay.dwRGBBitCount == devmode.dmBitsPerPel );

        /* The rgb mask can not be detected in user mode, for it can be 15Bpp convert to 16Bpp mode, so we have no way detect correct mask  
         * But the mask can never be Zero 
         */
        RTEST(pHalInfo->vmiData.ddpfDisplay.dwRBitMask  !=  0 );
        RTEST(pHalInfo->vmiData.ddpfDisplay.dwGBitMask !=  0 );
        RTEST(pHalInfo->vmiData.ddpfDisplay.dwBBitMask != 0 );

        /* primary never set the alpha blend mask */
        RTEST(pHalInfo->vmiData.ddpfDisplay.dwRGBAlphaBitMask ==  0 );

        /* This can not be test at usermode it is each hardware drv that fill in it, 
         * only way to found them is to use this call  */
        // pHalInfo->vmiData->dwOffscreenAlign
        // pHalInfo->vmiData->dwOverlayAlign
        // pHalInfo->vmiData->dwTextureAlign
        // pHalInfo->vmiData->dwZBufferAlign
        // pHalInfo->vmiData->dwAlphaAlign 

        /* the primary display address */
        RTEST( ( (DWORD)pHalInfo->vmiData.pvPrimary & (~0x80000000)) != 0 );

        /* test see if we got back the pvmList here 
         * acording msdn vmiData.dwNumHeaps and vmiData.pvmList
         * exists for _VIDEOMEMORYINFO but they do not, it reviews 
         * in ddk and wdk and own testcase 
         */         
         // RTEST(pHalInfo->vmiData.dwNumHeaps  != 0 );
         // RTEST(pHalInfo->vmiData.pvmList  != 0 );

        /* Test see if we got any hardware acclartions for 2d or 3d, this always fill in 
         * that mean we found a bugi drv and dx does not work on this drv 
         */

        /* the SIS 760 GX will never fill it in, it is a bugi drv */
        RTEST(pHalInfo->ddCaps.dwSize == sizeof(DDCORECAPS));

        /* Testing see if we got any hw support for
         * This test can fail on video card that does not support 2d/overlay/3d
         */
        RTEST( pHalInfo->ddCaps.dwCaps != 0);
        RTEST( pHalInfo->ddCaps.ddsCaps.dwCaps != 0);

        /* if this fail we do not have a dx driver install acodring ms, some version of windows it
         * is okay this fail and drv does then only support basic dx
         */
        RTEST( (pHalInfo->dwFlags & (DDHALINFO_GETDRIVERINFOSET | DDHALINFO_GETDRIVERINFO2)) != 0 );

        /* point to kmode direcly to the graphic drv, the drv is kmode and it is kmode address we getting back*/
        RTEST( ( (DWORD)pHalInfo->GetDriverInfo & (~0x80000000)) != 0 );
        ASSERT( ((DWORD)pHalInfo->GetDriverInfo & (~0x80000000)) != 0 );

       /* the pHalInfo->ddCaps.ddsCaps.dwCaps & DDSCAPS_3DDEVICE will be ignore, only way detect it proper follow code,
        * this will be fill in of all drv, it is not only for 3d stuff, this always fill by win32k.sys or dxg.sys depns 
        * if it windows 2000 or windows xp/2003
        *
        * point to kmode direcly to the win32k.sys, win32k.sys is kmode and it is kmode address we getting back
        */
        RTEST( ( (DWORD)pHalInfo->lpD3DGlobalDriverData & (~0x80000000)) != 0 );
        RTEST( ( (DWORD)pHalInfo->lpD3DHALCallbacks & (~0x80000000)) != 0 );
        RTEST( ( (DWORD)pHalInfo->lpD3DHALCallbacks & (~0x80000000)) != 0 );
    }

    /* Backup DD_HALINFO so we do not need resting it */
    RtlCopyMemory(&oldHalInfo, &HalInfo, sizeof(DD_HALINFO));

    /* testing  NtGdiDdQueryDirectDrawObject( hDirectDrawLocal, pHalInfo, pCallBackFlags, NULL, ....  */
    pHalInfo = &HalInfo;
    pCallBackFlags = CallBackFlags;
    RtlZeroMemory(pHalInfo,sizeof(DD_HALINFO));

    RTEST(NtGdiDdQueryDirectDrawObject( hDirectDraw, pHalInfo,
                                        pCallBackFlags, puD3dCallbacks,
                                        puD3dDriverData, puD3dBufferCallbacks,
                                        puD3dTextureFormats, puNumHeaps,
                                        puvmList, puNumFourCC,
                                        puFourCC)== FALSE);
    RTEST(pHalInfo != NULL);
    ASSERT(pHalInfo != NULL);

    RTEST(pCallBackFlags != NULL);
    ASSERT(pCallBackFlags != NULL);

    RTEST(puD3dCallbacks == NULL);
    RTEST(puD3dDriverData == NULL);
    RTEST(puD3dBufferCallbacks == NULL);
    RTEST(puD3dTextureFormats == NULL);
    RTEST(puNumFourCC == NULL);
    RTEST(puFourCC == NULL);
    RTEST(puNumHeaps == NULL);
    RTEST(puvmList == NULL);

    /* We do not retesting DD_HALINFO, instead we compare it */
    RTEST(memcmp(&oldHalInfo, pHalInfo, sizeof(DD_HALINFO)) == 0);

    /* Rember on some nivida drv the pCallBackFlags will not be set even they api exists in the drv
     * known workaround is to check if the drv really return a kmode pointer for the drv functions 
     * we want to use.
     */
    RTEST(pCallBackFlags[0] != 0);
    RTEST(pCallBackFlags[1] != 0);
    RTEST(pCallBackFlags[2] == 0);

    /* testing  NtGdiDdQueryDirectDrawObject( hDirectDrawLocal, pHalInfo, pCallBackFlags, D3dCallbacks, NULL, ....  */
    pHalInfo = &HalInfo;
    pCallBackFlags = CallBackFlags;
    puD3dCallbacks = &D3dCallbacks;

    RtlZeroMemory(pHalInfo,sizeof(DD_HALINFO));
    RtlZeroMemory(pCallBackFlags,sizeof(DWORD)*3);

    RTEST(NtGdiDdQueryDirectDrawObject( hDirectDraw, pHalInfo,
                                        pCallBackFlags, puD3dCallbacks,
                                        puD3dDriverData, puD3dBufferCallbacks,
                                        puD3dTextureFormats, puNumHeaps,
                                        puvmList, puNumFourCC,
                                        puFourCC)== FALSE);
    RTEST(pHalInfo != NULL);
    ASSERT(pHalInfo != NULL);

    RTEST(pCallBackFlags != NULL);
    ASSERT(pCallBackFlags != NULL);

    /* rember puD3dCallbacks shall never return NULL */
    RTEST(puD3dCallbacks != NULL);
    ASSERT(puD3dCallbacks != NULL);

    /* the pHalInfo->ddCaps.ddsCaps.dwCaps & DDSCAPS_3DDEVICE will be ignore, only way detect it proper follow code,
     * this will be fill in of all drv, it is not only for 3d stuff, this always fill by win32k.sys or dxg.sys depns 
     * if it windows 2000 or windows xp/2003
     */
    RTEST(puD3dCallbacks->dwSize == sizeof(D3DNTHAL_CALLBACKS));

    /* Nivda like GF7900GS will not follow ms design rule here, 
     * ContextDestroyAll must alwyas be NULL for it is not longer inuse in windows 2000 and higher
     */
    RTEST(puD3dCallbacks->ContextDestroyAll == NULL);

    /* Nivda like GF7900GS will not follow ms design rule here, 
     * SceneCapture must alwyas be NULL for it is not longer inuse in windows 2000 and higher
     */
    RTEST(puD3dCallbacks->SceneCapture  == NULL);
    RTEST(puD3dCallbacks->dwReserved10 == 0);
    RTEST(puD3dCallbacks->dwReserved11 == 0);
    RTEST(puD3dCallbacks->dwReserved22 == 0);
    RTEST(puD3dCallbacks->dwReserved23 == 0);
    RTEST(puD3dCallbacks->dwReserved == 0);
    RTEST(puD3dCallbacks->TextureCreate  == NULL);
    RTEST(puD3dCallbacks->TextureDestroy  == NULL);
    RTEST(puD3dCallbacks->TextureSwap  == NULL);
    RTEST(puD3dCallbacks->TextureGetSurf  == NULL);
    RTEST(puD3dCallbacks->dwReserved12 == 0);
    RTEST(puD3dCallbacks->dwReserved13 == 0);
    RTEST(puD3dCallbacks->dwReserved14 == 0);
    RTEST(puD3dCallbacks->dwReserved15 == 0);
    RTEST(puD3dCallbacks->dwReserved16 == 0);
    RTEST(puD3dCallbacks->dwReserved17 == 0);
    RTEST(puD3dCallbacks->dwReserved18 == 0);
    RTEST(puD3dCallbacks->dwReserved19 == 0);
    RTEST(puD3dCallbacks->dwReserved20 == 0);
    RTEST(puD3dCallbacks->dwReserved21 == 0);
    RTEST(puD3dCallbacks->dwReserved24 == 0);
    RTEST(puD3dCallbacks->dwReserved0 == 0);
    RTEST(puD3dCallbacks->dwReserved1 == 0);
    RTEST(puD3dCallbacks->dwReserved2 == 0);
    RTEST(puD3dCallbacks->dwReserved3 == 0);
    RTEST(puD3dCallbacks->dwReserved4 == 0);
    RTEST(puD3dCallbacks->dwReserved5 == 0);
    RTEST(puD3dCallbacks->dwReserved6 == 0);
    RTEST(puD3dCallbacks->dwReserved7 == 0);
    RTEST(puD3dCallbacks->dwReserved8 == 0);
    RTEST(puD3dCallbacks->dwReserved9 == 0);

    /* how detect puD3dCallbacks->ContextCreate and puD3dCallbacks->ContextDestroy shall be set for bugi drv like nivda ? */
    /* pointer direcly to the graphic drv, it is kmode pointer */
    // RTEST( ( (DWORD)puD3dCallbacks->ContextCreate & (~0x80000000)) != 0 );
    // RTEST( ( (DWORD)puD3dCallbacks->ContextDestroy & (~0x80000000)) != 0 );

    RTEST(puD3dDriverData == NULL);
    RTEST(puD3dBufferCallbacks == NULL);
    RTEST(puD3dTextureFormats == NULL);
    RTEST(puNumFourCC == NULL);
    RTEST(puFourCC == NULL);
    RTEST(puNumHeaps == NULL);
    RTEST(puvmList == NULL);

    /* We do not retesting DD_HALINFO, instead we compare it */
    RTEST(memcmp(&oldHalInfo, pHalInfo, sizeof(DD_HALINFO)) == 0);
    RTEST(pCallBackFlags[0] != 0);
    RTEST(pCallBackFlags[1] != 0);
    RTEST(pCallBackFlags[2] == 0);

    /* Backup D3DNTHAL_CALLBACKS so we do not need resting it */
    RtlCopyMemory(&oldD3dCallbacks, &D3dCallbacks, sizeof(D3DNTHAL_CALLBACKS));


/* testing  NtGdiDdQueryDirectDrawObject( hDD, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, NULL, */
    pHalInfo = &HalInfo;
    pCallBackFlags = CallBackFlags;
    puD3dCallbacks = &D3dCallbacks;
    puD3dDriverData = &D3dDriverData;

    RtlZeroMemory(pHalInfo,sizeof(DD_HALINFO));
    RtlZeroMemory(pCallBackFlags,sizeof(DWORD)*3);
    RtlZeroMemory(puD3dCallbacks,sizeof(D3DNTHAL_CALLBACKS));

    RTEST(NtGdiDdQueryDirectDrawObject( hDirectDraw, pHalInfo,
                                        pCallBackFlags, puD3dCallbacks,
                                        puD3dDriverData, puD3dBufferCallbacks,
                                        puD3dTextureFormats, puNumHeaps,
                                        puvmList, puNumFourCC,
                                        puFourCC)== FALSE);
    RTEST(pHalInfo != NULL);
    ASSERT(pHalInfo != NULL);

    RTEST(pCallBackFlags != NULL);
    ASSERT(pCallBackFlags != NULL);

    RTEST(puD3dCallbacks != NULL);
    ASSERT(puD3dCallbacks != NULL);

    RTEST(puD3dDriverData != NULL);
    ASSERT(puD3dDriverData != NULL);

    RTEST(puD3dBufferCallbacks == NULL);
    RTEST(puD3dTextureFormats == NULL);
    RTEST(puNumFourCC == NULL);
    RTEST(puFourCC == NULL);
    RTEST(puNumHeaps == NULL);
    RTEST(puvmList == NULL);

    /* We do not retesting DD_HALINFO, instead we compare it */
    RTEST(memcmp(&oldHalInfo, pHalInfo, sizeof(DD_HALINFO)) == 0);
    RTEST(pCallBackFlags[0] != 0);
    RTEST(pCallBackFlags[1] != 0);
    RTEST(pCallBackFlags[2] == 0);

    /* We do not retesting D3DNTHAL_CALLBACKS, instead we compare it */
    RTEST(memcmp(&oldD3dCallbacks, puD3dCallbacks, sizeof(D3DNTHAL_CALLBACKS)) == 0);

    /* start test of puD3dDriverData */

/* Next Start 5 */
    pHalInfo = &HalInfo;
    pCallBackFlags = CallBackFlags;
    puD3dCallbacks = &D3dCallbacks;
    puD3dDriverData = &D3dDriverData;
    puD3dBufferCallbacks = &D3dBufferCallbacks;

    RtlZeroMemory(pHalInfo,sizeof(DD_HALINFO));
    RtlZeroMemory(pCallBackFlags,sizeof(DWORD)*3);
    RtlZeroMemory(puD3dCallbacks,sizeof(D3DNTHAL_CALLBACKS));
    RtlZeroMemory(puD3dDriverData,sizeof(D3DNTHAL_CALLBACKS));

    RTEST(NtGdiDdQueryDirectDrawObject( hDirectDraw, pHalInfo,
                                        pCallBackFlags, puD3dCallbacks,
                                        puD3dDriverData, puD3dBufferCallbacks,
                                        puD3dTextureFormats, puNumHeaps,
                                        puvmList, puNumFourCC,
                                        puFourCC)== FALSE);
    RTEST(pHalInfo != NULL);
    RTEST(pCallBackFlags != NULL);

    if (pHalInfo->ddCaps.ddsCaps.dwCaps  & DDSCAPS_3DDEVICE )
    {
        RTEST(puD3dCallbacks != NULL);
        RTEST(puD3dDriverData != NULL);
        RTEST(puD3dBufferCallbacks != NULL);
    }


    RTEST(puD3dTextureFormats == NULL);
    RTEST(puNumFourCC == NULL);
    RTEST(puFourCC == NULL);
    RTEST(puNumHeaps == NULL);
    RTEST(puvmList == NULL);
    ASSERT(pHalInfo != NULL);

    /* We do not retesting DD_HALINFO, instead we compare it */
    RTEST(memcmp(&oldHalInfo, pHalInfo, sizeof(DD_HALINFO)) == 0);
    RTEST(pCallBackFlags[0] != 0);
    RTEST(pCallBackFlags[1] != 0);
    RTEST(pCallBackFlags[2] == 0);

    /* We do not retesting D3DNTHAL_CALLBACKS, instead we compare it */
    RTEST(memcmp(&oldD3dCallbacks, puD3dCallbacks, sizeof(D3DNTHAL_CALLBACKS)) == 0);

/* Next Start 6 */
    pHalInfo = &HalInfo;
    pCallBackFlags = CallBackFlags;
    puD3dCallbacks = &D3dCallbacks;
    puD3dDriverData = &D3dDriverData;
    puD3dBufferCallbacks = &D3dBufferCallbacks;
    
    RtlZeroMemory(pHalInfo,sizeof(DD_HALINFO));
    RtlZeroMemory(pCallBackFlags,sizeof(DWORD)*3);
    RtlZeroMemory(puD3dCallbacks,sizeof(D3DNTHAL_CALLBACKS));
    RtlZeroMemory(puD3dDriverData,sizeof(D3DNTHAL_GLOBALDRIVERDATA));
    RtlZeroMemory(&D3dBufferCallbacks,sizeof(DD_D3DBUFCALLBACKS));

    RTEST(NtGdiDdQueryDirectDrawObject( hDirectDraw, pHalInfo,
                                        pCallBackFlags, puD3dCallbacks,
                                        puD3dDriverData, puD3dBufferCallbacks,
                                        puD3dTextureFormats, puNumHeaps,
                                        puvmList, puNumFourCC,
                                        puFourCC)== FALSE);
    RTEST(pHalInfo != NULL);
    RTEST(pCallBackFlags != NULL);

    if (pHalInfo->ddCaps.ddsCaps.dwCaps  & DDSCAPS_3DDEVICE )
    {
        RTEST(puD3dCallbacks != NULL);
        RTEST(puD3dDriverData != NULL);
        RTEST(puD3dBufferCallbacks != NULL);
    }

    RTEST(puD3dTextureFormats == NULL);
    RTEST(puNumFourCC == NULL);
    RTEST(puFourCC == NULL);
    RTEST(puNumHeaps == NULL);
    RTEST(puvmList == NULL);
    ASSERT(pHalInfo != NULL);

    /* We do not retesting DD_HALINFO, instead we compare it */
    RTEST(memcmp(&oldHalInfo, pHalInfo, sizeof(DD_HALINFO)) == 0);
    RTEST(pCallBackFlags[0] != 0);
    RTEST(pCallBackFlags[1] != 0);
    RTEST(pCallBackFlags[2] == 0);

    /* We do not retesting D3DNTHAL_CALLBACKS, instead we compare it */
    RTEST(memcmp(&oldD3dCallbacks, puD3dCallbacks, sizeof(D3DNTHAL_CALLBACKS)) == 0);


/* Next Start 7 */
    pHalInfo = &HalInfo;
    pCallBackFlags = CallBackFlags;
    puD3dCallbacks = &D3dCallbacks;
    puD3dDriverData = &D3dDriverData;
    puD3dBufferCallbacks = &D3dBufferCallbacks;

    /* It is forbein to return a  DDSURFACEDESC2 it should always be DDSURFACEDESC
        This is only for detected bad drivers that does not follow the rules, if they
        does not follow tthe rules, not everthing being copy then in gdi32.dll
        gdi32.dll always assume it is DDSURFACEDESC size
    */
    if (puD3dDriverData->dwNumTextureFormats != 0)
    {
        puD3dTextureFormats = malloc (puD3dDriverData->dwNumTextureFormats * sizeof(DDSURFACEDESC2));
    }

    RtlZeroMemory(pHalInfo,sizeof(DD_HALINFO));
    RtlZeroMemory(pCallBackFlags,sizeof(DWORD)*3);
    RtlZeroMemory(puD3dCallbacks,sizeof(D3DNTHAL_CALLBACKS));
    RtlZeroMemory(puD3dDriverData,sizeof(D3DNTHAL_GLOBALDRIVERDATA));
    RtlZeroMemory(&D3dBufferCallbacks,sizeof(DD_D3DBUFCALLBACKS));

    RTEST(NtGdiDdQueryDirectDrawObject( hDirectDraw, pHalInfo,
                                        pCallBackFlags, puD3dCallbacks,
                                        puD3dDriverData, puD3dBufferCallbacks,
                                        puD3dTextureFormats, puNumHeaps,
                                        puvmList, puNumFourCC,
                                        puFourCC)== FALSE);
    RTEST(pHalInfo != NULL);
    RTEST(pCallBackFlags != NULL);

    if (pHalInfo->ddCaps.ddsCaps.dwCaps  & DDSCAPS_3DDEVICE )
    {
        RTEST(puD3dCallbacks != NULL);
        RTEST(puD3dDriverData != NULL);
    RTEST(puD3dBufferCallbacks != NULL);
    if (puD3dDriverData->dwNumTextureFormats != 0)
    {
            /* FIXME add a better test for texture */
            RTEST(puD3dTextureFormats != NULL);
        }
    }

    RTEST(puNumFourCC == NULL);
    RTEST(puFourCC == NULL);
    RTEST(puNumHeaps == NULL);
    RTEST(puvmList == NULL);
    ASSERT(pHalInfo != NULL);

    /* We do not retesting DD_HALINFO, instead we compare it */
    RTEST(memcmp(&oldHalInfo, pHalInfo, sizeof(DD_HALINFO)) == 0);
    RTEST(pCallBackFlags[0] != 0);
    RTEST(pCallBackFlags[1] != 0);
    RTEST(pCallBackFlags[2] == 0);

    /* We do not retesting D3DNTHAL_CALLBACKS, instead we compare it */
    RTEST(memcmp(&oldD3dCallbacks, puD3dCallbacks, sizeof(D3DNTHAL_CALLBACKS)) == 0);



    /* Todo
    * adding test for
    * puD3dCallbacks
    * puD3dDriverData
    * puD3dBufferCallbacks
    * puNumFourCC
    * puFourCC
    * puNumHeaps
    * puvmList
    */

    /* Cleanup ReactX setup */
    DeleteDC(hdc);
    Syscall(L"NtGdiDdDeleteDirectDrawObject", 1, &hDirectDraw);

    return APISTATUS_NORMAL;
}
