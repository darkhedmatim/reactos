/* ------------- slidebox.c ------------ */

#include "dflat.h"

static int (*GenericProc)
    (DFWINDOW wnd,DFMESSAGE msg,PARAM p1,PARAM p2);
static BOOL KeepRunning;
static int SliderLen;
static int Percent;
extern DBOX SliderBoxDB;

static void InsertPercent(char *s)
{
    int offset;
    char pcc[5];

    sprintf(s, "%c%c%c",
            CHANGECOLOR,
            color[DIALOG][SELECT_COLOR][FG]+0x80,
            color[DIALOG][SELECT_COLOR][BG]+0x80);
    s += 3;
    memset(s, ' ', SliderLen);
    *(s+SliderLen) = '\0';
    sprintf(pcc, "%d%%", Percent);
    strncpy(s+SliderLen/2-1, pcc, strlen(pcc));
    offset = (SliderLen * Percent) / 100;
    memmove(s+offset+4, s+offset, strlen(s+offset)+1);
    sprintf(pcc, "%c%c%c%c",
            RESETCOLOR,
            CHANGECOLOR,
            color[DIALOG][SELECT_COLOR][BG]+0x80,
            color[DIALOG][SELECT_COLOR][FG]+0x80);
    strncpy(s+offset, pcc, 4);
    *(s + strlen(s) - 1) = RESETCOLOR;
}

static int
SliderTextProc (DFWINDOW wnd,DFMESSAGE msg,PARAM p1,PARAM p2)
{
    switch (msg)
    {
        case PAINT:
            Percent = (int)p2;
            InsertPercent(GetText(wnd) ?
                GetText(wnd) : SliderBoxDB.ctl[1].itext);
            GenericProc(wnd, PAINT, 0, 0);
            if (Percent >= 100)
                DfSendMessage(GetParent(wnd),DFM_COMMAND,ID_CANCEL,0);
            if (!DfDispatchMessage ())
                DfPostMessage(GetParent(wnd), ENDDIALOG, 0, 0);
            return KeepRunning;
        default:
            break;
    }
    return GenericProc(wnd, msg, p1, p2);
}

static int
SliderBoxProc (DFWINDOW wnd, DFMESSAGE msg, PARAM p1, PARAM p2)
{
    int rtn;
    DFWINDOW twnd;
    switch (msg)    {
        case CREATE_WINDOW:
            AddAttribute(wnd, SAVESELF);
            rtn = DefaultWndProc(wnd, msg, p1, p2);
            twnd = SliderBoxDB.ctl[1].wnd;
            GenericProc = twnd->wndproc;
            twnd->wndproc = SliderTextProc;
            KeepRunning = TRUE;
            DfSendMessage(wnd, CAPTURE_MOUSE, 0, 0);
            DfSendMessage(wnd, CAPTURE_KEYBOARD, 0, 0);
            return rtn;
        case DFM_COMMAND:
            if ((int)p2 == 0 && (int)p1 == ID_CANCEL)    {
                if (Percent >= 100 ||
                        DfYesNoBox("Terminate process?"))
                    KeepRunning = FALSE;
                else
                    return TRUE;
            }
            break;
        case CLOSE_WINDOW:
            DfSendMessage(wnd, RELEASE_MOUSE, 0, 0);
            DfSendMessage(wnd, RELEASE_KEYBOARD, 0, 0);
            break;
        default:
            break;
    }
    return DefaultWndProc(wnd, msg, p1, p2);
}

DFWINDOW SliderBox(int len, char *ttl, char *msg)
{
    SliderLen = len;
    SliderBoxDB.dwnd.title = ttl;
    SliderBoxDB.dwnd.w =
        max((int)strlen(ttl),max(len, (int)strlen(msg)))+4;
    SliderBoxDB.ctl[0].itext = msg;
    SliderBoxDB.ctl[0].dwnd.w = strlen(msg);
    SliderBoxDB.ctl[0].dwnd.x =
        (SliderBoxDB.dwnd.w - strlen(msg)-1) / 2;
    SliderBoxDB.ctl[1].itext =
        DFrealloc(SliderBoxDB.ctl[1].itext, len+10);
    Percent = 0;
    InsertPercent(SliderBoxDB.ctl[1].itext);
    SliderBoxDB.ctl[1].dwnd.w = len;
    SliderBoxDB.ctl[1].dwnd.x = (SliderBoxDB.dwnd.w-len-1)/2;
    SliderBoxDB.ctl[2].dwnd.x = (SliderBoxDB.dwnd.w-10)/2;
    DfDialogBox(NULL, &SliderBoxDB, FALSE, SliderBoxProc);
    return SliderBoxDB.ctl[1].wnd;
}

/* EOF */
