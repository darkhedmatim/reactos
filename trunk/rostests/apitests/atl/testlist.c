#define STANDALONE
#include <apitest.h>

extern void func_atltypes(void);
extern void func_CComBSTR(void);
extern void func_CComHeapPtr(void);
extern void func_CImage(void);
extern void func_CRegKey(void);
extern void func_CSimpleArray(void);
extern void func_CSimpleMap(void);
extern void func_CString(void);

const struct test winetest_testlist[] =
{
    { "atltypes", func_atltypes },
    { "CComBSTR", func_CComBSTR },
    { "CComHeapPtr", func_CComHeapPtr },
    { "CImage", func_CImage },
    { "CRegKey", func_CRegKey },
    { "CSimpleArray", func_CSimpleArray },
    { "CSimpleMap", func_CSimpleMap },
    { "CString", func_CString },
    { 0, 0 }
};
