/* $Id: Console.cpp,v 1.1 2000/10/04 21:04:30 ea Exp $
 *
 * regexpl - Console Registry Explorer
 *
 * Copyright (c) 1999-2000 Nedko Arnaoudov <nedkohome@atia.com>
 *
 * License: GNU GPL
 *
 */

// Console.cpp: implementation of the CConsole class.
//
//////////////////////////////////////////////////////////////////////

#include "ph.h"
#include "Console.h"
/*
TCHAR * _tcsnchr(const TCHAR *string, TCHAR ch, int count)
{
	while (count--)
	{
		if (*string == 0) return NULL;
		if (*string == ch) return const_cast <char *>(string);
		string++;
	}
	return NULL;
}*/


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CConsole::CConsole()
{
	m_hStdIn = INVALID_HANDLE_VALUE;
	m_hStdOut = INVALID_HANDLE_VALUE;
	m_blnInsetMode = TRUE;		// Insert
//	m_blnInsetMode = FALSE;		// Overwrite
	m_dwInsertModeCursorHeight = 15;
	m_dwOverwriteModeCursorHeight = 100;
//	m_Lines = 0;
	m_pchBuffer = NULL;
	m_pchBuffer1 = NULL;
	m_pchBuffer2 = NULL;
	m_pfReplaceCompletionCallback = NULL;
	m_blnMoreMode = TRUE;
}

CConsole::~CConsole()
{
	if (m_hStdIn != INVALID_HANDLE_VALUE)
		VERIFY(CloseHandle(m_hStdIn));
	if (m_hStdOut != INVALID_HANDLE_VALUE)
		VERIFY(CloseHandle(m_hStdOut));
	if (m_pchBuffer)
		delete m_pchBuffer;
	if (m_pchBuffer1)
		delete m_pchBuffer1;
	if (m_pchBuffer2)
		delete m_pchBuffer2;
}

BOOL CConsole::Write(const TCHAR *p, DWORD dwChars)
{
	if (m_hStdOut == INVALID_HANDLE_VALUE)
		return FALSE;
	if (m_hStdIn == INVALID_HANDLE_VALUE)
		return FALSE;
	if (p == NULL)
	{
		ASSERT(FALSE);
		return FALSE;
	}
	DWORD dwCharsToWrite = (dwChars)?dwChars:_tcslen(p);
	DWORD dwCharsWrittenAdd = 0;
	BOOL ret = TRUE;
	while (dwCharsToWrite && (!m_blnDisableWrite))
	{
		switch(p[dwCharsWrittenAdd])
		{
		case _T('\n'):
			m_CursorPosition.Y++;
			m_CursorPosition.X = 0;
			break;
		case _T('\r'):
			break;
		case _T('\t'):
			do
			{
				if (!Write(_T(" "))) return FALSE;
			}
			while ((m_CursorPosition.X % 8) && (!m_blnDisableWrite));
			dwCharsWrittenAdd++;
			dwCharsToWrite--;
			continue;
		default:
			{
				if (!WriteChar(p[dwCharsWrittenAdd])) return FALSE;
				m_CursorPosition.X++;
			}
		}
		if (m_CursorPosition.X == m_BufferSize.X)
		{
			m_CursorPosition.Y++;
			m_CursorPosition.X = 0;
		}
		if (m_CursorPosition.Y == m_BufferSize.Y)
		{
			ASSERT(m_CursorPosition.X == 0);
			SMALL_RECT Src;
			Src.Left = 0;
			Src.Right = (SHORT)(m_BufferSize.X-1);
			Src.Top = 1;
			Src.Bottom = (SHORT)(m_BufferSize.Y-1);
			CHAR_INFO ci;
#ifdef UNICODE
			ci.Char.UnicodeChar = L' ';
#else
			ci.Char.AsciiChar = ' ';
#endif
			ci.Attributes = 0;
			COORD Dest;
			Dest.X = 0;
			Dest.Y = 0;
			if (!ScrollConsoleScreenBuffer(m_hStdOut,&Src,NULL,Dest,&ci)) return FALSE;
			m_CursorPosition.Y--;
			m_LinesScrolled++;
		}
		if (!SetConsoleCursorPosition(m_hStdOut,m_CursorPosition)) return FALSE;
		VERIFY(WriteChar(_T(' ')));
		if ((m_blnMoreMode)&&(m_CursorPosition.X == 0))
		{
			m_Lines++;
			if (m_Lines >= m_BufferSize.Y-1)
			{
				ASSERT(m_Lines == m_BufferSize.Y-1);
				m_Lines = 0;
				VERIFY(WriteString(_T("-- More --"),m_CursorPosition));
				VERIFY(FlushInputBuffer());

				CONSOLE_CURSOR_INFO cci;
				cci.bVisible = FALSE;
				cci.dwSize = 10;
				VERIFY(SetConsoleCursorInfo(m_hStdOut,&cci));

				INPUT_RECORD InputRecord;
				DWORD dwRecordsReaded;
				while ((ret = ReadConsoleInput(m_hStdIn,&InputRecord,1,&dwRecordsReaded)) != FALSE)
				{
					ASSERT(dwRecordsReaded == 1);
					if (dwRecordsReaded != 1) break;
					if (InputRecord.EventType != KEY_EVENT) continue;
					if (!InputRecord.Event.KeyEvent.bKeyDown) continue;
#ifdef UNICODE
					TCHAR ch = InputRecord.Event.KeyEvent.uChar.UnicodeChar;
#else
					TCHAR ch = InputRecord.Event.KeyEvent.uChar.AsciiChar;
#endif
					if (ch == VK_CANCEL)
					{
						VERIFY(GenerateConsoleCtrlEvent(CTRL_C_EVENT,0));
						continue;
					}
					if (ch) break;
				}
				VERIFY(WriteString(_T("          "),m_CursorPosition));
				m_CursorPosition.X = 0;

				cci.bVisible = TRUE;
				cci.dwSize = m_blnInsetMode?m_dwInsertModeCursorHeight:m_dwOverwriteModeCursorHeight;
				VERIFY(SetConsoleCursorInfo(m_hStdOut,&cci));
			}
		}
		dwCharsWrittenAdd++;
		dwCharsToWrite--;
	}
	return ret;
}

BOOL CConsole::SetTitle(TCHAR *p)
{
	return SetConsoleTitle(p);
}

BOOL CConsole::SetTextAttribute(WORD wAttributes)
{
	m_wAttributes = wAttributes;
	return TRUE;
}
/*
BOOL CConsole::SetInputMode(DWORD dwMode)
{
	return SetConsoleMode(m_hStdIn,dwMode);
}

BOOL CConsole::SetOutputMode(DWORD dwMode)
{
	return SetConsoleMode(m_hStdOut,dwMode);
}*/

BOOL CConsole::FlushInputBuffer()
{
	if (m_hStdIn == INVALID_HANDLE_VALUE) return FALSE;
	return FlushConsoleInputBuffer(m_hStdIn);
}

BOOL CConsole::ReadLine()
{
	if (m_hStdIn == INVALID_HANDLE_VALUE) return FALSE;
	if (m_hStdOut == INVALID_HANDLE_VALUE) return FALSE;
	if (m_dwBufferSize == 0)
	{
		ASSERT(FALSE);
		return FALSE;
	}
	if (m_pchBuffer == NULL)
	{
		ASSERT(FALSE);
		return FALSE;
	}
	if (m_pchBuffer1 == NULL)
	{
		ASSERT(FALSE);
		return FALSE;
	}
	if (!FlushConsoleInputBuffer(m_hStdIn)) return FALSE;

	COORD FristCharCursorPosition = m_CursorPosition;
#define X_CURSOR_POSITION_FROM_OFFSET(ofs)	USHORT(((FristCharCursorPosition.X + ofs)%m_BufferSize.X))
#define Y_CURSOR_POSITION_FROM_OFFSET(ofs)	USHORT((FristCharCursorPosition.Y + (FristCharCursorPosition.X + ofs)/m_BufferSize.X))
//#define OFFSET_FROM_CURSOR_POSITION(pos)	((pos.Y-FristCharCursorPosition.Y)*m_BufferSize.X+pos.X-FristCharCursorPosition.X)

	DWORD dwRecordsReaded;
	DWORD dwCurrentCharOffset = 0;
	DWORD dwLastCharOffset = 0;
	BOOL ret;

	BOOL blnCompletionMode = FALSE;
//	unsigned __int64 nCompletionIndex = 0;
	unsigned long long nCompletionIndex = 0;
	DWORD dwCompletionOffset = 0;
	DWORD dwCompletionStringSize = 0;
	COORD CompletionPosition = FristCharCursorPosition;

	m_LinesScrolled = 0;
	BOOL blnOldMoreMode = m_blnMoreMode;
	m_blnMoreMode = FALSE;

	DWORD dwHistoryIndex = 0;

	INPUT_RECORD InputRecord;
	while ((ret = ReadConsoleInput(m_hStdIn,&InputRecord,1,&dwRecordsReaded)) != FALSE)
	{
		ASSERT(dwRecordsReaded == 1);
		if (dwRecordsReaded != 1) return FALSE;
		if (InputRecord.EventType != KEY_EVENT) continue;
		if (!InputRecord.Event.KeyEvent.bKeyDown) continue;
#ifdef UNICODE
		TCHAR ch = InputRecord.Event.KeyEvent.uChar.UnicodeChar;
#else
		TCHAR ch = InputRecord.Event.KeyEvent.uChar.AsciiChar;
#endif
KeyRepeat:
		if (m_LinesScrolled)
		{
			if (m_LinesScrolled > FristCharCursorPosition.Y) return FALSE;
			FristCharCursorPosition.Y = SHORT(FristCharCursorPosition.Y - m_LinesScrolled);
			if (m_LinesScrolled > CompletionPosition.Y) return FALSE;
			CompletionPosition.Y = SHORT(CompletionPosition.Y - m_LinesScrolled);
			m_LinesScrolled = 0;
		}
//		char Buf[1024];
//		sprintf(Buf,"wVirtualKeyCode = %u\nchar = %u\n\n",InputRecord.Event.KeyEvent.wVirtualKeyCode,ch);
//		OutputDebugString(Buf);
		if ((ch == 0x16)&&(InputRecord.Event.KeyEvent.wVirtualKeyCode == 'V'))
		{
			goto Paste;
		}
		else if (ch == 0)
		{
			if (InputRecord.Event.KeyEvent.wVirtualKeyCode == VK_INSERT)
			{
				if (!(InputRecord.Event.KeyEvent.dwControlKeyState & SHIFT_PRESSED))
				{
					VERIFY(SetInsertMode(!m_blnInsetMode));
				}
				else
				{
					if (blnCompletionMode) blnCompletionMode = FALSE;

Paste:
					if (!IsClipboardFormatAvailable(
#ifdef UNICODE
						CF_UNICODETEXT
#else
						CF_TEXT
#endif
						)) 
						continue; 
					if (!OpenClipboard(NULL)) 
						continue; 
 
					const TCHAR *pch = NULL;

					HANDLE hglb = GetClipboardData(
#ifdef UNICODE
						CF_UNICODETEXT
#else
						CF_TEXT
#endif
						);
					if (hglb != NULL) 
					{ 
						LPTSTR lptstr = (LPTSTR)GlobalLock(hglb);
						if (lptstr != NULL) 
						{
							_tcsncpy(m_pchBuffer1,lptstr,m_dwBufferSize);
							m_pchBuffer1[m_dwBufferSize-1] = 0;
							pch = m_pchBuffer1;
							GlobalUnlock(hglb); 
						} 
					} 
					CloseClipboard();

					if (pch == NULL) continue;

					while (*pch)
					{
						if (_istprint(*pch))
						{
							if (dwLastCharOffset >= m_dwBufferSize-1)
							{
								ASSERT(dwLastCharOffset == m_dwBufferSize-1);
								//				Beep(1000,100);
								break;
							}
							TCHAR ch1;
							//if (m_blnInsetMode)
							ch1 = m_pchBuffer[dwCurrentCharOffset];
							m_pchBuffer[dwCurrentCharOffset] = *pch;
							if ((m_blnInsetMode)||(dwCurrentCharOffset == dwLastCharOffset)) dwLastCharOffset++;
							dwCurrentCharOffset++;
							if (!Write(pch,1)) return FALSE;
							if (m_blnInsetMode)
							{
								COORD Cursor = m_CursorPosition;
								DWORD ofs = dwCurrentCharOffset;
								
								while(ofs <= dwLastCharOffset)
								{
									ch = m_pchBuffer[ofs];
									m_pchBuffer[ofs] = ch1;
									ch1 = ch;
									ofs++;
								}
								
								if (dwCurrentCharOffset < dwLastCharOffset)
								{
									if (!Write(m_pchBuffer+dwCurrentCharOffset,dwLastCharOffset-dwCurrentCharOffset)) return FALSE;
									
									if (m_LinesScrolled)
									{
										if (m_LinesScrolled > FristCharCursorPosition.Y) return FALSE;
										Cursor.Y = SHORT(Cursor.Y - m_LinesScrolled);
									}
									// Update cursor position
									m_CursorPosition = Cursor;
									if (!SetConsoleCursorPosition(m_hStdOut,m_CursorPosition)) return FALSE;
								}
							}
						}
						pch++;
					}
				}
			}
			else if (InputRecord.Event.KeyEvent.wVirtualKeyCode == VK_LEFT)
			{
				if (blnCompletionMode) blnCompletionMode = FALSE;
				if (dwCurrentCharOffset)
				{
					if (InputRecord.Event.KeyEvent.dwControlKeyState & (LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED))
					{
						TCHAR *pchWordBegin = m_pchBuffer+dwCurrentCharOffset-1;

						while (pchWordBegin > m_pchBuffer)
						{
							if (!_istspace(*pchWordBegin)) break;
							pchWordBegin--;
						}

						while (pchWordBegin > m_pchBuffer)
						{
							if (_istspace(*(pchWordBegin-1))) break;
							pchWordBegin--;
						}							

						ASSERT(pchWordBegin >= m_pchBuffer);
						dwCurrentCharOffset = pchWordBegin - m_pchBuffer;

						ASSERT(dwCurrentCharOffset < dwLastCharOffset);

						m_CursorPosition.X = X_CURSOR_POSITION_FROM_OFFSET(dwCurrentCharOffset);
						m_CursorPosition.Y = Y_CURSOR_POSITION_FROM_OFFSET(dwCurrentCharOffset);
						VERIFY(SetConsoleCursorPosition(m_hStdOut,m_CursorPosition));
					}
					else
					{
						dwCurrentCharOffset--;
						if (m_CursorPosition.X)
						{
							m_CursorPosition.X--;
						}
						else
						{
							m_CursorPosition.X = SHORT(m_BufferSize.X-1);
							ASSERT(m_CursorPosition.Y);
							m_CursorPosition.Y--;
						}
						VERIFY(SetConsoleCursorPosition(m_hStdOut,m_CursorPosition));
					}
				}
			}
			else if (InputRecord.Event.KeyEvent.wVirtualKeyCode == VK_RIGHT)
			{
				if (blnCompletionMode) blnCompletionMode = FALSE;
				if (dwCurrentCharOffset < dwLastCharOffset)
				{
					if (InputRecord.Event.KeyEvent.dwControlKeyState & (LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED))
					{
						TCHAR *pchWordBegin = m_pchBuffer+dwCurrentCharOffset;

						while ((DWORD)(pchWordBegin - m_pchBuffer) < dwLastCharOffset)
						{
							if (_istspace(*pchWordBegin)) break;
							pchWordBegin++;
						}

						while ((DWORD)(pchWordBegin - m_pchBuffer) < dwLastCharOffset)
						{
							if (!_istspace(*pchWordBegin)) break;
							pchWordBegin++;
						}

						dwCurrentCharOffset = pchWordBegin - m_pchBuffer;
						ASSERT(dwCurrentCharOffset <= dwLastCharOffset);
						m_CursorPosition.X = X_CURSOR_POSITION_FROM_OFFSET(dwCurrentCharOffset);
						m_CursorPosition.Y = Y_CURSOR_POSITION_FROM_OFFSET(dwCurrentCharOffset);
						VERIFY(SetConsoleCursorPosition(m_hStdOut,m_CursorPosition));
					}
					else
					{
						dwCurrentCharOffset++;
						m_CursorPosition.X++;
						if (m_CursorPosition.X == m_BufferSize.X)
						{
							m_CursorPosition.Y++;
							m_CursorPosition.X = 0;
						}
						VERIFY(SetConsoleCursorPosition(m_hStdOut,m_CursorPosition));
					}
				}
			}
			else if (InputRecord.Event.KeyEvent.wVirtualKeyCode == VK_HOME)
			{
				if (blnCompletionMode) blnCompletionMode = FALSE;
				dwCurrentCharOffset = 0;
				m_CursorPosition = FristCharCursorPosition;
				VERIFY(SetConsoleCursorPosition(m_hStdOut,m_CursorPosition));
			}
			else if (InputRecord.Event.KeyEvent.wVirtualKeyCode == VK_END)
			{
				if (blnCompletionMode) blnCompletionMode = FALSE;
				dwCurrentCharOffset = dwLastCharOffset;
				m_CursorPosition.X = X_CURSOR_POSITION_FROM_OFFSET(dwLastCharOffset);
				m_CursorPosition.Y = Y_CURSOR_POSITION_FROM_OFFSET(dwLastCharOffset);
				VERIFY(SetConsoleCursorPosition(m_hStdOut,m_CursorPosition));
			}
			else if (InputRecord.Event.KeyEvent.wVirtualKeyCode == VK_UP)
			{
				if (blnCompletionMode) blnCompletionMode = FALSE;
				dwHistoryIndex++;
				const TCHAR *pchHistoryLine = m_History.GetHistoryLine(dwHistoryIndex-1);
				if (pchHistoryLine)
				{
					if (dwLastCharOffset)
					{
						_tcsnset(m_pchBuffer,_T(' '),dwLastCharOffset);
						m_CursorPosition = FristCharCursorPosition;
						VERIFY(SetConsoleCursorPosition(m_hStdOut,m_CursorPosition));
						VERIFY(Write(m_pchBuffer,dwLastCharOffset));
						dwCurrentCharOffset = dwLastCharOffset = 0;
						m_CursorPosition = FristCharCursorPosition;
						VERIFY(SetConsoleCursorPosition(m_hStdOut,m_CursorPosition));
					}
					dwCurrentCharOffset = dwLastCharOffset = _tcslen(pchHistoryLine);
					if (dwLastCharOffset >= m_dwBufferSize)
					{
						ASSERT(FALSE);
						return FALSE;
					}
					_tcscpy(m_pchBuffer,pchHistoryLine);
					if (!Write(m_pchBuffer)) return FALSE;
				}
				else
				{
					dwHistoryIndex--;
				}
			}
			else if (InputRecord.Event.KeyEvent.wVirtualKeyCode == VK_DOWN)
			{
				if (blnCompletionMode) blnCompletionMode = FALSE;
				if (dwHistoryIndex)
				{
					dwHistoryIndex--;
					const TCHAR *pchHistoryLine = m_History.GetHistoryLine(dwHistoryIndex-1);
					if (dwLastCharOffset)
					{
						_tcsnset(m_pchBuffer,_T(' '),dwLastCharOffset);
						m_CursorPosition = FristCharCursorPosition;
						VERIFY(SetConsoleCursorPosition(m_hStdOut,m_CursorPosition));
						VERIFY(Write(m_pchBuffer,dwLastCharOffset));
						dwCurrentCharOffset = dwLastCharOffset = 0;
						m_CursorPosition = FristCharCursorPosition;
						VERIFY(SetConsoleCursorPosition(m_hStdOut,m_CursorPosition));
					}
					if (pchHistoryLine)
					{
						dwCurrentCharOffset = dwLastCharOffset = _tcslen(pchHistoryLine);
						if (dwLastCharOffset >= m_dwBufferSize)
						{
							ASSERT(FALSE);
							return FALSE;
						}
						_tcscpy(m_pchBuffer,pchHistoryLine);
						if (!Write(m_pchBuffer)) return FALSE;
					}
				}
			}
			else if ((InputRecord.Event.KeyEvent.wVirtualKeyCode == VK_DELETE)&&
				(dwLastCharOffset))
			{
				// Move the characters if any...
				ASSERT(dwLastCharOffset);
				DWORD dwCharOffset = dwCurrentCharOffset;
				if (dwCharOffset < dwLastCharOffset)
				{
					while(dwCharOffset < dwLastCharOffset)
					{
						m_pchBuffer[dwCharOffset] = m_pchBuffer[dwCharOffset+1];
						dwCharOffset++;
					}
					
					m_pchBuffer[dwLastCharOffset-1] = _T(' ');
					
					// Save cursor position
					COORD Cursor = m_CursorPosition;
					
					if (!Write(m_pchBuffer+dwCurrentCharOffset,dwLastCharOffset-dwCurrentCharOffset)) return FALSE;
					
					dwLastCharOffset--;
					
					// Update cursor position
					m_CursorPosition = Cursor;
					if (!SetConsoleCursorPosition(m_hStdOut,m_CursorPosition)) return FALSE;
				}

			}
//			else if ((InputRecord.Event.KeyEvent.wVirtualKeyCode == VK_PAUSE)&&
//				(InputRecord.Event.KeyEvent.dwControlKeyState & (LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED)))
//			{
//					if (!GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT,0)) return FALSE;
//			}
		}
		else if ((ch == 27) && dwLastCharOffset &&
			(InputRecord.Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE))
		{
			if (blnCompletionMode) blnCompletionMode = FALSE;
			_tcsnset(m_pchBuffer,_T(' '),dwLastCharOffset);
			m_CursorPosition = FristCharCursorPosition;
			VERIFY(SetConsoleCursorPosition(m_hStdOut,m_CursorPosition));
			VERIFY(Write(m_pchBuffer,dwLastCharOffset));
			dwCurrentCharOffset = dwLastCharOffset = 0;
			m_CursorPosition = FristCharCursorPosition;
			VERIFY(SetConsoleCursorPosition(m_hStdOut,m_CursorPosition));
		}
		else if (ch == _T('\r'))
		{	// carriage return
			if (!SetConsoleCursorPosition(m_hStdOut,m_CursorPosition = FristCharCursorPosition)) return FALSE;
			ASSERT(dwLastCharOffset <= m_dwBufferSize);
			m_pchBuffer[dwLastCharOffset] = 0;	// terminate string in buffer
			ret = Write(m_pchBuffer);
			m_History.AddHistoryLine(m_pchBuffer);
			TCHAR *strLF = _T("\n");
			ret = Write(strLF);
			break;
		}
		else if (ch == _T('\b'))
		{	// backspace
			if (blnCompletionMode) blnCompletionMode = FALSE;
			if ((dwCurrentCharOffset) && ((m_CursorPosition.X != 0) || (m_CursorPosition.Y != 0)))
			{
				// Calculate new cursor position
				COORD NewCursorPosition;
				if (m_CursorPosition.X)
				{
					NewCursorPosition.X = SHORT(m_CursorPosition.X-1);
					NewCursorPosition.Y = m_CursorPosition.Y;
				}
				else
				{
					ASSERT(m_BufferSize.X);
					NewCursorPosition.X = SHORT(m_BufferSize.X-1);
					ASSERT(m_CursorPosition.Y);
					NewCursorPosition.Y = SHORT(m_CursorPosition.Y-1);
				}

				// Move the characters if any...
				ASSERT(dwLastCharOffset);
				DWORD dwCharOffset = dwCurrentCharOffset-1;
				while(dwCharOffset < dwLastCharOffset-1)
				{
					m_pchBuffer[dwCharOffset] = m_pchBuffer[dwCharOffset+1];
					dwCharOffset++;
				}

				m_pchBuffer[dwLastCharOffset-1] = _T(' ');

				dwCurrentCharOffset--;
				m_CursorPosition = NewCursorPosition;
				if (!Write(m_pchBuffer+dwCurrentCharOffset,dwLastCharOffset-dwCurrentCharOffset)) return FALSE;

				// Update cursor position
				m_CursorPosition = NewCursorPosition;
				if (!SetConsoleCursorPosition(m_hStdOut,m_CursorPosition)) return FALSE;

				dwLastCharOffset--;
			}
		}
		else if (ch == _T('\t'))
		{
			if (!blnCompletionMode)
			{
				if (InputRecord.Event.KeyEvent.dwControlKeyState & SHIFT_PRESSED)
//					nCompletionIndex = 0xFFFFFFFFFFFFFFFF;
					nCompletionIndex = (unsigned long long) -1;
				else
					nCompletionIndex = 0;
				dwCompletionOffset = dwCurrentCharOffset;
				BOOL b = FALSE;
				while(dwCompletionOffset)
				{
					dwCompletionOffset--;
					if (m_pchBuffer[dwCompletionOffset] == _T('\"'))
					{
						b = !b;
					}
					else if (!b && _istspace(m_pchBuffer[dwCompletionOffset]))
					{
						dwCompletionOffset++;
						break;
					}
				}
				ASSERT(dwCompletionOffset <= dwCurrentCharOffset);
				_tcsncpy(m_pchBuffer1,m_pchBuffer,dwCompletionOffset);
				m_pchBuffer1[dwCompletionOffset] = 0;
				dwCompletionStringSize = dwCurrentCharOffset-dwCompletionOffset;
				if (dwCompletionStringSize)
					_tcsncpy(m_pchBuffer2,m_pchBuffer+dwCompletionOffset,dwCompletionStringSize);
				m_pchBuffer2[dwCompletionStringSize] = 0;
				CompletionPosition.X = X_CURSOR_POSITION_FROM_OFFSET(dwCompletionOffset);
				CompletionPosition.Y = Y_CURSOR_POSITION_FROM_OFFSET(dwCompletionOffset);
//				Beep(1000,500);
			}
			else
			{
//				Beep(1000,50);
//				Beep(2000,50);
//				Beep(3000,50);
//				Beep(4000,50);
//				Beep(3000,50);
//				Beep(2000,50);
//				Beep(1000,50);
			}
			const TCHAR *pchCompletion = NULL;
			BOOL blnForward = !(InputRecord.Event.KeyEvent.dwControlKeyState & SHIFT_PRESSED);
			if (m_pfReplaceCompletionCallback)
				pchCompletion = m_pfReplaceCompletionCallback(nCompletionIndex,
				blnCompletionMode?&blnForward:NULL,
				m_pchBuffer1,m_pchBuffer2);
			if (pchCompletion)
			{
				// Set cursor position
				m_CursorPosition = CompletionPosition;
				if (!SetConsoleCursorPosition(m_hStdOut,m_CursorPosition)) return FALSE;

				// Calculate buffer free space
				ASSERT(m_dwBufferSize > dwCompletionOffset);
				DWORD dwFree = m_dwBufferSize - dwCompletionOffset - 1;

				DWORD dwOldCompletionStringSize = dwCompletionStringSize;

				// Write completion string to buffer
				dwCompletionStringSize = _tcslen(pchCompletion);

				if (dwCompletionStringSize > dwFree)
					dwCompletionStringSize = dwFree;
				if (dwCompletionStringSize)
				{
					_tcsncpy(m_pchBuffer+dwCompletionOffset,pchCompletion,dwCompletionStringSize);
//					m_pchBuffer[dwCompletionOffset+dwCompletionStringSize] = 0;
					
					// Write completion string to console
					if (!Write(m_pchBuffer+dwCompletionOffset,dwCompletionStringSize)) return FALSE;
					dwCurrentCharOffset = dwLastCharOffset = dwCompletionOffset + dwCompletionStringSize;
					ASSERT(dwLastCharOffset < m_dwBufferSize);
				}

				// Erase rest from previous completion string
				if (dwOldCompletionStringSize > dwCompletionStringSize)
				{
					_tcsnset(m_pchBuffer+dwCompletionOffset+dwCompletionStringSize,_T(' '),
						dwOldCompletionStringSize - dwCompletionStringSize);

					// Save cursor position
					COORD pos = m_CursorPosition;

					if (!Write(m_pchBuffer+dwCompletionOffset+dwCompletionStringSize,
						dwOldCompletionStringSize - dwCompletionStringSize))
						return FALSE;

					// Set cursor position
					m_CursorPosition = pos;
					if (!SetConsoleCursorPosition(m_hStdOut,m_CursorPosition)) return FALSE;
				}

			}
			else
			{
/*				if (InputRecord.Event.KeyEvent.dwControlKeyState & SHIFT_PRESSED)
				{
					 nCompletionIndex++;
				}
				else
				{
					if (nCompletionIndex)
						nCompletionIndex--;
				}*/
			}
			blnCompletionMode = TRUE;
		}
		else if (_istprint(ch))
		{
			if (blnCompletionMode) blnCompletionMode = FALSE;
			if (dwLastCharOffset >= m_dwBufferSize-1)
			{
				ASSERT(dwLastCharOffset == m_dwBufferSize-1);
//				Beep(1000,100);
				continue;
			}
			TCHAR ch1;
			//if (m_blnInsetMode)
			ch1 = m_pchBuffer[dwCurrentCharOffset];
			m_pchBuffer[dwCurrentCharOffset] = ch;
			if ((m_blnInsetMode)||(dwCurrentCharOffset == dwLastCharOffset)) dwLastCharOffset++;
			dwCurrentCharOffset++;
			if (!Write(&ch,1)) return FALSE;
			if (m_blnInsetMode)
			{
				COORD Cursor = m_CursorPosition;
				DWORD ofs = dwCurrentCharOffset;

				while(ofs <= dwLastCharOffset)
				{
					ch = m_pchBuffer[ofs];
					m_pchBuffer[ofs] = ch1;
					ch1 = ch;
					ofs++;
				}

				if (dwCurrentCharOffset < dwLastCharOffset)
				{
					if (!Write(m_pchBuffer+dwCurrentCharOffset,dwLastCharOffset-dwCurrentCharOffset)) return FALSE;

					if (m_LinesScrolled)
					{
						if (m_LinesScrolled > FristCharCursorPosition.Y) return FALSE;
						Cursor.Y = SHORT(Cursor.Y - m_LinesScrolled);
					}
					// Update cursor position
					m_CursorPosition = Cursor;
					if (!SetConsoleCursorPosition(m_hStdOut,m_CursorPosition)) return FALSE;
				}
			}
		}
		ASSERT(InputRecord.Event.KeyEvent.wRepeatCount);
		if (!InputRecord.Event.KeyEvent.wRepeatCount) return FALSE;
		if (--InputRecord.Event.KeyEvent.wRepeatCount) goto KeyRepeat;
	}
	m_blnMoreMode = blnOldMoreMode;
	return TRUE;
}

BOOL CConsole::GetTextAttribute(WORD& rwAttributes)
{
	rwAttributes = m_wAttributes;
	return TRUE;
}

// Parameters:
//		dwBufferSize - size in chars of the input line buffer
//
// Rerturns:
//		NULL - Failed.
//		pointer to the input buffer
TCHAR * CConsole::Init(DWORD dwBufferSize, DWORD dwMaxHistoryLines)
{
	if (m_hStdIn != INVALID_HANDLE_VALUE) VERIFY(CloseHandle(m_hStdIn));
	if (m_hStdOut != INVALID_HANDLE_VALUE) VERIFY(CloseHandle(m_hStdOut));

	m_hStdIn = GetStdHandle(STD_INPUT_HANDLE);
	if (m_hStdIn == INVALID_HANDLE_VALUE) goto Abort;

	m_hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (m_hStdOut == INVALID_HANDLE_VALUE) goto Abort;

	CONSOLE_SCREEN_BUFFER_INFO info;
	if (!GetConsoleScreenBufferInfo(m_hStdOut,&info)) goto Abort;
	m_wAttributes = info.wAttributes;
	
	if (!SetConsoleMode(m_hStdIn,0)) goto Abort;
	if (!SetConsoleMode(m_hStdOut,0)) goto Abort;

	m_CursorPosition = info.dwCursorPosition;
	m_BufferSize = info.dwSize;

	CONSOLE_CURSOR_INFO cci;
	cci.bVisible = TRUE;
	cci.dwSize = m_blnInsetMode?m_dwInsertModeCursorHeight:m_dwOverwriteModeCursorHeight;

	if (!SetConsoleCursorInfo(m_hStdOut,&cci)) goto Abort;

	m_dwBufferSize = dwBufferSize;

	if (m_pchBuffer) delete m_pchBuffer;
	m_pchBuffer = NULL;

	if (m_pchBuffer1) delete m_pchBuffer1;
	m_pchBuffer1 = NULL;

	if (m_pchBuffer2) delete m_pchBuffer2;
	m_pchBuffer2 = NULL;

	m_pchBuffer = new TCHAR [dwBufferSize];
	if (!m_pchBuffer) goto Abort;
	m_pchBuffer[dwBufferSize-1] = 0;

	m_pchBuffer1 = new TCHAR [dwBufferSize];
	if (!m_pchBuffer1) goto Abort;
	m_pchBuffer1[dwBufferSize-1] = 0;

	m_pchBuffer2 = new TCHAR [dwBufferSize];
	if (!m_pchBuffer2) goto Abort;
	m_pchBuffer2[dwBufferSize-1] = 0;

	if (dwMaxHistoryLines)
	{
		if (!m_History.Init(dwBufferSize,dwMaxHistoryLines)) goto Abort;
	}

	return m_pchBuffer;

Abort:
	if (m_hStdIn != INVALID_HANDLE_VALUE) VERIFY(CloseHandle(m_hStdIn));
	m_hStdIn = INVALID_HANDLE_VALUE;

	if (m_hStdOut != INVALID_HANDLE_VALUE) VERIFY(CloseHandle(m_hStdOut));
	m_hStdOut = INVALID_HANDLE_VALUE;

	if (m_pchBuffer) delete m_pchBuffer;
	m_pchBuffer = NULL;

	if (m_pchBuffer1) delete m_pchBuffer1;
	m_pchBuffer1 = NULL;

	if (m_pchBuffer2) delete m_pchBuffer2;
	m_pchBuffer2 = NULL;

	m_dwBufferSize = 0;

	return NULL;
}

BOOL CConsole::WriteChar(TCHAR ch)
{
	CHAR_INFO ci;
	ci.Attributes = m_wAttributes;
#ifdef UNICODE
	ci.Char.UnicodeChar = ch;
#else
	ci.Char.AsciiChar = ch;
#endif
	static COORD BufferSize = {1,1};
	static COORD BufferCoord = {0,0};
	SMALL_RECT Dest;
	Dest.Bottom = Dest.Top = m_CursorPosition.Y;
	Dest.Left = Dest.Right = m_CursorPosition.X;
	return WriteConsoleOutput(m_hStdOut,&ci,BufferSize,BufferCoord,&Dest);
}

void CConsole::BeginScrollingOperation()
{
	m_Lines = 0;
}

BOOL CConsole::WriteString(TCHAR *pchString, COORD Position)
{
	CHAR_INFO ciBuffer[256];
	int nSize = _tcslen(pchString);
	if ((nSize > 256)||(nSize <= 0))
	{
		ASSERT(FALSE);
		return FALSE;
	}

	COORD BufferSize;
	BufferSize.X = (SHORT)nSize;
	BufferSize.Y = 1;
	static COORD BufferCoord = {0,0};
	SMALL_RECT Dest;
	Dest.Bottom = Dest.Top = Position.Y;
	Dest.Right = SHORT((Dest.Left = Position.X) + nSize - 1);

	while(nSize--)
	{
		ciBuffer[nSize].Attributes = m_wAttributes;
#ifdef UNICODE
		ciBuffer[nSize].Char.UnicodeChar = pchString[nSize];
#else
		ciBuffer[nSize].Char.AsciiChar = pchString[nSize];
#endif
	}

	return WriteConsoleOutput(m_hStdOut,ciBuffer,BufferSize,BufferCoord,&Dest);
}

BOOL CConsole::SetInsertMode(BOOL blnInsetMode)
{
	if (m_hStdOut == INVALID_HANDLE_VALUE) return FALSE;

	CONSOLE_CURSOR_INFO cci;
	cci.bVisible = TRUE;
	cci.dwSize = blnInsetMode?m_dwInsertModeCursorHeight:m_dwOverwriteModeCursorHeight;

	BOOL ret = SetConsoleCursorInfo(m_hStdOut,&cci);
	if (ret) m_blnInsetMode = blnInsetMode;
	return ret;
}



void CConsole::SetReplaceCompletionCallback(ReplaceCompletionCallback pfCallback)
{
	m_pfReplaceCompletionCallback = pfCallback;
}


void CConsole::DisableWrite()
{
	m_blnDisableWrite = TRUE;
	INPUT_RECORD InputRecord;
	DWORD dwRecordsWriten;
	InputRecord.EventType = KEY_EVENT;
	InputRecord.Event.KeyEvent.bKeyDown = TRUE;
#ifdef UNICODE
	InputRecord.Event.KeyEvent.uChar.UnicodeChar = L' ';
#else
	InputRecord.Event.KeyEvent.uChar.AsciiChar = ' ';
#endif
	BOOL ret = WriteConsoleInput(m_hStdIn,&InputRecord,1,&dwRecordsWriten);
	ASSERT(ret);
}

void CConsole::EnableWrite()
{
	m_blnDisableWrite = FALSE;
}

/*				DWORD ofs = dwCurrentCharOffset;
				DWORD nLines = (FristCharCursorPosition.X + dwLastCharOffset-1)/m_BufferSize.X;
				for (DWORD nLine = 0 ; nLine <= nLines ; nLine++)
				{
					ASSERT(m_BufferSize.X > pos.X);
					DWORD nChars = m_BufferSize.X - pos.X - 1;
					if (nChars > dwLastCharOffset - ofs) nChars = dwLastCharOffset - ofs;
					if (nChars)
					{	// We have some chars to move in this line
						_tcsncpy(m_pchBuffer1,m_pchBuffer+ofs,nChars);
						m_pchBuffer1[nChars] = 0;
						if (!WriteString(m_pchBuffer1,pos)) return FALSE;
					}
					pos.X = SHORT(pos.X + nChars);
					// if current line is not the last line
					// Move the first char in next line to end of current line
					if (nLine < nLines)
					{
						ofs += nChars;
						m_pchBuffer1[0] = m_pchBuffer[ofs];
						m_pchBuffer1[1] = 0;
						if (!WriteString(m_pchBuffer1,pos)) return FALSE;
						pos.Y++;
						pos.X = 0;
						ofs++;
					}
					else	// Adjust end of read line
					{
						m_pchBuffer1[0] = _T(' ');
						m_pchBuffer1[1] = 0;
						if (!WriteString(m_pchBuffer1,pos)) return FALSE;
					}
				}*/
