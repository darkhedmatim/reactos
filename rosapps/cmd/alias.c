/*
 *  ALIAS.C - alias administration module.
 *
 *
 *  History:
 *
 *    02/02/1996 (Oliver Mueller)
 *        started.
 *
 *    02/03/1996 (Oliver Mueller)
 *        Added sorting algorithm and case sensitive substitution by using
 *        partstrupr().
 *
 *    27 Jul 1998  John P. Price
 *        added config.h include
 *        added ifdef's to disable aliases
 *
 *    09-Dec-1998 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Fixed crash when removing an alias in DeleteAlias().
 *        Added help text ("/?").
 *
 *    14-Jan-1998 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Clean up and Unicode safe!
 *
 *    24-Jan-1998 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Redirection safe!
 */


#include "config.h"

#ifdef FEATURE_ALIASES

#include <windows.h>
#include <tchar.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "cmd.h"


typedef struct tagALIAS
{
	struct tagALIAS *next;
	LPTSTR lpName;
	LPTSTR lpSubst;
	WORD   wUsed;
} ALIAS, *LPALIAS;


static LPALIAS lpFirst = NULL;
static LPALIAS lpLast = NULL;
static WORD    wUsed = 0;


/* module internal functions */
/* strlwr only for first word in string */
static VOID
partstrlwr (LPTSTR str)
{
	LPTSTR c = str;
	while (*c && !_istspace (*c) && *c != _T('='))
	{
		*c = _totlower (*c);
		c++;
	}
}


static VOID
PrintAlias (VOID)
{
	LPALIAS ptr = lpFirst;
	while (ptr)
	{
		ConOutPrintf (_T("%s=%s\n"), ptr->lpName, ptr->lpSubst);
		ptr = ptr->next;
	}
}


static VOID
DeleteAlias (LPTSTR pszName)
{
	LPALIAS ptr = lpFirst;
	LPALIAS prev = NULL;

	while (ptr)
	{
		if (!_tcsicmp (ptr->lpName, pszName))
		{
			if (prev)
				prev->next = ptr->next;
			else
				lpFirst = ptr->next;
			free (ptr->lpName);
			free (ptr->lpSubst);
			free (ptr);
			return;
		}
		prev = ptr;
		ptr = ptr->next;
	}
}


static VOID
AddAlias (LPTSTR name, LPTSTR subst)
{
	LPALIAS ptr = lpFirst;
	LPALIAS prev, entry;
	LPTSTR s;

	while (ptr)
	{
		if (!_tcsicmp (ptr->lpName, name))
		{
			s = (LPTSTR)malloc ((_tcslen (subst) + 1)*sizeof(TCHAR));
			if (!s)
			{
				error_out_of_memory ();
				return;
			}

			free (ptr->lpSubst);
			ptr->lpSubst = s;
			_tcscpy (ptr->lpSubst, subst);
			return;
		}
		ptr = ptr->next;
	}

	ptr = (LPALIAS)malloc (sizeof (ALIAS));
	if (!ptr)
		return;

	ptr->next = 0;

	ptr->lpName = (LPTSTR)malloc ((_tcslen (name) + 1)*sizeof(TCHAR));
	if (!ptr->lpName)
	{
		error_out_of_memory ();
		free (ptr);
		return;
	}
	_tcscpy (ptr->lpName, name);

	ptr->lpSubst = (LPTSTR)malloc ((_tcslen (subst) + 1)*sizeof(TCHAR));
	if (!ptr->lpSubst)
	{
		error_out_of_memory ();
		free (ptr->lpName);
		free (ptr);
		return;
	}
	_tcscpy (ptr->lpSubst, subst);

	/* it's necessary for recursive substitution */
	partstrlwr (ptr->lpSubst);

	ptr->wUsed = 0;

	/* Alias table must be sorted!
	 * Here a little example:
	 *   command line = "ls -c"
	 * If the entries are
	 *   ls=dir
	 *   ls -c=ls /w
	 * command line will be expanded to "dir -c" which is not correct.
	 * If the entries are sortet as
	 *   ls -c=ls /w
	 *   ls=dir
	 * it will be expanded to "dir /w" which is a valid DOS command.
	 */
	entry = lpFirst;
	prev = 0;
	while (entry)
	{
		if (_tcsicmp (ptr->lpName, entry->lpName) > 0)
		{
			if (prev)
			{
				prev->next = ptr;
				ptr->next = entry;
			}
			else
			{
				ptr->next = entry;
				lpFirst = ptr;
			}
			return;
		}
		prev = entry;
		entry = entry->next;
	}

	/* The new entry is the smallest (or the first) and must be
	 * added to the end of the list.
	 */
	if (!lpFirst)
		lpFirst = ptr;
	else
		lpLast->next = ptr;
	lpLast = ptr;

	return;
}


/* specified routines */
VOID ExpandAlias (LPTSTR cmd, INT maxlen)
{
	unsigned n = 0,
		m,
		i,
		len;
	short d = 1;
	LPALIAS ptr = lpFirst;

	wUsed++;
	if (wUsed == 0)
	{
		while (ptr)
			ptr->wUsed = 0;
		ptr = lpFirst;
		wUsed = 1;
	}

	/* skipping white spaces */
	while (_istspace (cmd[n]))
		n++;

	partstrlwr (&cmd[n]);

	if (!_tcsncmp (&cmd[n], _T("NOALIAS"), 7) &&
		(_istspace (cmd[n + 7]) || cmd[n + 7] == _T('\0')))
	{
		memmove (cmd, &cmd[n + 7], (_tcslen (&cmd[n + 7]) + 1) * sizeof (TCHAR));
		return;
	}

	/* substitution loop */
	while (d)
	{
		d = 0;
		while (ptr)
		{
			len = _tcslen (ptr->lpName);
			if (!_tcsncmp (&cmd[n], ptr->lpName, len) &&
				(_istspace (cmd[n + len]) || cmd[n + len] == _T('\0')) &&
				ptr->wUsed != wUsed)
			{
				m = _tcslen (ptr->lpSubst);
				if ((int)(_tcslen (cmd) - len + m - n) > maxlen)
				{
					ConErrPrintf (_T("Command line too long after alias expansion!\n"));
					/* the parser won't cause any problems with an empty line */
					cmd[0] = _T('\0');
				}
				else
				{
					memmove (&cmd[m], &cmd[n + len], (_tcslen(&cmd[n + len]) + 1) * sizeof (TCHAR));
					for (i = 0; i < m; i++)
						cmd[i] = ptr->lpSubst[i];
					ptr->wUsed = wUsed;
					/* whitespaces are removed! */
					n = 0;
					d = 1;
				}
			}
			ptr = ptr->next;
		}
	}
}


VOID CommandAlias (LPTSTR cmd, LPTSTR param)
{
	LPTSTR ptr;

	if (!_tcsncmp (param, _T("/?"), 2))
	{
		ConOutPuts (_T("Sets, removes or shows aliases.\n"
		               "\n"
		               "ALIAS [alias=[command]]\n"
		               "\n"
		               "  alias    Name for an alias.\n"
		               "  command  Text to be substituted for an alias.\n"
		               "\n"
//					   "For example:\n"
					   "To list all aliases:\n"
					   "  ALIAS\n\n"
					   "To set a new or replace an existing alias:\n"
					   "  ALIAS da=dir a:\n\n"
					   "To remove an alias from the alias list:\n"
					   "  ALIAS da="
//					   "Type ALIAS without a parameter to display the alias list.\n"
					   ));
		return;
	}

	if (param[0] == _T('\0'))
	{
		PrintAlias ();
		return;
	}

	/* error if no '=' found */
	if ((ptr = _tcschr (param, _T('='))) == 0)
		return;

	/* Split rest into name and substitute */
	*ptr++ = _T('\0');

	partstrlwr (param);

	if (ptr[0] == _T('\0'))
		DeleteAlias (param);
	else
		AddAlias (param, ptr);

	return;
}
#endif
