/*
 * Copyright (c) 1987, 1993, 1994, 1996
 *  The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *  This product includes software developed by the University of
 *  California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

extern int   opterr;      /* if error message should be printed */
extern int   optind;      /* index into parent argv vector */
extern int   optopt;      /* character checked for validity */
extern int   optreset;    /* reset getopt */
extern char *optarg;      /* argument associated with option */

int getopt (int, char * const *, const char *);

struct option {
	const char *name;
	int  has_arg;
	int *flag;
	int val;
};

int getopt_long (int, char *const *, const char *, const struct option *, int *);

#define no_argument             0
#define required_argument       1
#define optional_argument       2

#define REPLACE_GETOPT

#define _DIAGASSERT(x) do {} while (0)

#ifdef REPLACE_GETOPT
int opterr = 1;
int optind = 1;
int optopt = '?';
int optreset;
char *optarg;
#endif

#define __progname __argv[0]

#define IGNORE_FIRST (*options == '-' || *options == '+')
#define PRINT_ERROR ((opterr) && ((*options != ':') || (IGNORE_FIRST && options[1] != ':')))

#ifndef IS_POSIXLY_CORRECT
#define IS_POSIXLY_CORRECT (getenv("POSIXLY_CORRECT") != NULL)
#endif

#define PERMUTE (!IS_POSIXLY_CORRECT && !IGNORE_FIRST)

#define IN_ORDER (!IS_POSIXLY_CORRECT && *options == '-')

#define BADCH (int)'?'
#define BADARG ((IGNORE_FIRST && options[1] == ':') || (*options == ':') ? (int)':' : (int)'?')
#define INORDER (int)1

static char EMSG[1];

static int getopt_internal (int,char * const *,const char *);
static int gcd (int,int);
static void permute_args (int,int,int,char * const *);

static char *place = EMSG;

static int nonopt_start = -1;
static int nonopt_end = -1;

static const char recargchar[] = "option requires an argument -- %c";
static const char recargstring[] = "option requires an argument -- %s";
static const char ambig[] = "ambiguous option -- %.*s";
static const char noarg[] = "option doesn't take an argument -- %.*s";
static const char illoptchar[] = "unknown option -- %c";
static const char illoptstring[] = "unknown option -- %s";

static void
_vwarnx(const char *fmt,va_list ap)
{
  (void)fprintf(stderr,"%s: ",__progname);
  if (fmt != NULL)
    (void)vfprintf(stderr,fmt,ap);
  (void)fprintf(stderr,"\n");
}

static void
warnx(const char *fmt,...)
{
  va_list ap;
  va_start(ap,fmt);
  _vwarnx(fmt,ap);
  va_end(ap);
}

static int
gcd(a,b)
	int a;
	int b;
{
	int c;

	c = a % b;
	while (c != 0) {
		a = b;
		b = c;
		c = a % b;
	}

	return b;
}

static void
permute_args(panonopt_start,panonopt_end,opt_end,nargv)
	int panonopt_start;
	int panonopt_end;
	int opt_end;
	char * const *nargv;
{
	int cstart,cyclelen,i,j,ncycle,nnonopts,nopts,pos;
	char *swap;

	_DIAGASSERT(nargv != NULL);

	nnonopts = panonopt_end - panonopt_start;
	nopts = opt_end - panonopt_end;
	ncycle = gcd(nnonopts,nopts);
	cyclelen = (opt_end - panonopt_start) / ncycle;

	for (i = 0; i < ncycle; i++) {
		cstart = panonopt_end+i;
		pos = cstart;
		for (j = 0; j < cyclelen; j++) {
			if (pos >= panonopt_end)
				pos -= nnonopts;
			else
				pos += nopts;
			swap = nargv[pos];

			((char **) nargv)[pos] = nargv[cstart];

			((char **)nargv)[cstart] = swap;
		}
	}
}

static int
getopt_internal(nargc,nargv,options)
	int nargc;
	char * const *nargv;
	const char *options;
{
	char *oli;
	int optchar;

	_DIAGASSERT(nargv != NULL);
	_DIAGASSERT(options != NULL);

	optarg = NULL;

	if (optind == 0)
		optind = 1;

	if (optreset)
		nonopt_start = nonopt_end = -1;
start:
	if (optreset || !*place) {
		optreset = 0;
		if (optind >= nargc) {
			place = EMSG;
			if (nonopt_end != -1) {

				permute_args(nonopt_start,nonopt_end,optind,nargv);
				optind -= nonopt_end - nonopt_start;
			}
			else if (nonopt_start != -1) {

				optind = nonopt_start;
			}
			nonopt_start = nonopt_end = -1;
			return -1;
		}
		if ((*(place = nargv[optind]) != '-')
		    || (place[1] == '\0')) {
			place = EMSG;
			if (IN_ORDER) {

				optarg = nargv[optind++];
				return INORDER;
			}
			if (!PERMUTE) {

				return -1;
			}

			if (nonopt_start == -1)
				nonopt_start = optind;
			else if (nonopt_end != -1) {
				permute_args(nonopt_start,nonopt_end,optind,nargv);
				nonopt_start = optind -
				    (nonopt_end - nonopt_start);
				nonopt_end = -1;
			}
			optind++;

			goto start;
		}
		if (nonopt_start != -1 && nonopt_end == -1)
			nonopt_end = optind;
		if (place[1] && *++place == '-') {
			place++;
			return -2;
		}
	}
	if ((optchar = (int)*place++) == (int)':' ||
	    (oli = strchr(options + (IGNORE_FIRST ? 1 : 0),optchar)) == NULL) {

		if (!*place)
			++optind;
		if (PRINT_ERROR)
			warnx(illoptchar,optchar);
		optopt = optchar;
		return BADCH;
	}
	if (optchar == 'W' && oli[1] == ';') {

		if (*place)
			return -2;

		if (++optind >= nargc) {
			place = EMSG;
			if (PRINT_ERROR)
				warnx(recargchar,optchar);
			optopt = optchar;
			return BADARG;
		} else
			place = nargv[optind];

		return -2;
	}
	if (*++oli != ':') {
		if (!*place)
			++optind;
	} else {
		optarg = NULL;
		if (*place)
			optarg = place;

		else if (oli[1] != ':') {
			if (++optind >= nargc) {
				place = EMSG;
				if (PRINT_ERROR)
					warnx(recargchar,optchar);
				optopt = optchar;
				return BADARG;
			} else
				optarg = nargv[optind];
		}
		place = EMSG;
		++optind;
	}

	return optchar;
}

#ifdef REPLACE_GETOPT

int
getopt(nargc,nargv,options)
	int nargc;
	char * const *nargv;
	const char *options;
{
	int retval;

	_DIAGASSERT(nargv != NULL);
	_DIAGASSERT(options != NULL);

	if ((retval = getopt_internal(nargc,nargv,options)) == -2) {
		++optind;

		if (nonopt_end != -1) {
			permute_args(nonopt_start,nonopt_end,optind,nargv);
			optind -= nonopt_end - nonopt_start;
		}
		nonopt_start = nonopt_end = -1;
		retval = -1;
	}
	return retval;
}
#endif

int
getopt_long(nargc,nargv,options,long_options,idx)
	int nargc;
	char * const *nargv;
	const char *options;
	const struct option *long_options;
	int *idx;
{
	int retval;

	_DIAGASSERT(nargv != NULL);
	_DIAGASSERT(options != NULL);
	_DIAGASSERT(long_options != NULL);

	if ((retval = getopt_internal(nargc,nargv,options)) == -2) {
		char *current_argv,*has_equal;
		size_t current_argv_len;
		int i,match;

		current_argv = place;
		match = -1;

		optind++;
		place = EMSG;

		if (*current_argv == '\0') {

			if (nonopt_end != -1) {
				permute_args(nonopt_start,nonopt_end,optind,nargv);
				optind -= nonopt_end - nonopt_start;
			}
			nonopt_start = nonopt_end = -1;
			return -1;
		}
		if ((has_equal = strchr(current_argv,'=')) != NULL) {

			current_argv_len = has_equal - current_argv;
			has_equal++;
		} else
			current_argv_len = strlen(current_argv);

		for (i = 0; long_options[i].name; i++) {

			if (strncmp(current_argv,long_options[i].name,current_argv_len))
				continue;

			if (strlen(long_options[i].name) ==
			    (unsigned)current_argv_len) {

				match = i;
				break;
			}
			if (match == -1)
				match = i;
			else {

				if (PRINT_ERROR)
					warnx(ambig,(int)current_argv_len,current_argv);
				optopt = 0;
				return BADCH;
			}
		}
		if (match != -1) {
			if (long_options[match].has_arg == no_argument
			    && has_equal) {
				if (PRINT_ERROR)
					warnx(noarg,(int)current_argv_len,current_argv);

				if (long_options[match].flag == NULL)
					optopt = long_options[match].val;
				else
					optopt = 0;
				return BADARG;
			}
			if (long_options[match].has_arg == required_argument ||
			    long_options[match].has_arg == optional_argument) {
				if (has_equal)
					optarg = has_equal;
				else if (long_options[match].has_arg ==
				    required_argument) {

					optarg = nargv[optind++];
				}
			}
			if ((long_options[match].has_arg == required_argument)
			    && (optarg == NULL)) {

				if (PRINT_ERROR)
					warnx(recargstring,current_argv);

				if (long_options[match].flag == NULL)
					optopt = long_options[match].val;
				else
					optopt = 0;
				--optind;
				return BADARG;
			}
		} else {
			if (PRINT_ERROR)
				warnx(illoptstring,current_argv);
			optopt = 0;
			return BADCH;
		}
		if (long_options[match].flag) {
			*long_options[match].flag = long_options[match].val;
			retval = 0;
		} else
			retval = long_options[match].val;
		if (idx)
			*idx = match;
	}
	return retval;
}
