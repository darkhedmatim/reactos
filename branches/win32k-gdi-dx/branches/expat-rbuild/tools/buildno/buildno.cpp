/* $Id$
 *
 * buildno - Generate the build number for ReactOS
 *
 * Copyright (c) 1999,2000 Emanuele Aliberti
 *
 * The build number is the day on which the build took
 * place, as YYYYMMDD
 *
 * The build number is stored in the file
 * <reactos/buildno.h> as a set of macros:
 *
 * KERNEL_VERSION_BUILD		base 10 number
 * KERNEL_VERSION_BUILD_STR	C string
 * KERNEL_VERSION_BUILD_RC	RC string
 *
 * REVISIONS
 * ---------
 * 2000-01-22 (ea)
 * 	Fixed bugs: tm_year is (current_year - 1900),
 * 	tm_month is 0-11 not 1-12 and code ignored TZ.
 * 2000-12-10 (ea)
 * 	Added -p option to make it simply print the
 * 	version number, but skip buildno.h generation.
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "version.h"

#include "xml.h"
using namespace XMLStorage;
using namespace std;

#define FALSE 0
#define TRUE  1

static char * argv0 = "";
static char * filename = "";

#ifdef DBG
void
tm_dump (const char *tag, struct tm * t)
{
	printf ("%s->tm_sec   = %d\n", tag, t->tm_sec);
	printf ("%s->tm_min   = %d\n", tag, t->tm_min);
	printf ("%s->tm_hour  = %d\n", tag, t->tm_hour);
	printf ("%s->tm_mday  = %d\n", tag, t->tm_mday);
	printf ("%s->tm_mon   = %d\n", tag, t->tm_mon);
	printf ("%s->tm_year  = %d\n", tag, t->tm_year);
	printf ("%s->tm_wday  = %d\n", tag, t->tm_wday);
	printf ("%s->tm_yday  = %d\n", tag, t->tm_yday);
	printf ("%s->tm_isdst = %d\n\n", tag, t->tm_isdst);
}
#endif

static int
write_h (int build, char *buildstr)
{
  FILE	*h = NULL;
  char* s;
  char* s1;
  unsigned int length;
  int dllversion = KERNEL_VERSION_MAJOR + 42;

  if (!filename || !*filename)
	return 1;

  s1 = s = (char *) malloc(256 * 1024);
  
  s = s + sprintf (s, "/* Do not edit - Machine generated */\n");
	
  s = s + sprintf (s, "#ifndef _INC_REACTOS_BUILDNO\n" );
  s = s + sprintf (s, "#define _INC_REACTOS_BUILDNO\n" );
  
  s = s + sprintf (s, "#define KERNEL_VERSION_BUILD\t%d\n", build);
  s = s + sprintf (s, "#define KERNEL_VERSION_BUILD_STR\t\"%s\"\n", buildstr);
  s = s + sprintf (s, "#define KERNEL_VERSION_BUILD_RC\t\"%s\\0\"\n", buildstr);
  s = s + sprintf (s, "#define KERNEL_RELEASE_RC\t\"%d.%d",
		   KERNEL_VERSION_MAJOR, KERNEL_VERSION_MINOR);
  if (0 != KERNEL_VERSION_PATCH_LEVEL)
    {
      s = s + sprintf (s, ".%d", KERNEL_VERSION_PATCH_LEVEL);
    }
  s = s + sprintf (s, "-%S\\0\"\n", KERNEL_VERSION_BUILD_TYPE);
  s = s + sprintf (s, "#define KERNEL_RELEASE_STR\t\"%d.%d",
		   KERNEL_VERSION_MAJOR,
		   KERNEL_VERSION_MINOR);
  if (0 != KERNEL_VERSION_PATCH_LEVEL)
    {
      s = s + sprintf (s, ".%d", KERNEL_VERSION_PATCH_LEVEL);
    }
  s = s + sprintf (s, "-%S\"\n", KERNEL_VERSION_BUILD_TYPE);
  s = s + sprintf (s, "#define KERNEL_VERSION_RC\t\"%d.%d",
		   KERNEL_VERSION_MAJOR,
		   KERNEL_VERSION_MINOR);
  if (0 != KERNEL_VERSION_PATCH_LEVEL)
    {
      s = s + sprintf (s, ".%d", KERNEL_VERSION_PATCH_LEVEL);
    }
  s = s + sprintf (s, "-%S\\0\"\n", KERNEL_VERSION_BUILD_TYPE);
  s = s + sprintf (s, "#define KERNEL_VERSION_STR\t\"%d.%d", 
		   KERNEL_VERSION_MAJOR,
		   KERNEL_VERSION_MINOR);
  if (0 != KERNEL_VERSION_PATCH_LEVEL)
    {
      s = s + sprintf (s, ".%d", KERNEL_VERSION_PATCH_LEVEL);
    }
  s = s + sprintf (s, "-%S\"\n", KERNEL_VERSION_BUILD_TYPE);
  s = s + sprintf (s, "#define REACTOS_DLL_VERSION_MAJOR\t%d\n", dllversion);
  s = s + sprintf (s, "#define REACTOS_DLL_RELEASE_RC\t\"%d.%d",
		   dllversion, KERNEL_VERSION_MINOR);
  if (0 != KERNEL_VERSION_PATCH_LEVEL)
    {
      s = s + sprintf (s, ".%d", KERNEL_VERSION_PATCH_LEVEL);
    }
  s = s + sprintf (s, "-%S\\0\"\n", KERNEL_VERSION_BUILD_TYPE);
  s = s + sprintf (s, "#define REACTOS_DLL_RELEASE_STR\t\"%d.%d",
		   dllversion,
		   KERNEL_VERSION_MINOR);
  if (0 != KERNEL_VERSION_PATCH_LEVEL)
    {
      s = s + sprintf (s, ".%d", KERNEL_VERSION_PATCH_LEVEL);
    }
  s = s + sprintf (s, "-%S\"\n", KERNEL_VERSION_BUILD_TYPE);
  s = s + sprintf (s, "#define REACTOS_DLL_VERSION_RC\t\"%d.%d",
		   dllversion,
		   KERNEL_VERSION_MINOR);
  if (0 != KERNEL_VERSION_PATCH_LEVEL)
    {
      s = s + sprintf (s, ".%d", KERNEL_VERSION_PATCH_LEVEL);
    }
  s = s + sprintf (s, "-%S\\0\"\n", KERNEL_VERSION_BUILD_TYPE);
  s = s + sprintf (s, "#define REACTOS_DLL_VERSION_STR\t\"%d.%d", 
		   dllversion,
		   KERNEL_VERSION_MINOR);
  if (0 != KERNEL_VERSION_PATCH_LEVEL)
    {
      s = s + sprintf (s, ".%d", KERNEL_VERSION_PATCH_LEVEL);
    }
  s = s + sprintf (s, "-%S\"\n", KERNEL_VERSION_BUILD_TYPE);
  s = s + sprintf (s, "#endif\n/* EOF */\n");

  h = fopen (filename, "wb");
  if (h != NULL)
    {
      fseek(h, 0, SEEK_END);
      length = ftell(h);
      if (length == strlen(s1))
	{
	  char* orig;
	  
	  orig = (char *) malloc(length);
	  fseek(h, 0, SEEK_SET);
	  fread(orig, 1, length, h);
	  if (memcmp(s1, orig, length) == 0)
	    {
	      fclose(h);
	      return 0;
	    }
	}
      fclose(h);
    }

  h = fopen (filename, "wb");
  if (!h) 
    {
      fprintf (stderr,
	       "%s: can not create file \"%s\"!\n",
	       argv0,
	       filename);
      return -1;
    }
  fwrite(s1, 1, strlen(s1), h);
  fclose (h);

  return 0;
}

static string
GetRev(void)
{
  try {
	XMLDoc doc;

	if (!doc.read(".svn/entries"))
		doc.read("_svn/entries");

	XMLPos pos(&doc);

	 // enter the "wc-entries" root element
	if (pos.go_down("wc-entries")) {
		 // enumerate all "entry" children elements
		const_XMLChildrenFilter entries(pos, "entry");

		for(const_XMLChildrenFilter::const_iterator it=entries.begin(); it!=entries.end(); ++it) {
			const XMLNode& entry = **it;

			string kind = entry.get("kind");
			string name = entry.get("name");
			string revision = entry.get("revision");

			if (kind=="dir" && name=="" && !revision.empty())
				return revision;
		}

		pos.back();
	}
  }
  catch(exception& e)
	{
	  fprintf(stderr, "Exception: %s\n", e.what());
	}

  return "UNKNOWN";
}


void
usage (void)
{
	fprintf (
		stderr,
		"Usage: %s [-{p|q}] path-to-header\n\n  -p  print version number and exit\n  -q  run in quiet mode\n",
		argv0
		);
	exit (EXIT_SUCCESS);
}


int
main (int argc, char * argv [])
{
	int		print_only = FALSE;
	int		quiet = FALSE;

	int		build = 0;
	char		buildstr[64];

	time_t		t1 = 0;
	struct tm	* t1_tm = NULL;

	argv0 = argv[0];
		
	switch (argc)
	{
		case 1:
			break;
		case 2:
		case 3:
			if (argv[1][0] == '-')
			{
				if (argv[1][1] == 'q')
				{
					quiet = TRUE;
				}
				else  if (argv[1][1] == 'p')
				{
					print_only = TRUE;
				}
				else
				{
					usage ();
				}
				filename = argv[2];
			}
			else if (argc == 2)
			{
				filename = argv[1];
			}
			else
			{
				usage ();
			}
			break;
		default:
			usage ();
	}
	/*
	 * Set TZ information.
	 */
	tzset ();
	/*
	 * We are building TODAY!
	 */
	if (! quiet)
	{
		printf ( "\nReactOS Build Number Generator\n\n");
	}

	time (& t1); /* current build time */
	t1_tm = gmtime (& t1);

#ifdef DBG
	tm_dump ("t1", t1_tm);
#endif
	t1_tm->tm_year += 1900;
#ifdef DBG
	tm_dump ("t1", t1_tm);
#endif
	if (! quiet)
	{
		printf ( 
			"Current date: %4d-%02d-%02d\n\n",
			t1_tm->tm_year,
			(t1_tm->tm_mon + 1),
			t1_tm->tm_mday
			);
	}
	/*
	 * Compute build number.
	 */
	build =	t1_tm->tm_year * 10000 + (t1_tm->tm_mon + 1) * 100 + t1_tm->tm_mday;

	sprintf(buildstr, "%d-r%s", build, GetRev().c_str());

	if (! quiet)
	{
		printf (
			"ROS Version : %d.%d",
			KERNEL_VERSION_MAJOR,
			KERNEL_VERSION_MINOR
			);
		if (0 != KERNEL_VERSION_PATCH_LEVEL)
		{
			printf(".%d", KERNEL_VERSION_PATCH_LEVEL);
		}
		printf("-%S (Build %s)\n\n", KERNEL_VERSION_BUILD_TYPE, buildstr);
	}
	/*
	 * (Over)write the include file, unless
	 * the user switched on -p.
	 */
	if (! print_only)
	{
		if (0 != write_h (build, buildstr))
		{
			exit(EXIT_FAILURE);
		}
	}
	else
	{
		printf ("%s: no code generated", argv [0]);
	}

	return EXIT_SUCCESS;
}


/* EOF */
