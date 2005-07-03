/*
 * ReactOS Access Control List Editor
 * Copyright (C) 2004 ReactOS Team
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
/* $Id$
 *
 * PROJECT:         ReactOS Access Control List Editor
 * FILE:            lib/aclui/checklist.c
 * PURPOSE:         Access Control List Editor
 * PROGRAMMER:      Thomas Weidenmueller <w3seek@reactos.com>
 *
 * UPDATE HISTORY:
 *      07/01/2005  Created
 */
#include "acluilib.h"

typedef struct _CHECKITEM
{
    struct _CHECKITEM *Next;
    DWORD State;
    WCHAR Name[1];
} CHECKITEM, *PCHECKITEM;

typedef struct _CHECKLISTWND
{
   HWND hSelf;
   HWND hNotify;
   HFONT hFont;
   
   PCHECKITEM CheckItemListHead;
   UINT CheckItemCount;
   
   INT ItemHeight;
   
   BOOL HasFocus;
   PCHECKITEM FocusedCheckItem;
   UINT FocusedCheckItemBox;
   
   COLORREF TextColor[2];
   UINT CheckBoxLeft[2];
} CHECKLISTWND, *PCHECKLISTWND;

#define CI_TEXT_MARGIN_WIDTH    (6)
#define CI_TEXT_MARGIN_HEIGHT   (2)

static PCHECKITEM
FindCheckItemByIndex(IN PCHECKLISTWND infoPtr,
                     IN UINT Index)
{
    PCHECKITEM Item, Found = NULL;
    
    if (Index >= 0)
    {
        for (Item = infoPtr->CheckItemListHead;
             Item != NULL;
             Item = Item->Next)
        {
            if (Index == 0)
            {
                Found = Item;
                break;
            }

            Index--;
        }
    }
    
    return Found;
}

static INT
CheckItemToIndex(IN PCHECKLISTWND infoPtr,
                 IN PCHECKITEM Item)
{
    PCHECKITEM CurItem;
    INT Index;
    
    for (CurItem = infoPtr->CheckItemListHead, Index = 0;
         CurItem != NULL;
         CurItem = CurItem->Next, Index++)
    {
        if (CurItem == Item)
        {
            return Index;
        }
    }
    
    return -1;
}

static PCHECKITEM
FindFirstEnabledCheckBox(IN PCHECKLISTWND infoPtr,
                         OUT UINT *CheckBox)
{
    PCHECKITEM CurItem;

    for (CurItem = infoPtr->CheckItemListHead;
         CurItem != NULL;
         CurItem = CurItem->Next)
    {
        if ((CurItem->State & CIS_DISABLED) != CIS_DISABLED)
        {
            /* return the Allow checkbox in case both check boxes are enabled! */
            *CheckBox = ((!(CurItem->State & CIS_ALLOWDISABLED)) ? CLB_ALLOW : CLB_DENY);
            return CurItem;
        }
    }
    
    return NULL;
}

static PCHECKITEM
FindLastEnabledCheckBox(IN PCHECKLISTWND infoPtr,
                        OUT UINT *CheckBox)
{
    PCHECKITEM CurItem;
    PCHECKITEM LastEnabledItem = NULL;

    for (CurItem = infoPtr->CheckItemListHead;
         CurItem != NULL;
         CurItem = CurItem->Next)
    {
        if ((CurItem->State & CIS_DISABLED) != CIS_DISABLED)
        {
            LastEnabledItem = CurItem;
        }
    }
    
    if (LastEnabledItem != NULL)
    {
        /* return the Deny checkbox in case both check boxes are enabled! */
        *CheckBox = ((!(LastEnabledItem->State & CIS_DENYDISABLED)) ? CLB_DENY : CLB_ALLOW);
        return LastEnabledItem;
    }

    return NULL;
}

static PCHECKITEM
FindPreviousEnabledCheckBox(IN PCHECKLISTWND infoPtr,
                            OUT UINT *CheckBox)
{
    PCHECKITEM Item;

    if (infoPtr->FocusedCheckItem != NULL)
    {
        Item = infoPtr->FocusedCheckItem;

        if (infoPtr->FocusedCheckItemBox == CLB_DENY &&
            !(Item->State & CIS_ALLOWDISABLED))
        {
            /* currently an Deny checkbox is focused. return the Allow checkbox
               if it's enabled */
            *CheckBox = CLB_ALLOW;
        }
        else
        {
            PCHECKITEM CurItem;

            Item = NULL;

            for (CurItem = infoPtr->CheckItemListHead;
                 CurItem != infoPtr->FocusedCheckItem;
                 CurItem = CurItem->Next)
            {
                if ((CurItem->State & CIS_DISABLED) != CIS_DISABLED)
                {
                    Item = CurItem;
                }
            }
            
            if (Item != NULL)
            {
                /* return the Deny checkbox in case both check boxes are enabled! */
                *CheckBox = ((!(Item->State & CIS_DENYDISABLED)) ? CLB_DENY : CLB_ALLOW);
            }
        }
    }
    else
    {
        Item = FindLastEnabledCheckBox(infoPtr,
                                       CheckBox);
    }

    return Item;
}

static PCHECKITEM
FindNextEnabledCheckBox(IN PCHECKLISTWND infoPtr,
                        OUT UINT *CheckBox)
{
    PCHECKITEM Item;
    
    if (infoPtr->FocusedCheckItem != NULL)
    {
        Item = infoPtr->FocusedCheckItem;
        
        if (infoPtr->FocusedCheckItemBox != CLB_DENY &&
            !(Item->State & CIS_DENYDISABLED))
        {
            /* currently an Allow checkbox is focused. return the Deny checkbox
               if it's enabled */
            *CheckBox = CLB_DENY;
        }
        else
        {
            Item = Item->Next;
            
            while (Item != NULL)
            {
                if ((Item->State & CIS_DISABLED) != CIS_DISABLED)
                {
                    /* return the Allow checkbox in case both check boxes are enabled! */
                    *CheckBox = ((!(Item->State & CIS_ALLOWDISABLED)) ? CLB_ALLOW : CLB_DENY);
                    break;
                }

                Item = Item->Next;
            }
        }
    }
    else
    {
        Item = FindFirstEnabledCheckBox(infoPtr,
                                        CheckBox);
    }
    
    return Item;
}

static PCHECKITEM
FindEnabledCheckBox(IN PCHECKLISTWND infoPtr,
                    IN BOOL ReverseSearch,
                    OUT UINT *CheckBox)
{
    PCHECKITEM Item;
    
    if (ReverseSearch)
    {
        Item = FindPreviousEnabledCheckBox(infoPtr,
                                           CheckBox);
    }
    else
    {
        Item = FindNextEnabledCheckBox(infoPtr,
                                       CheckBox);
    }
    
    return Item;
}

static PCHECKITEM
PtToCheckItemBox(IN PCHECKLISTWND infoPtr,
                 IN PPOINT ppt,
                 OUT UINT *CheckBox,
                 OUT BOOL *DirectlyInCheckBox)
{
    LONG Style;
    INT FirstVisible, Index;
    PCHECKITEM Item;
    
    Style = GetWindowLong(infoPtr->hSelf,
                          GWL_STYLE);
    
    if (Style & WS_VSCROLL)
    {
        FirstVisible = GetScrollPos(infoPtr->hSelf,
                                    SB_VERT);
    }
    else
    {
        FirstVisible = 0;
    }
    
    Index = FirstVisible + (ppt->y / infoPtr->ItemHeight);
    
    Item = FindCheckItemByIndex(infoPtr,
                                Index);
    if (Item != NULL)
    {
        INT cx;
        
        cx = infoPtr->CheckBoxLeft[CLB_ALLOW] +
             ((infoPtr->CheckBoxLeft[CLB_DENY] - infoPtr->CheckBoxLeft[CLB_ALLOW]) / 2);

        *CheckBox = ((ppt->x <= cx) ? CLB_ALLOW : CLB_DENY);
        
        if (DirectlyInCheckBox != NULL)
        {
            INT y = ppt->y % infoPtr->ItemHeight;
            
            if ((y >= CI_TEXT_MARGIN_HEIGHT &&
                 y <= infoPtr->ItemHeight - CI_TEXT_MARGIN_HEIGHT) &&

                (((ppt->x >= (infoPtr->CheckBoxLeft[CLB_ALLOW] - (infoPtr->ItemHeight / 2))) &&
                  (ppt->x <= (infoPtr->CheckBoxLeft[CLB_ALLOW] - (infoPtr->ItemHeight / 2) + infoPtr->ItemHeight)))
                 ||
                 ((ppt->x >= (infoPtr->CheckBoxLeft[CLB_DENY] - (infoPtr->ItemHeight / 2))) &&
                  (ppt->x <= (infoPtr->CheckBoxLeft[CLB_DENY] - (infoPtr->ItemHeight / 2) + infoPtr->ItemHeight)))))
            {
                *DirectlyInCheckBox = TRUE;
            }
            else
            {
                *DirectlyInCheckBox = FALSE;
            }
        }
    }
    
    return Item;
}

static VOID
ClearCheckItems(IN PCHECKLISTWND infoPtr)
{
    PCHECKITEM CurItem, NextItem;

    CurItem = infoPtr->CheckItemListHead;
    while (CurItem != NULL)
    {
        NextItem = CurItem->Next;
        HeapFree(GetProcessHeap(),
                 0,
                 CurItem);
        CurItem = NextItem;
    }

    infoPtr->CheckItemListHead = NULL;
    infoPtr->CheckItemCount = 0;
}

static BOOL
DeleteCheckItem(IN PCHECKLISTWND infoPtr,
                IN PCHECKITEM Item)
{
    PCHECKITEM CurItem;
    PCHECKITEM *PrevPtr = &infoPtr->CheckItemListHead;
    
    for (CurItem = infoPtr->CheckItemListHead;
         CurItem != NULL;
         CurItem = CurItem->Next)
    {
        if (CurItem == Item)
        {
            *PrevPtr = CurItem->Next;
            HeapFree(GetProcessHeap(),
                     0,
                     CurItem);
            infoPtr->CheckItemCount--;
            return TRUE;
        }
        
        PrevPtr = &CurItem->Next;
    }
    
    return FALSE;
}

static PCHECKITEM
AddCheckItem(IN PCHECKLISTWND infoPtr,
             IN LPWSTR Name,
             IN DWORD State,
             OUT INT *Index)
{
    PCHECKITEM CurItem;
    INT i;
    PCHECKITEM *PrevPtr = &infoPtr->CheckItemListHead;
    PCHECKITEM Item = HeapAlloc(GetProcessHeap(),
                                0,
                                sizeof(CHECKITEM) + (wcslen(Name) * sizeof(WCHAR)));
    if (Item != NULL)
    {
        for (CurItem = infoPtr->CheckItemListHead, i = 0;
             CurItem != NULL;
             CurItem = CurItem->Next)
        {
            PrevPtr = &CurItem->Next;
            i++;
        }
        
        Item->Next = NULL;
        Item->State = State & CIS_MASK;
        wcscpy(Item->Name,
               Name);
        
        *PrevPtr = Item;
        infoPtr->CheckItemCount++;
        
        if (Index != NULL)
        {
            *Index = i;
        }
    }
    
    return Item;
}

static UINT
ClearCheckBoxes(IN PCHECKLISTWND infoPtr)
{
    PCHECKITEM CurItem;
    UINT nUpdated = 0;
    
    for (CurItem = infoPtr->CheckItemListHead;
         CurItem != NULL;
         CurItem = CurItem->Next)
    {
        if (CurItem->State & (CIS_ALLOW | CIS_DENY))
        {
            CurItem->State &= ~(CIS_ALLOW | CIS_DENY);
            nUpdated++;
        }
    }
    
    return nUpdated;
}

static VOID
UpdateControl(IN PCHECKLISTWND infoPtr,
              IN BOOL AllowChangeStyle)
{
    RECT rcClient;
    SCROLLINFO ScrollInfo;
    LONG Style;
    
    GetClientRect(infoPtr->hSelf,
                  &rcClient);

    ScrollInfo.cbSize = sizeof(ScrollInfo);
    ScrollInfo.fMask = SIF_PAGE | SIF_RANGE;
    ScrollInfo.nMin = 0;
    ScrollInfo.nMax = infoPtr->CheckItemCount;
    ScrollInfo.nPage = ((rcClient.bottom - rcClient.top) + infoPtr->ItemHeight - 1) / infoPtr->ItemHeight;
    ScrollInfo.nPos = 0;
    ScrollInfo.nTrackPos = 0;

    if (AllowChangeStyle)
    {
        Style = GetWindowLong(infoPtr->hSelf,
                              GWL_STYLE);

        /* determine whether the vertical scrollbar has to be visible or not */
        if (ScrollInfo.nMax > ScrollInfo.nPage &&
            !(Style & WS_VSCROLL))
        {
            SetWindowLong(infoPtr->hSelf,
                          GWL_STYLE,
                          Style | WS_VSCROLL);
        }
        else if (ScrollInfo.nMax < ScrollInfo.nPage &&
                 Style & WS_VSCROLL)
        {
            SetWindowLong(infoPtr->hSelf,
                          GWL_STYLE,
                          Style & ~WS_VSCROLL);
        }
    }
    
    SetScrollInfo(infoPtr->hSelf,
                  SB_VERT,
                  &ScrollInfo,
                  TRUE);

    RedrawWindow(infoPtr->hSelf,
                 NULL,
                 NULL,
                 RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW | RDW_NOCHILDREN);
}

static VOID
UpdateCheckItem(IN PCHECKLISTWND infoPtr,
                IN PCHECKITEM Item)
{
    LONG Style;
    RECT rcClient;
    INT VisibleFirst, VisibleItems;
    INT Index = CheckItemToIndex(infoPtr,
                                 Item);
    if (Index != -1)
    {
        Style = GetWindowLong(infoPtr->hSelf,
                              GWL_STYLE);

        if (Style & WS_VSCROLL)
        {
            VisibleFirst = GetScrollPos(infoPtr->hSelf,
                                        SB_VERT);
        }
        else
        {
            VisibleFirst = 0;
        }
        
        if (Index >= VisibleFirst)
        {
            GetClientRect(infoPtr->hSelf,
                          &rcClient);

            VisibleItems = ((rcClient.bottom - rcClient.top) + infoPtr->ItemHeight - 1) / infoPtr->ItemHeight;
            
            if (Index < VisibleFirst + VisibleItems)
            {
                RECT rcUpdate;
                
                rcUpdate.left = rcClient.left;
                rcUpdate.right = rcClient.right;
                rcUpdate.top = (Index - VisibleFirst) * infoPtr->ItemHeight;
                rcUpdate.bottom = rcUpdate.top + infoPtr->ItemHeight;

                RedrawWindow(infoPtr->hSelf,
                             &rcUpdate,
                             NULL,
                             RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW | RDW_NOCHILDREN);
            }
        }
    }
}

static VOID
MakeCheckItemVisible(IN PCHECKLISTWND infoPtr,
                     IN PCHECKITEM Item)
{
    LONG Style;
    RECT rcClient;
    INT VisibleFirst, VisibleItems, NewPos;
    INT Index = CheckItemToIndex(infoPtr,
                                 Item);
    if (Index != -1)
    {
        Style = GetWindowLong(infoPtr->hSelf,
                              GWL_STYLE);

        if (Style & WS_VSCROLL)
        {
            VisibleFirst = GetScrollPos(infoPtr->hSelf,
                                        SB_VERT);
        
            if (Index <= VisibleFirst)
            {
                NewPos = Index;
            }
            else
            {
                GetClientRect(infoPtr->hSelf,
                              &rcClient);

                VisibleItems = (rcClient.bottom - rcClient.top) / infoPtr->ItemHeight;
                if (Index - VisibleItems + 1 > VisibleFirst)
                {
                    NewPos = Index - VisibleItems + 1;
                }
                else
                {
                    NewPos = VisibleFirst;
                }
            }
            
            if (VisibleFirst != NewPos)
            {
                SCROLLINFO ScrollInfo;
                
                ScrollInfo.cbSize = sizeof(ScrollInfo);
                ScrollInfo.fMask = SIF_POS;
                ScrollInfo.nPos = NewPos;
                NewPos = SetScrollInfo(infoPtr->hSelf,
                                       SB_VERT,
                                       &ScrollInfo,
                                       TRUE);

                if (VisibleFirst != NewPos)
                {
                    ScrollWindowEx(infoPtr->hSelf,
                                   0,
                                   (NewPos - VisibleFirst) * infoPtr->ItemHeight,
                                   NULL,
                                   NULL,
                                   NULL,
                                   NULL,
                                   SW_INVALIDATE);

                    RedrawWindow(infoPtr->hSelf,
                                 NULL,
                                 NULL,
                                 RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW | RDW_NOCHILDREN);
                }
            }
        }
    }
}

static UINT
GetIdealItemHeight(IN PCHECKLISTWND infoPtr)
{
    HDC hdc = GetDC(infoPtr->hSelf);
    if(hdc != NULL)
    {
        UINT height;
        TEXTMETRIC tm;
        HGDIOBJ hOldFont = SelectObject(hdc,
                                        infoPtr->hFont);

        if(GetTextMetrics(hdc,
                          &tm))
        {
            height = tm.tmHeight;
        }
        else
        {
            height = 0;
        }

        SelectObject(hdc,
                     hOldFont);

        ReleaseDC(infoPtr->hSelf,
                  hdc);

        return height;
    }
    return 0;
}

static HFONT
RetChangeControlFont(IN PCHECKLISTWND infoPtr,
                     IN HFONT hFont,
                     IN BOOL Redraw)
{
    HFONT hOldFont = infoPtr->hFont;
    infoPtr->hFont = hFont;
    
    if (hOldFont != hFont)
    {
        infoPtr->ItemHeight = 4 + GetIdealItemHeight(infoPtr);
    }

    UpdateControl(infoPtr,
                  TRUE);

    return hOldFont;
}

static VOID
PaintControl(IN PCHECKLISTWND infoPtr,
             IN HDC hDC,
             IN PRECT rcUpdate)
{
    INT ScrollPos;
    PCHECKITEM FirstItem, Item;
    RECT rcClient;
    LONG Style;
    UINT VisibleFirstIndex = rcUpdate->top / infoPtr->ItemHeight;
    UINT LastTouchedIndex = rcUpdate->bottom / infoPtr->ItemHeight;
    
    FillRect(hDC,
             rcUpdate,
             (HBRUSH)(COLOR_WINDOW + 1));
    
    GetClientRect(infoPtr->hSelf, &rcClient);
    
    Style = GetWindowLong(infoPtr->hSelf,
                          GWL_STYLE);
    
    if (Style & WS_VSCROLL)
    {
        ScrollPos = GetScrollPos(infoPtr->hSelf,
                                 SB_VERT);
    }
    else
    {
        ScrollPos = 0;
    }
    
    FirstItem = FindCheckItemByIndex(infoPtr,
                                     ScrollPos + VisibleFirstIndex);
    if (FirstItem != NULL)
    {
        RECT TextRect, ItemRect, CheckBox;
        HFONT hOldFont;
        DWORD CurrentIndex;
        COLORREF OldTextColor;
        BOOL Enabled, PrevEnabled;
        POINT hOldBrushOrg;
        
        Enabled = IsWindowEnabled(infoPtr->hSelf);
        PrevEnabled = Enabled;
        
        ItemRect.left = 0;
        ItemRect.right = rcClient.right;
        ItemRect.top = VisibleFirstIndex * infoPtr->ItemHeight;
        
        TextRect.left = ItemRect.left + CI_TEXT_MARGIN_WIDTH;
        TextRect.right = ItemRect.right - CI_TEXT_MARGIN_WIDTH;
        TextRect.top = ItemRect.top + CI_TEXT_MARGIN_HEIGHT;
        
        SetBrushOrgEx(hDC,
                      ItemRect.left,
                      ItemRect.top,
                      &hOldBrushOrg);
        
        OldTextColor = SetTextColor(hDC,
                                    infoPtr->TextColor[Enabled]);

        hOldFont = SelectObject(hDC,
                                infoPtr->hFont);
        
        for (Item = FirstItem, CurrentIndex = VisibleFirstIndex;
             Item != NULL && CurrentIndex <= LastTouchedIndex;
             Item = Item->Next, CurrentIndex++)
        {
            TextRect.bottom = TextRect.top + infoPtr->ItemHeight;
            ItemRect.bottom = ItemRect.top + infoPtr->ItemHeight;
            
            SetBrushOrgEx(hDC,
                          ItemRect.left,
                          ItemRect.top,
                          NULL);
            
            if (Enabled && PrevEnabled != ((Item->State & CIS_DISABLED) != CIS_DISABLED))
            {
                PrevEnabled = ((Item->State & CIS_DISABLED) != CIS_DISABLED);
                
                SetTextColor(hDC,
                             infoPtr->TextColor[PrevEnabled]);
            }
            
            /* draw the text */
            DrawText(hDC,
                     Item->Name,
                     -1,
                     &TextRect,
                     DT_LEFT | DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER);
            
            /* draw the Allow checkbox */
            CheckBox.left = infoPtr->CheckBoxLeft[CLB_ALLOW] - ((TextRect.bottom - TextRect.top) / 2);
            CheckBox.right = CheckBox.left + (TextRect.bottom - TextRect.top) - (2 * CI_TEXT_MARGIN_HEIGHT);
            CheckBox.top = TextRect.top;
            CheckBox.bottom = CheckBox.top + (TextRect.bottom - TextRect.top) - (2 * CI_TEXT_MARGIN_HEIGHT);
            DrawFrameControl(hDC,
                             &CheckBox,
                             DFC_BUTTON,
                             DFCS_BUTTONCHECK | DFCS_FLAT |
                             ((Item->State & CIS_ALLOWDISABLED) || !Enabled ? DFCS_INACTIVE : 0) |
                             ((Item->State & CIS_ALLOW) ? DFCS_CHECKED : 0));
            if (infoPtr->HasFocus &&
                Item == infoPtr->FocusedCheckItem &&
                infoPtr->FocusedCheckItemBox != CLB_DENY)
            {
                RECT rcFocus = CheckBox;
                
                InflateRect (&rcFocus,
                             CI_TEXT_MARGIN_HEIGHT,
                             CI_TEXT_MARGIN_HEIGHT);

                DrawFocusRect(hDC,
                              &rcFocus);
            }

            /* draw the Deny checkbox */
            CheckBox.left = infoPtr->CheckBoxLeft[CLB_DENY] - ((TextRect.bottom - TextRect.top) / 2);
            CheckBox.right = CheckBox.left + (TextRect.bottom - TextRect.top) - (2 * CI_TEXT_MARGIN_HEIGHT);
            DrawFrameControl(hDC,
                             &CheckBox,
                             DFC_BUTTON,
                             DFCS_BUTTONCHECK | DFCS_FLAT |
                             ((Item->State & CIS_DENYDISABLED) || !Enabled ? DFCS_INACTIVE : 0) |
                             ((Item->State & CIS_DENY) ? DFCS_CHECKED : 0));
            if (infoPtr->HasFocus &&
                Item == infoPtr->FocusedCheckItem &&
                infoPtr->FocusedCheckItemBox == CLB_DENY)
            {
                RECT rcFocus = CheckBox;

                InflateRect (&rcFocus,
                             CI_TEXT_MARGIN_HEIGHT,
                             CI_TEXT_MARGIN_HEIGHT);

                DrawFocusRect(hDC,
                              &rcFocus);
            }

            TextRect.top += infoPtr->ItemHeight;
            ItemRect.top += infoPtr->ItemHeight;
        }

        SelectObject(hDC,
                     hOldFont);

        SetTextColor(hDC,
                     OldTextColor);

        SetBrushOrgEx(hDC,
                      hOldBrushOrg.x,
                      hOldBrushOrg.y,
                      NULL);
    }
}

static VOID
ChangeCheckItemFocus(IN PCHECKLISTWND infoPtr,
                     IN PCHECKITEM NewFocus,
                     IN INT NewFocusBox)
{
    if (NewFocus != infoPtr->FocusedCheckItem)
    {
        PCHECKITEM OldFocus = infoPtr->FocusedCheckItem;
        infoPtr->FocusedCheckItem = NewFocus;
        infoPtr->FocusedCheckItemBox = NewFocusBox;

        if (OldFocus != NULL)
        {
            UpdateCheckItem(infoPtr,
                            OldFocus);
        }
    }
    else
    {
        infoPtr->FocusedCheckItemBox = NewFocusBox;
    }

    if (NewFocus != NULL)
    {
        MakeCheckItemVisible(infoPtr,
                             NewFocus);
        UpdateCheckItem(infoPtr,
                        NewFocus);
    }
}

static LRESULT CALLBACK
CheckListWndProc(IN HWND hwnd,
                 IN UINT uMsg,
                 IN WPARAM wParam,
                 IN LPARAM lParam)
{
    PCHECKLISTWND infoPtr;
    LRESULT Ret;
    
    infoPtr = (PCHECKLISTWND)GetWindowLongPtr(hwnd,
                                              0);
    
    if (infoPtr == NULL && uMsg != WM_CREATE)
    {
        return DefWindowProc(hwnd,
                             uMsg,
                             wParam,
                             lParam);
    }
    
    Ret = 0;
    
    switch (uMsg)
    {
        case WM_PAINT:
        {
            HDC hdc;
            RECT rcUpdate;
            PAINTSTRUCT ps;
            
            if (GetUpdateRect(hwnd,
                              &rcUpdate,
                              FALSE))
            {
                hdc = (wParam != 0 ? (HDC)wParam : BeginPaint(hwnd, &ps));

                PaintControl(infoPtr,
                             hdc,
                             &rcUpdate);

                if (wParam == 0)
                {
                    EndPaint(hwnd,
                             &ps);
                }
            }
            break;
        }
        
        case WM_VSCROLL:
        {
            SCROLLINFO ScrollInfo;
            
            ScrollInfo.cbSize = sizeof(ScrollInfo);
            ScrollInfo.fMask = SIF_RANGE | SIF_POS;

            if (GetScrollInfo(hwnd,
                              SB_VERT,
                              &ScrollInfo))
            {
                INT OldPos = ScrollInfo.nPos;
                
                switch (LOWORD(wParam))
                {
                    case SB_BOTTOM:
                        ScrollInfo.nPos = ScrollInfo.nMax;
                        break;

                    case SB_LINEDOWN:
                        if (ScrollInfo.nPos < ScrollInfo.nMax)
                        {
                            ScrollInfo.nPos++;
                        }
                        break;

                    case SB_LINEUP:
                        if (ScrollInfo.nPos > 0)
                        {
                            ScrollInfo.nPos--;
                        }
                        break;

                    case SB_PAGEDOWN:
                    {
                        RECT rcClient;
                        INT ScrollLines;
                        
                        /* don't use ScrollInfo.nPage because we should only scroll
                           down by the number of completely visible list entries.
                           nPage however also includes the partly cropped list
                           item at the bottom of the control */

                        GetClientRect(hwnd, &rcClient);
                        ScrollLines = max(1, (rcClient.bottom - rcClient.top) / infoPtr->ItemHeight);
                        
                        if (ScrollInfo.nPos + ScrollLines <= ScrollInfo.nMax)
                        {
                            ScrollInfo.nPos += ScrollLines;
                        }
                        else
                        {
                            ScrollInfo.nPos = ScrollInfo.nMax;
                        }
                        break;
                    }

                    case SB_PAGEUP:
                    {
                        RECT rcClient;
                        INT ScrollLines;

                        /* don't use ScrollInfo.nPage because we should only scroll
                           down by the number of completely visible list entries.
                           nPage however also includes the partly cropped list
                           item at the bottom of the control */

                        GetClientRect(hwnd, &rcClient);
                        ScrollLines = max(1, (rcClient.bottom - rcClient.top) / infoPtr->ItemHeight);
                        
                        if (ScrollInfo.nPos >= ScrollLines)
                        {
                            ScrollInfo.nPos -= ScrollLines;
                        }
                        else
                        {
                            ScrollInfo.nPos = 0;
                        }
                        break;
                    }

                    case SB_THUMBPOSITION:
                    case SB_THUMBTRACK:
                    {
                        ScrollInfo.nPos = HIWORD(wParam);
                        break;
                    }

                    case SB_TOP:
                        ScrollInfo.nPos = 0;
                        break;
                }
                
                if (OldPos != ScrollInfo.nPos)
                {
                    ScrollInfo.fMask = SIF_POS;
                    
                    ScrollInfo.nPos = SetScrollInfo(hwnd,
                                                    SB_VERT,
                                                    &ScrollInfo,
                                                    TRUE);
                    
                    if (OldPos != ScrollInfo.nPos)
                    {
                        ScrollWindowEx(hwnd,
                                       0,
                                       (OldPos - ScrollInfo.nPos) * infoPtr->ItemHeight,
                                       NULL,
                                       NULL,
                                       NULL,
                                       NULL,
                                       SW_INVALIDATE);

                        RedrawWindow(hwnd,
                                     NULL,
                                     NULL,
                                     RDW_ERASE | RDW_UPDATENOW | RDW_NOCHILDREN);
                    }
                }
            }
            break;
        }
        
        case CLM_ADDITEM:
        {
            INT Index = -1;
            PCHECKITEM Item = AddCheckItem(infoPtr,
                                           (LPWSTR)lParam,
                                           (DWORD)wParam,
                                           &Index);
            if (Item != NULL)
            {
                UpdateControl(infoPtr,
                              TRUE);
                Ret = (LRESULT)Index;
            }
            else
            {
                Ret = (LRESULT)-1;
            }
            break;
        }
        
        case CLM_DELITEM:
        {
            PCHECKITEM Item = FindCheckItemByIndex(infoPtr,
                                                   wParam);
            if (Item != NULL)
            {
                Ret = DeleteCheckItem(infoPtr,
                                      Item);
                if (Ret)
                {
                    UpdateControl(infoPtr,
                                  TRUE);
                }
            }
            else
            {
                Ret = FALSE;
            }
            break;
        }
        
        case CLM_SETITEMSTATE:
        {
            PCHECKITEM Item = FindCheckItemByIndex(infoPtr,
                                                   wParam);
            if (Item != NULL)
            {
                DWORD OldState = Item->State;
                Item->State = (DWORD)lParam & CIS_MASK;
                
                if (Item->State != OldState)
                {
                    /* revert the focus if the currently focused item is about
                       to be disabled */
                    if (Item == infoPtr->FocusedCheckItem &&
                        (Item->State & CIS_DISABLED))
                    {
                        if (infoPtr->FocusedCheckItemBox == CLB_DENY)
                        {
                            if (Item->State & CIS_DENYDISABLED)
                            {
                                infoPtr->FocusedCheckItem = NULL;
                            }
                        }
                        else
                        {
                            if (Item->State & CIS_ALLOWDISABLED)
                            {
                                infoPtr->FocusedCheckItem = NULL;
                            }
                        }
                    }

                    UpdateControl(infoPtr,
                                  TRUE);
                }
                Ret = TRUE;
            }
            else
            {
                Ret = FALSE;
            }
            break;
        }
        
        case CLM_GETITEMCOUNT:
        {
            Ret = infoPtr->CheckItemCount;
            break;
        }
        
        case CLM_CLEAR:
        {
            ClearCheckItems(infoPtr);
            UpdateControl(infoPtr,
                          TRUE);
            break;
        }
        
        case CLM_SETCHECKBOXCOLUMN:
        {
            infoPtr->CheckBoxLeft[wParam != CLB_DENY] = (UINT)lParam;
            Ret = 1;
            break;
        }
        
        case CLM_GETCHECKBOXCOLUMN:
        {
            Ret = (LRESULT)infoPtr->CheckBoxLeft[wParam != CLB_DENY];
            break;
        }
        
        case CLM_CLEARCHECKBOXES:
        {
            Ret = (LRESULT)ClearCheckBoxes(infoPtr);
            if (Ret)
            {
                UpdateControl(infoPtr,
                              TRUE);
            }
            break;
        }
        
        case WM_SETFONT:
        {
            Ret = (LRESULT)RetChangeControlFont(infoPtr,
                                                (HFONT)wParam,
                                                (BOOL)lParam);
            break;
        }
        
        case WM_GETFONT:
        {
            Ret = (LRESULT)infoPtr->hFont;
            break;
        }
        
        case WM_STYLECHANGED:
        {
            LPSTYLESTRUCT Style = (LPSTYLESTRUCT)lParam;
            
            if (wParam == GWL_STYLE)
            {
                BOOL AllowChangeStyle;

                /* don't allow the control to enable/disable the vertical scrollbar
                   if this message was invoked due to such a window style change! */
                AllowChangeStyle = ((Style->styleNew & WS_VSCROLL) == (Style->styleOld & WS_VSCROLL));

                UpdateControl(infoPtr,
                              AllowChangeStyle);
            }
            break;
        }
        
        case WM_ENABLE:
        {
            UpdateControl(infoPtr,
                          TRUE);
            break;
        }
        
        case WM_MOUSEWHEEL:
        {
            SHORT ScrollDelta;
            UINT ScrollLines = 3;
            
            SystemParametersInfo(SPI_GETWHEELSCROLLLINES,
                                 0,
                                 &ScrollLines,
                                 0);
            ScrollDelta = 0 - (SHORT)HIWORD(wParam);
            
            if (ScrollLines != 0 &&
                abs(ScrollDelta) >= WHEEL_DELTA)
            {
                SCROLLINFO ScrollInfo;

                ScrollInfo.cbSize = sizeof(ScrollInfo);
                ScrollInfo.fMask = SIF_RANGE | SIF_POS;
                
                if (GetScrollInfo(hwnd,
                                  SB_VERT,
                                  &ScrollInfo))
                {
                    INT OldPos = ScrollInfo.nPos;
                    
                    ScrollInfo.nPos += (ScrollDelta / WHEEL_DELTA) * ScrollLines;
                    if (ScrollInfo.nPos < 0)
                        ScrollInfo.nPos = 0;
                    else if (ScrollInfo.nPos > ScrollInfo.nMax)
                        ScrollInfo.nPos = ScrollInfo.nMax;

                    if (OldPos != ScrollInfo.nPos)
                    {
                        ScrollInfo.fMask = SIF_POS;
                        
                        ScrollInfo.nPos = SetScrollInfo(hwnd,
                                                        SB_VERT,
                                                        &ScrollInfo,
                                                        TRUE);
                        
                        if (OldPos != ScrollInfo.nPos)
                        {
                            ScrollWindowEx(hwnd,
                                           0,
                                           (OldPos - ScrollInfo.nPos) * infoPtr->ItemHeight,
                                           NULL,
                                           NULL,
                                           NULL,
                                           NULL,
                                           SW_INVALIDATE);

                            RedrawWindow(hwnd,
                                         NULL,
                                         NULL,
                                         RDW_ERASE | RDW_UPDATENOW | RDW_NOCHILDREN);
                        }
                    }
                }
            }
            break;
        }
        
        case WM_SETFOCUS:
        {
            infoPtr->HasFocus = TRUE;

            if (infoPtr->FocusedCheckItem == NULL)
            {
                BOOL Shift = GetKeyState(VK_SHIFT) & 0x8000;
                infoPtr->FocusedCheckItem = FindEnabledCheckBox(infoPtr,
                                                                Shift,
                                                                &infoPtr->FocusedCheckItemBox);
            }
            if (infoPtr->FocusedCheckItem != NULL)
            {
                MakeCheckItemVisible(infoPtr,
                                     infoPtr->FocusedCheckItem);

                UpdateCheckItem(infoPtr,
                                infoPtr->FocusedCheckItem);
            }
            break;
        }
        
        case WM_KILLFOCUS:
        {
            infoPtr->HasFocus = FALSE;
            if (infoPtr->FocusedCheckItem != NULL)
            {
                UpdateCheckItem(infoPtr,
                                infoPtr->FocusedCheckItem);
            }
            break;
        }
        
        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN:
        {
            if (IsWindowEnabled(hwnd))
            {
                PCHECKITEM NewFocus;
                INT NewFocusBox = 0;
                BOOL InCheckBox;
                POINT pt;
                BOOL ChangeFocus;
                
                pt.x = (LONG)LOWORD(lParam);
                pt.y = (LONG)HIWORD(lParam);
                
                NewFocus = PtToCheckItemBox(infoPtr,
                                            &pt,
                                            &NewFocusBox,
                                            &InCheckBox);
                if (NewFocus != NULL)
                {
                    if (NewFocus->State & ((NewFocusBox != CLB_DENY) ? CIS_ALLOWDISABLED : CIS_DENYDISABLED))
                    {
                        /* the user clicked on a disabled checkbox, try to set
                           the focus to the other one or not change it at all */

                        InCheckBox = FALSE;
                        
                        ChangeFocus = ((NewFocus->State & CIS_DISABLED) != CIS_DISABLED);
                        if (ChangeFocus)
                        {
                            NewFocusBox = ((NewFocusBox != CLB_DENY) ? CLB_DENY : CLB_ALLOW);
                        }
                    }
                    else
                    {
                        ChangeFocus = TRUE;
                    }
                }
                else
                {
                    ChangeFocus = TRUE;
                }
                
                if (ChangeFocus)
                {
                    ChangeCheckItemFocus(infoPtr,
                                         NewFocus,
                                         NewFocusBox);
                }
                
                if (!infoPtr->HasFocus)
                {
                    SetFocus(hwnd);
                }
            }
            break;
        }
        
        case WM_KEYDOWN:
        {
            switch (wParam)
            {
                case VK_RETURN:
                {
                    /* FIXME */
                    break;
                }
                
                case VK_TAB:
                {
                    PCHECKITEM NewFocus;
                    UINT NewFocusBox = 0;
                    BOOL Shift = GetKeyState(VK_SHIFT) & 0x8000;
                    
                    NewFocus = FindEnabledCheckBox(infoPtr,
                                                   Shift,
                                                   &NewFocusBox);

                    ChangeCheckItemFocus(infoPtr,
                                         NewFocus,
                                         NewFocusBox);
                    break;
                }
                
                default:
                {
                    Ret = DefWindowProc(hwnd,
                                        uMsg,
                                        wParam,
                                        lParam);
                    break;
                }
            }
            break;
        }
        
        case WM_GETDLGCODE:
        {
            INT virtKey;
            
            Ret = DLGC_HASSETSEL;
            virtKey = (lParam != 0 ? (INT)((LPMSG)lParam)->wParam : 0);
            switch (virtKey)
            {
                case VK_RETURN:
                {
                    Ret |= DLGC_WANTMESSAGE;
                    break;
                }
                
                case VK_TAB:
                {
                    INT CheckBox;
                    BOOL EnabledBox;
                    BOOL Shift = GetKeyState(VK_SHIFT) & 0x8000;
                    
                    EnabledBox = FindEnabledCheckBox(infoPtr,
                                                     Shift,
                                                     &CheckBox) != NULL;
                    Ret |= (EnabledBox ? DLGC_WANTTAB : DLGC_WANTCHARS);
                    break;
                }
            }
            break;
        }
        
        case WM_SYSCOLORCHANGE:
        {
            infoPtr->TextColor[0] = GetSysColor(COLOR_GRAYTEXT);
            infoPtr->TextColor[1] = GetSysColor(COLOR_WINDOWTEXT);
            break;
        }
        
        case WM_CREATE:
        {
            infoPtr = HeapAlloc(GetProcessHeap(),
                                0,
                                sizeof(CHECKLISTWND));
            if (infoPtr != NULL)
            {
                RECT rcClient;
                
                infoPtr->hSelf = hwnd;
                infoPtr->hNotify = ((LPCREATESTRUCTW)lParam)->hwndParent;
                
                SetWindowLongPtr(hwnd,
                                 0,
                                 (DWORD_PTR)infoPtr);

                infoPtr->CheckItemListHead = NULL;
                infoPtr->CheckItemCount = 0;
                
                infoPtr->ItemHeight = 10;
                
                infoPtr->HasFocus = FALSE;
                infoPtr->FocusedCheckItem = NULL;
                infoPtr->FocusedCheckItemBox = 0;
                
                infoPtr->TextColor[0] = GetSysColor(COLOR_GRAYTEXT);
                infoPtr->TextColor[1] = GetSysColor(COLOR_WINDOWTEXT);
                
                GetClientRect(hwnd, &rcClient);
                
                infoPtr->CheckBoxLeft[0] = rcClient.right - 30;
                infoPtr->CheckBoxLeft[1] = rcClient.right - 15;
            }
            else
            {
                Ret = -1;
            }
            break;
        }
        
        case WM_DESTROY:
        {
            ClearCheckItems(infoPtr);
            
            HeapFree(GetProcessHeap(),
                     0,
                     infoPtr);
            SetWindowLongPtr(hwnd,
                             0,
                             (DWORD_PTR)NULL);
            break;
        }
        
        default:
        {
            Ret = DefWindowProc(hwnd,
                                uMsg,
                                wParam,
                                lParam);
            break;
        }
    }
    
    return Ret;
}

BOOL
RegisterCheckListControl(HINSTANCE hInstance)
{
    WNDCLASS wc;
    
    ZeroMemory(&wc, sizeof(WNDCLASS));
    
    wc.style = 0;
    wc.lpfnWndProc = CheckListWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = sizeof(PCHECKLISTWND);
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(0, (LPWSTR)IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"CHECKLIST_ACLUI";
    
    return RegisterClass(&wc) != 0;
}

VOID
UnregisterCheckListControl(VOID)
{
    UnregisterClass(L"CHECKLIST_ACLUI",
                    NULL);
}
