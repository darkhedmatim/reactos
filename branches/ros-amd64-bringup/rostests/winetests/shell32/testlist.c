/* Automatically generated file; DO NOT EDIT!! */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define STANDALONE
#include "wine/test.h"

extern void func_appbar(void);
extern void func_autocomplete(void);
extern void func_generated(void);
extern void func_progman_dde(void);
extern void func_shelllink(void);
extern void func_shellpath(void);
extern void func_shfldr_netplaces(void);
extern void func_shfldr_special(void);
extern void func_shlexec(void);
extern void func_shlfileop(void);
extern void func_shlfolder(void);
extern void func_string(void);
extern void func_systray(void);

const struct test winetest_testlist[] =
{
    { "appbar", func_appbar },  
	{ "autocomplete", func_autocomplete },
    { "generated", func_generated },
	{ "progman_dde", func_progman_dde },
    { "shelllink", func_shelllink },
    { "shellpath", func_shellpath },
    { "shfldr_netplaces", func_shfldr_netplaces },
	{ "shfldr_special", func_shfldr_special },
    { "shlexec", func_shlexec },
    { "shlfileop", func_shlfileop },
    { "shlfolder", func_shlfolder },
    { "string", func_string },
    { "systray", func_systray },
    { 0, 0 }
};
