/*
 * RichEdit - functions and interfaces around CreateTextServices
 *
 * Copyright 2005, 2006, Maarten Lankhorst
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

#include "editor.h"


#ifdef __i386__  /* thiscall functions are i386-specific */

#define THISCALL(func) __thiscall_ ## func
#define DEFINE_THISCALL_WRAPPER(func,args) \
   extern typeof(func) THISCALL(func); \
   __ASM_STDCALL_FUNC(__thiscall_ ## func, args, \
                   "popl %eax\n\t" \
                   "pushl %ecx\n\t" \
                   "pushl %eax\n\t" \
                   "jmp " __ASM_NAME(#func) __ASM_STDCALL(args) )
#else /* __i386__ */

#define THISCALL(func) func
#define DEFINE_THISCALL_WRAPPER(func,args) /* nothing */

#endif /* __i386__ */

WINE_DEFAULT_DEBUG_CHANNEL(richedit);

typedef struct ITextServicesImpl {
   IUnknown IUnknown_inner;
   ITextServices ITextServices_iface;
   IUnknown *outer_unk;
   LONG ref;
   ITextHost *pMyHost;
   CRITICAL_SECTION csTxtSrv;
   ME_TextEditor *editor;
   char spare[256];
} ITextServicesImpl;

static inline ITextServicesImpl *impl_from_IUnknown(IUnknown *iface)
{
   return CONTAINING_RECORD(iface, ITextServicesImpl, IUnknown_inner);
}

static HRESULT WINAPI ITextServicesImpl_QueryInterface(IUnknown *iface, REFIID riid, void **ppv)
{
   ITextServicesImpl *This = impl_from_IUnknown(iface);

   TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), ppv);

   if (IsEqualIID(riid, &IID_IUnknown))
      *ppv = &This->IUnknown_inner;
   else if (IsEqualIID(riid, &IID_ITextServices))
      *ppv = &This->ITextServices_iface;
   else if (IsEqualIID(riid, &IID_IRichEditOle) || IsEqualIID(riid, &IID_ITextDocument)) {
      if (!This->editor->reOle)
         if (!CreateIRichEditOle(This->outer_unk, This->editor, (void **)(&This->editor->reOle)))
            return E_OUTOFMEMORY;
      if (IsEqualIID(riid, &IID_ITextDocument))
         ME_GetITextDocumentInterface(This->editor->reOle, ppv);
      else
         *ppv = This->editor->reOle;
   } else {
      *ppv = NULL;
      FIXME("Unknown interface: %s\n", debugstr_guid(riid));
      return E_NOINTERFACE;
   }

   IUnknown_AddRef((IUnknown*)*ppv);
   return S_OK;
}

static ULONG WINAPI ITextServicesImpl_AddRef(IUnknown *iface)
{
   ITextServicesImpl *This = impl_from_IUnknown(iface);
   LONG ref = InterlockedIncrement(&This->ref);

   TRACE("(%p) ref=%d\n", This, ref);

   return ref;
}

static ULONG WINAPI ITextServicesImpl_Release(IUnknown *iface)
{
   ITextServicesImpl *This = impl_from_IUnknown(iface);
   LONG ref = InterlockedDecrement(&This->ref);

   TRACE("(%p) ref=%d\n", This, ref);

   if (!ref)
   {
      ME_DestroyEditor(This->editor);
      This->csTxtSrv.DebugInfo->Spare[0] = 0;
      DeleteCriticalSection(&This->csTxtSrv);
      CoTaskMemFree(This);
   }
   return ref;
}

static const IUnknownVtbl textservices_inner_vtbl =
{
   ITextServicesImpl_QueryInterface,
   ITextServicesImpl_AddRef,
   ITextServicesImpl_Release
};

static inline ITextServicesImpl *impl_from_ITextServices(ITextServices *iface)
{
   return CONTAINING_RECORD(iface, ITextServicesImpl, ITextServices_iface);
}

static HRESULT WINAPI fnTextSrv_QueryInterface(ITextServices *iface, REFIID riid, void **ppv)
{
   ITextServicesImpl *This = impl_from_ITextServices(iface);
   return IUnknown_QueryInterface(This->outer_unk, riid, ppv);
}

static ULONG WINAPI fnTextSrv_AddRef(ITextServices *iface)
{
   ITextServicesImpl *This = impl_from_ITextServices(iface);
   return IUnknown_AddRef(This->outer_unk);
}

static ULONG WINAPI fnTextSrv_Release(ITextServices *iface)
{
   ITextServicesImpl *This = impl_from_ITextServices(iface);
   return IUnknown_Release(This->outer_unk);
}

DECLSPEC_HIDDEN HRESULT WINAPI fnTextSrv_TxSendMessage(ITextServices *iface, UINT msg, WPARAM wparam,
                                       LPARAM lparam, LRESULT *plresult)
{
   ITextServicesImpl *This = impl_from_ITextServices(iface);
   HRESULT hresult;
   LRESULT lresult;

   lresult = ME_HandleMessage(This->editor, msg, wparam, lparam, TRUE, &hresult);
   if (plresult) *plresult = lresult;
   return hresult;
}

DECLSPEC_HIDDEN HRESULT WINAPI fnTextSrv_TxDraw(ITextServices *iface, DWORD dwDrawAspect, LONG lindex,
                                void *pvAspect, DVTARGETDEVICE *ptd, HDC hdcDraw, HDC hdcTargetDev,
                                LPCRECTL lprcBounds, LPCRECTL lprcWBounds, LPRECT lprcUpdate,
                                BOOL (CALLBACK * pfnContinue)(DWORD), DWORD dwContinue,
                                LONG lViewId)
{
   ITextServicesImpl *This = impl_from_ITextServices(iface);

   FIXME("%p: STUB\n", This);
   return E_NOTIMPL;
}

DECLSPEC_HIDDEN HRESULT WINAPI fnTextSrv_TxGetHScroll(ITextServices *iface, LONG *plMin, LONG *plMax, LONG *plPos,
                                      LONG *plPage, BOOL *pfEnabled)
{
   ITextServicesImpl *This = impl_from_ITextServices(iface);

   *plMin = This->editor->horz_si.nMin;
   *plMax = This->editor->horz_si.nMax;
   *plPos = This->editor->horz_si.nPos;
   *plPage = This->editor->horz_si.nPage;
   *pfEnabled = (This->editor->styleFlags & WS_HSCROLL) != 0;
   return S_OK;
}

DECLSPEC_HIDDEN HRESULT WINAPI fnTextSrv_TxGetVScroll(ITextServices *iface, LONG *plMin, LONG *plMax, LONG *plPos,
                                      LONG *plPage, BOOL *pfEnabled)
{
   ITextServicesImpl *This = impl_from_ITextServices(iface);

   *plMin = This->editor->vert_si.nMin;
   *plMax = This->editor->vert_si.nMax;
   *plPos = This->editor->vert_si.nPos;
   *plPage = This->editor->vert_si.nPage;
   *pfEnabled = (This->editor->styleFlags & WS_VSCROLL) != 0;
   return S_OK;
}

DECLSPEC_HIDDEN HRESULT WINAPI fnTextSrv_OnTxSetCursor(ITextServices *iface, DWORD dwDrawAspect, LONG lindex,
                                       void *pvAspect, DVTARGETDEVICE *ptd, HDC hdcDraw,
                                       HDC hicTargetDev, LPCRECT lprcClient, INT x, INT y)
{
   ITextServicesImpl *This = impl_from_ITextServices(iface);

   FIXME("%p: STUB\n", This);
   return E_NOTIMPL;
}

DECLSPEC_HIDDEN HRESULT WINAPI fnTextSrv_TxQueryHitPoint(ITextServices *iface, DWORD dwDrawAspect, LONG lindex,
                                         void *pvAspect, DVTARGETDEVICE *ptd, HDC hdcDraw,
                                         HDC hicTargetDev, LPCRECT lprcClient, INT x, INT y,
                                         DWORD *pHitResult)
{
   ITextServicesImpl *This = impl_from_ITextServices(iface);

   FIXME("%p: STUB\n", This);
   return E_NOTIMPL;
}

DECLSPEC_HIDDEN HRESULT WINAPI fnTextSrv_OnTxInplaceActivate(ITextServices *iface, LPCRECT prcClient)
{
   ITextServicesImpl *This = impl_from_ITextServices(iface);

   FIXME("%p: STUB\n", This);
   return E_NOTIMPL;
}

DECLSPEC_HIDDEN HRESULT WINAPI fnTextSrv_OnTxInplaceDeactivate(ITextServices *iface)
{
   ITextServicesImpl *This = impl_from_ITextServices(iface);

   FIXME("%p: STUB\n", This);
   return E_NOTIMPL;
}

DECLSPEC_HIDDEN HRESULT WINAPI fnTextSrv_OnTxUIActivate(ITextServices *iface)
{
   ITextServicesImpl *This = impl_from_ITextServices(iface);

   FIXME("%p: STUB\n", This);
   return E_NOTIMPL;
}

DECLSPEC_HIDDEN HRESULT WINAPI fnTextSrv_OnTxUIDeactivate(ITextServices *iface)
{
   ITextServicesImpl *This = impl_from_ITextServices(iface);

   FIXME("%p: STUB\n", This);
   return E_NOTIMPL;
}

DECLSPEC_HIDDEN HRESULT WINAPI fnTextSrv_TxGetText(ITextServices *iface, BSTR *pbstrText)
{
   ITextServicesImpl *This = impl_from_ITextServices(iface);
   int length;

   length = ME_GetTextLength(This->editor);
   if (length)
   {
      ME_Cursor start;
      BSTR bstr;
      bstr = SysAllocStringByteLen(NULL, length * sizeof(WCHAR));
      if (bstr == NULL)
         return E_OUTOFMEMORY;

      ME_CursorFromCharOfs(This->editor, 0, &start);
      ME_GetTextW(This->editor, bstr, length, &start, INT_MAX, FALSE, FALSE);
      *pbstrText = bstr;
   } else {
      *pbstrText = NULL;
   }

   return S_OK;
}

DECLSPEC_HIDDEN HRESULT WINAPI fnTextSrv_TxSetText(ITextServices *iface, LPCWSTR pszText)
{
   ITextServicesImpl *This = impl_from_ITextServices(iface);
   ME_Cursor cursor;

   ME_SetCursorToStart(This->editor, &cursor);
   ME_InternalDeleteText(This->editor, &cursor,
                         ME_GetTextLength(This->editor), FALSE);
   ME_InsertTextFromCursor(This->editor, 0, pszText, -1,
                           This->editor->pBuffer->pDefaultStyle);
   ME_SetSelection(This->editor, 0, 0);
   This->editor->nModifyStep = 0;
   OleFlushClipboard();
   ME_EmptyUndoStack(This->editor);
   ME_UpdateRepaint(This->editor, FALSE);

   return S_OK;
}

DECLSPEC_HIDDEN HRESULT WINAPI fnTextSrv_TxGetCurTargetX(ITextServices *iface, LONG *x)
{
   ITextServicesImpl *This = impl_from_ITextServices(iface);

   FIXME("%p: STUB\n", This);
   return E_NOTIMPL;
}

DECLSPEC_HIDDEN HRESULT WINAPI fnTextSrv_TxGetBaseLinePos(ITextServices *iface, LONG *x)
{
   ITextServicesImpl *This = impl_from_ITextServices(iface);

   FIXME("%p: STUB\n", This);
   return E_NOTIMPL;
}

DECLSPEC_HIDDEN HRESULT WINAPI fnTextSrv_TxGetNaturalSize(ITextServices *iface, DWORD dwAspect, HDC hdcDraw,
                                          HDC hicTargetDev, DVTARGETDEVICE *ptd, DWORD dwMode,
                                          const SIZEL *psizelExtent, LONG *pwidth, LONG *pheight)
{
   ITextServicesImpl *This = impl_from_ITextServices(iface);

   FIXME("%p: STUB\n", This);
   return E_NOTIMPL;
}

DECLSPEC_HIDDEN HRESULT WINAPI fnTextSrv_TxGetDropTarget(ITextServices *iface, IDropTarget **ppDropTarget)
{
   ITextServicesImpl *This = impl_from_ITextServices(iface);

   FIXME("%p: STUB\n", This);
   return E_NOTIMPL;
}

DECLSPEC_HIDDEN HRESULT WINAPI fnTextSrv_OnTxPropertyBitsChange(ITextServices *iface, DWORD dwMask, DWORD dwBits)
{
   ITextServicesImpl *This = impl_from_ITextServices(iface);

   FIXME("%p: STUB\n", This);
   return E_NOTIMPL;
}

DECLSPEC_HIDDEN HRESULT WINAPI fnTextSrv_TxGetCachedSize(ITextServices *iface, DWORD *pdwWidth, DWORD *pdwHeight)
{
   ITextServicesImpl *This = impl_from_ITextServices(iface);

   FIXME("%p: STUB\n", This);
   return E_NOTIMPL;
}

DEFINE_THISCALL_WRAPPER(fnTextSrv_TxSendMessage,20)
DEFINE_THISCALL_WRAPPER(fnTextSrv_TxDraw,52)
DEFINE_THISCALL_WRAPPER(fnTextSrv_TxGetHScroll,24)
DEFINE_THISCALL_WRAPPER(fnTextSrv_TxGetVScroll,24)
DEFINE_THISCALL_WRAPPER(fnTextSrv_OnTxSetCursor,40)
DEFINE_THISCALL_WRAPPER(fnTextSrv_TxQueryHitPoint,44)
DEFINE_THISCALL_WRAPPER(fnTextSrv_OnTxInplaceActivate,8)
DEFINE_THISCALL_WRAPPER(fnTextSrv_OnTxInplaceDeactivate,4)
DEFINE_THISCALL_WRAPPER(fnTextSrv_OnTxUIActivate,4)
DEFINE_THISCALL_WRAPPER(fnTextSrv_OnTxUIDeactivate,4)
DEFINE_THISCALL_WRAPPER(fnTextSrv_TxGetText,8)
DEFINE_THISCALL_WRAPPER(fnTextSrv_TxSetText,8)
DEFINE_THISCALL_WRAPPER(fnTextSrv_TxGetCurTargetX,8)
DEFINE_THISCALL_WRAPPER(fnTextSrv_TxGetBaseLinePos,8)
DEFINE_THISCALL_WRAPPER(fnTextSrv_TxGetNaturalSize,36)
DEFINE_THISCALL_WRAPPER(fnTextSrv_TxGetDropTarget,8)
DEFINE_THISCALL_WRAPPER(fnTextSrv_OnTxPropertyBitsChange,12)
DEFINE_THISCALL_WRAPPER(fnTextSrv_TxGetCachedSize,12)

static const ITextServicesVtbl textservices_vtbl =
{
   fnTextSrv_QueryInterface,
   fnTextSrv_AddRef,
   fnTextSrv_Release,
   THISCALL(fnTextSrv_TxSendMessage),
   THISCALL(fnTextSrv_TxDraw),
   THISCALL(fnTextSrv_TxGetHScroll),
   THISCALL(fnTextSrv_TxGetVScroll),
   THISCALL(fnTextSrv_OnTxSetCursor),
   THISCALL(fnTextSrv_TxQueryHitPoint),
   THISCALL(fnTextSrv_OnTxInplaceActivate),
   THISCALL(fnTextSrv_OnTxInplaceDeactivate),
   THISCALL(fnTextSrv_OnTxUIActivate),
   THISCALL(fnTextSrv_OnTxUIDeactivate),
   THISCALL(fnTextSrv_TxGetText),
   THISCALL(fnTextSrv_TxSetText),
   THISCALL(fnTextSrv_TxGetCurTargetX),
   THISCALL(fnTextSrv_TxGetBaseLinePos),
   THISCALL(fnTextSrv_TxGetNaturalSize),
   THISCALL(fnTextSrv_TxGetDropTarget),
   THISCALL(fnTextSrv_OnTxPropertyBitsChange),
   THISCALL(fnTextSrv_TxGetCachedSize)
};

/******************************************************************
 *        CreateTextServices (RICHED20.4)
 */
HRESULT WINAPI CreateTextServices(IUnknown  *pUnkOuter, ITextHost *pITextHost, IUnknown  **ppUnk)
{
   ITextServicesImpl *ITextImpl;

   TRACE("%p %p --> %p\n", pUnkOuter, pITextHost, ppUnk);
   if (pITextHost == NULL)
      return E_POINTER;

   ITextImpl = CoTaskMemAlloc(sizeof(*ITextImpl));
   if (ITextImpl == NULL)
      return E_OUTOFMEMORY;
   InitializeCriticalSection(&ITextImpl->csTxtSrv);
   ITextImpl->csTxtSrv.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": ITextServicesImpl.csTxtSrv");
   ITextImpl->ref = 1;
   ITextHost_AddRef(pITextHost);
   ITextImpl->pMyHost = pITextHost;
   ITextImpl->IUnknown_inner.lpVtbl = &textservices_inner_vtbl;
   ITextImpl->ITextServices_iface.lpVtbl = &textservices_vtbl;
   ITextImpl->editor = ME_MakeEditor(pITextHost, FALSE, ES_LEFT);
   ITextImpl->editor->exStyleFlags = 0;
   ITextImpl->editor->rcFormat.left = 0;
   ITextImpl->editor->rcFormat.top = 0;
   ITextImpl->editor->rcFormat.right = 0;
   ITextImpl->editor->rcFormat.bottom = 0;

   if (pUnkOuter)
      ITextImpl->outer_unk = pUnkOuter;
   else
      ITextImpl->outer_unk = &ITextImpl->IUnknown_inner;

   *ppUnk = &ITextImpl->IUnknown_inner;
   return S_OK;
}
