#ifndef __WIN32K_MSGQUEUE_H
#define __WIN32K_MSGQUEUE_H

#include <windows.h>

typedef struct _USER_MESSAGE
{
  LIST_ENTRY ListEntry;
  MSG Msg;
} USER_MESSAGE, *PUSER_MESSAGE;

struct _USER_MESSAGE_QUEUE;

typedef struct _USER_SENT_MESSAGE
{
  LIST_ENTRY ListEntry;
  MSG Msg;
  PKEVENT CompletionEvent;
  LRESULT* Result;
  struct _USER_MESSAGE_QUEUE* CompletionQueue;
  SENDASYNCPROC CompletionCallback;
  ULONG_PTR CompletionCallbackContext;
} USER_SENT_MESSAGE, *PUSER_SENT_MESSAGE;

typedef struct _USER_SENT_MESSAGE_NOTIFY
{
  SENDASYNCPROC CompletionCallback;
  ULONG_PTR CompletionCallbackContext;
  LRESULT Result;
  HWND hWnd;
  UINT Msg;
  LIST_ENTRY ListEntry;
} USER_SENT_MESSAGE_NOTIFY, *PUSER_SENT_MESSAGE_NOTIFY;

typedef struct _USER_MESSAGE_QUEUE
{
  /* Queue of messages sent to the queue. */
  LIST_ENTRY SentMessagesListHead;
  /* Queue of messages posted to the queue. */
  LIST_ENTRY PostedMessagesListHead;
  /* Queue of sent-message notifies for the queue. */
  LIST_ENTRY NotifyMessagesListHead;
  /* Queue for hardware messages for the queue. */
  LIST_ENTRY HardwareMessagesListHead;
  /* Lock for the queue. */
  FAST_MUTEX Lock;
  /* True if a WM_QUIT message is pending. */
  BOOLEAN QuitPosted;
  /* The quit exit code. */
  ULONG QuitExitCode;
  /* Set if there are new messages in any of the queues. */
  KEVENT NewMessages;  
  /* FIXME: Unknown. */
  ULONG QueueStatus;
  /* Current window with focus (ie. receives keyboard input) for this queue. */
  HWND FocusWindow;
  /* True if a window needs painting. */
  BOOLEAN PaintPosted;
  /* Count of paints pending. */
  ULONG PaintCount;
  /* Current active window for this queue. */
  HWND ActiveWindow;
  /* Current capture window for this queue. */
  HWND CaptureWindow;
} USER_MESSAGE_QUEUE, *PUSER_MESSAGE_QUEUE;

VOID FASTCALL
MsqSendMessage(PUSER_MESSAGE_QUEUE MessageQueue,
	       PUSER_SENT_MESSAGE Message);
VOID FASTCALL
MsqInitializeMessage(PUSER_MESSAGE Message,
		     LPMSG Msg);
PUSER_MESSAGE FASTCALL
MsqCreateMessage(LPMSG Msg);
VOID FASTCALL
MsqDestroyMessage(PUSER_MESSAGE Message);
VOID FASTCALL
MsqPostMessage(PUSER_MESSAGE_QUEUE MessageQueue,
	       PUSER_MESSAGE Message);
VOID FASTCALL
MsqPostQuitMessage(PUSER_MESSAGE_QUEUE MessageQueue, ULONG ExitCode);
BOOLEAN STDCALL
MsqFindMessage(IN PUSER_MESSAGE_QUEUE MessageQueue,
	       IN BOOLEAN Hardware,
	       IN BOOLEAN Remove,
	       IN HWND Wnd,
	       IN UINT MsgFilterLow,
	       IN UINT MsgFilterHigh,
	       OUT PUSER_MESSAGE* Message);
VOID FASTCALL
MsqInitializeMessageQueue(PUSER_MESSAGE_QUEUE MessageQueue);
VOID FASTCALL
MsqFreeMessageQueue(PUSER_MESSAGE_QUEUE MessageQueue);
PUSER_MESSAGE_QUEUE FASTCALL
MsqCreateMessageQueue(VOID);
VOID FASTCALL
MsqDestroyMessageQueue(PUSER_MESSAGE_QUEUE MessageQueue);
PUSER_MESSAGE_QUEUE FASTCALL
MsqGetHardwareMessageQueue(VOID);
NTSTATUS FASTCALL
MsqWaitForNewMessage(PUSER_MESSAGE_QUEUE MessageQueue);
NTSTATUS FASTCALL
MsqInitializeImpl(VOID);
BOOLEAN FASTCALL
MsqDispatchOneSentMessage(PUSER_MESSAGE_QUEUE MessageQueue);
NTSTATUS FASTCALL
MsqWaitForNewMessages(PUSER_MESSAGE_QUEUE MessageQueue);
VOID FASTCALL
MsqSendNotifyMessage(PUSER_MESSAGE_QUEUE MessageQueue,
		     PUSER_SENT_MESSAGE_NOTIFY NotifyMessage);
VOID FASTCALL
MsqIncPaintCountQueue(PUSER_MESSAGE_QUEUE Queue);
VOID FASTCALL
MsqDecPaintCountQueue(PUSER_MESSAGE_QUEUE Queue);
LRESULT STDCALL
W32kSendMessage(HWND hWnd,
		UINT Msg,
		WPARAM wParam,
		LPARAM lParam,
		BOOL KernelMessage);
VOID STDCALL
MsqPostKeyboardMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
VOID FASTCALL
MsqInsertSystemMessage(MSG* Msg);

#define MAKE_LONG(x, y) ((((y) & 0xFFFF) << 16) | ((x) & 0xFFFF))

#endif /* __WIN32K_MSGQUEUE_H */

/* EOF */
