/*
 *  MOVE.C - move internal command.
 *
 *
 *  History:
 *
 *    14-Dec-1998 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Started.
 *
 *    18-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Unicode safe!
 *        Preliminary version!!!
 *
 *    20-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Redirection safe!
 *
 *    27-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Added help text ("/?").
 *        Added more error checks.
 *
 *    03-Feb-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Added "/N" option.
 *
 *    30-Apr-2005 (Magnus Olsen) <magnus@greatlord.com>)
 *        Remove all hardcode string to En.rc
 */

#include "precomp.h"
#include "resource.h"

#ifdef INCLUDE_CMD_MOVE


#define OVERWRITE_NO     0
#define OVERWRITE_YES    1
#define OVERWRITE_ALL    2
#define OVERWRITE_CANCEL 3


static INT Overwrite (LPTSTR fn)
{
	TCHAR szMsg[RC_STRING_MAX_SIZE];
	TCHAR inp[10];
	LPTSTR p;

	LoadString(CMD_ModuleHandle, STRING_MOVE_HELP1, szMsg, RC_STRING_MAX_SIZE);
	ConOutPrintf(szMsg, fn);
	ConInString(inp, 10);

	_tcsupr (inp);
	for (p = inp; _istspace(*p); p++)
		;

	if (*p != szMsg[0] && *p != szMsg[2])
		return OVERWRITE_NO;
	if (*p == szMsg[2])
		return OVERWRITE_ALL;

	return OVERWRITE_YES;
}



INT cmd_move (LPTSTR cmd, LPTSTR param)
{
	TCHAR szMsg[RC_STRING_MAX_SIZE];
	LPTSTR *arg;
	INT argc, i, nFiles;
	TCHAR szDestPath[MAX_PATH];
	TCHAR szSrcPath[MAX_PATH];
	BOOL bPrompt = TRUE;
	LPTSTR p;
	WIN32_FIND_DATA findBuffer;
	HANDLE hFile;
	LPTSTR pszFile;
	BOOL bNothing = FALSE;

	if (!_tcsncmp (param, _T("/?"), 2))
	{
#if 0
		ConOutPuts (_T("Moves files and renames files and directories.\n\n"
		               "To move one or more files:\n"
		               "MOVE [/N][/Y|/-Y][drive:][path]filename1[,...] destination\n"
		               "\n"
		               "To rename a directory:\n"
		               "MOVE [/N][/Y|/-Y][drive:][path]dirname1 dirname2\n"
		               "\n"
		               "  [drive:][path]filename1  Specifies the location and name of the file\n"
		               "                           or files you want to move.\n"
		               "  /N                       Nothing. Don everthing but move files or direcories.\n"
		               "  /Y\n"
		               "  /-Y\n"
		               "..."));
#else
		ConOutResPuts(STRING_MOVE_HELP2);
#endif
		return 0;
	}

	arg = split (param, &argc, FALSE);
	nFiles = argc;

	/* read options */
	for (i = 0; i < argc; i++)
	{
		p = arg[i];

		if (*p == _T('/'))
		{
			p++;
			if (*p == _T('-'))
			{
				p++;
				if (_totupper (*p) == _T('Y'))
					bPrompt = TRUE;
			}
			else
			{
				if (_totupper (*p) == _T('Y'))
					bPrompt = FALSE;
				else if (_totupper (*p) == _T('N'))
					bNothing = TRUE;
			}
			nFiles--;
		}
	}

	if (nFiles < 2)
	{
		/* there must be at least two pathspecs */
		error_req_param_missing ();
		return 1;
	}

	/* get destination */
	GetFullPathName (arg[argc - 1], MAX_PATH, szDestPath, NULL);
#ifdef _DEBUG
	DebugPrintf (_T("Destination: %s\n"), szDestPath);
#endif

	/* move it*/
	for (i = 0; i < argc - 1; i++)
	{
		if (*arg[i] == _T('/'))
			continue;

		hFile = FindFirstFile (arg[i], &findBuffer);
		if (hFile == INVALID_HANDLE_VALUE)
		{
			ErrorMessage (GetLastError (), arg[i]);
			freep (arg);
			return 1;
		}

		do
		{
			GetFullPathName (findBuffer.cFileName, MAX_PATH, szSrcPath, &pszFile);

			if (GetFileAttributes (szSrcPath) & FILE_ATTRIBUTE_DIRECTORY)
			{
				/* source is directory */

#ifdef _DEBUG
				DebugPrintf (_T("Move directory \'%s\' to \'%s\'\n"),
							 szSrcPath, szDestPath);
#endif
				if (!bNothing)
				{
					MoveFile (szSrcPath, szDestPath);
				}
			}
			else
			{
				/* source is file */

				if (IsExistingFile (szDestPath))
				{
					/* destination exists */
					if (GetFileAttributes (szDestPath) & FILE_ATTRIBUTE_DIRECTORY)
					{
						/* destination is existing directory */

						TCHAR szFullDestPath[MAX_PATH];

						_tcscpy (szFullDestPath, szDestPath);
						_tcscat (szFullDestPath, _T("\\"));
						_tcscat (szFullDestPath, pszFile);

						ConOutPrintf (_T("%s => %s"), szSrcPath, szFullDestPath);

						if (!bNothing)
						{
							if (MoveFile (szSrcPath, szFullDestPath))
								LoadString(CMD_ModuleHandle, STRING_MOVE_ERROR1, szMsg, RC_STRING_MAX_SIZE);
							else
								LoadString(CMD_ModuleHandle, STRING_MOVE_ERROR2, szMsg, RC_STRING_MAX_SIZE);
							ConOutPrintf(szMsg);
						}
					}
					else
					{
						/* destination is existing file */
						INT nOverwrite;

						/* must get the overwrite code */
						if ((nOverwrite = Overwrite (szDestPath)))
						{
#if 0
							if (nOverwrite == OVERWRITE_ALL)
								*lpFlags |= FLAG_OVERWRITE_ALL;
#endif
							ConOutPrintf (_T("%s => %s"), szSrcPath, szDestPath);

							if (!bNothing)
							{
								if (MoveFile (szSrcPath, szDestPath))
									LoadString(CMD_ModuleHandle, STRING_MOVE_ERROR1, szMsg, RC_STRING_MAX_SIZE);
								else
									LoadString(CMD_ModuleHandle, STRING_MOVE_ERROR2, szMsg, RC_STRING_MAX_SIZE);
								ConOutPrintf(szMsg);
							}
						}
					}
				}
				else
				{
					/* destination does not exist */
					TCHAR szFullDestPath[MAX_PATH];

					GetFullPathName (szDestPath, MAX_PATH, szFullDestPath, NULL);

					ConOutPrintf (_T("%s => %s"), szSrcPath, szFullDestPath);

					if (!bNothing)
					{
						if (MoveFile (szSrcPath, szFullDestPath))
							LoadString(CMD_ModuleHandle, STRING_MOVE_ERROR1, szMsg, RC_STRING_MAX_SIZE);
						else
							LoadString(CMD_ModuleHandle, STRING_MOVE_ERROR2, szMsg, RC_STRING_MAX_SIZE);
						ConOutPrintf(szMsg);
					}
				}
			}
		}
		while (FindNextFile (hFile, &findBuffer));

		FindClose (hFile);
	}

	freep (arg);

	return 0;
}

#endif /* INCLUDE_CMD_MOVE */
