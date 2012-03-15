
#ifndef __DIB_FUNCTION_NAME
#define __DIB_FUNCTION_NAME __DIB_FUNCTION_NAME_SRCDST
#endif

#define _SOURCE_BPP 1
#include "diblib_alldstbpp.h"
#undef _SOURCE_BPP

#define _SOURCE_BPP 4
#include "diblib_alldstbpp.h"
#undef _SOURCE_BPP

#define _SOURCE_BPP 8
#include "diblib_alldstbpp.h"
#undef _SOURCE_BPP

#define _SOURCE_BPP 16
#include "diblib_alldstbpp.h"
#undef _SOURCE_BPP

#define _SOURCE_BPP 24
#include "diblib_alldstbpp.h"
#undef _SOURCE_BPP

#define _SOURCE_BPP 32
#include "diblib_alldstbpp.h"
#undef _SOURCE_BPP

#undef _DibXlate
#define _DibXlate(pBltData, ulColor) (ulColor)
#define _SOURCE_BPP _DEST_BPP
#define _NextPixel_ _NextPixelR2L_

#undef __DIB_FUNCTION_NAME
#define __DIB_FUNCTION_NAME __DIB_FUNCTION_NAME_SRCDSTEQ
#include "diblib_alldstbpp.h"

#undef __DIB_FUNCTION_NAME
#define __DIB_FUNCTION_NAME __DIB_FUNCTION_NAME_SRCDSTEQR2L
#include "diblib_alldstbpp.h"
#undef _SOURCE_BPP

PFN_DIBFUNCTION
__PASTE(gapfn, __FUNCTIONNAME)[7][7] =
{
    {0, 0, 0, 0, 0, 0},
    {
        __DIB_FUNCTION_NAME_SRCDSTEQ(__FUNCTIONNAME, 1, 1),
        __DIB_FUNCTION_NAME_SRCDST(__FUNCTIONNAME, 1, 1),
        __DIB_FUNCTION_NAME_SRCDST(__FUNCTIONNAME, 4, 1),
        __DIB_FUNCTION_NAME_SRCDST(__FUNCTIONNAME, 8, 1),
        __DIB_FUNCTION_NAME_SRCDST(__FUNCTIONNAME, 16, 1),
        __DIB_FUNCTION_NAME_SRCDST(__FUNCTIONNAME, 24, 1),
        __DIB_FUNCTION_NAME_SRCDST(__FUNCTIONNAME, 32, 1),
    },
    {
        __DIB_FUNCTION_NAME_SRCDSTEQ(__FUNCTIONNAME, 4, 4),
        __DIB_FUNCTION_NAME_SRCDST(__FUNCTIONNAME, 1, 4),
        __DIB_FUNCTION_NAME_SRCDST(__FUNCTIONNAME, 4, 4),
        __DIB_FUNCTION_NAME_SRCDST(__FUNCTIONNAME, 8, 4),
        __DIB_FUNCTION_NAME_SRCDST(__FUNCTIONNAME, 16, 4),
        __DIB_FUNCTION_NAME_SRCDST(__FUNCTIONNAME, 24, 4),
        __DIB_FUNCTION_NAME_SRCDST(__FUNCTIONNAME, 32, 4),
    },
    {
        __DIB_FUNCTION_NAME_SRCDSTEQ(__FUNCTIONNAME, 8, 8),
        __DIB_FUNCTION_NAME_SRCDST(__FUNCTIONNAME, 1, 8),
        __DIB_FUNCTION_NAME_SRCDST(__FUNCTIONNAME, 4, 8),
        __DIB_FUNCTION_NAME_SRCDST(__FUNCTIONNAME, 8, 8),
        __DIB_FUNCTION_NAME_SRCDST(__FUNCTIONNAME, 16, 8),
        __DIB_FUNCTION_NAME_SRCDST(__FUNCTIONNAME, 24, 8),
        __DIB_FUNCTION_NAME_SRCDST(__FUNCTIONNAME, 32, 8),
    },
    {
        __DIB_FUNCTION_NAME_SRCDSTEQ(__FUNCTIONNAME, 16, 16),
        __DIB_FUNCTION_NAME_SRCDST(__FUNCTIONNAME, 1, 16),
        __DIB_FUNCTION_NAME_SRCDST(__FUNCTIONNAME, 4, 16),
        __DIB_FUNCTION_NAME_SRCDST(__FUNCTIONNAME, 8, 16),
        __DIB_FUNCTION_NAME_SRCDST(__FUNCTIONNAME, 16, 16),
        __DIB_FUNCTION_NAME_SRCDST(__FUNCTIONNAME, 24, 16),
        __DIB_FUNCTION_NAME_SRCDST(__FUNCTIONNAME, 32, 16),
    },
    {
        __DIB_FUNCTION_NAME_SRCDSTEQ(__FUNCTIONNAME, 24, 24),
        __DIB_FUNCTION_NAME_SRCDST(__FUNCTIONNAME, 1, 24),
        __DIB_FUNCTION_NAME_SRCDST(__FUNCTIONNAME, 4, 24),
        __DIB_FUNCTION_NAME_SRCDST(__FUNCTIONNAME, 8, 24),
        __DIB_FUNCTION_NAME_SRCDST(__FUNCTIONNAME, 16, 24),
        __DIB_FUNCTION_NAME_SRCDST(__FUNCTIONNAME, 24, 24),
        __DIB_FUNCTION_NAME_SRCDST(__FUNCTIONNAME, 32, 24),
    },
    {
        __DIB_FUNCTION_NAME_SRCDSTEQ(__FUNCTIONNAME, 32, 32),
        __DIB_FUNCTION_NAME_SRCDST(__FUNCTIONNAME, 1, 32),
        __DIB_FUNCTION_NAME_SRCDST(__FUNCTIONNAME, 4, 32),
        __DIB_FUNCTION_NAME_SRCDST(__FUNCTIONNAME, 8, 32),
        __DIB_FUNCTION_NAME_SRCDST(__FUNCTIONNAME, 16, 32),
        __DIB_FUNCTION_NAME_SRCDST(__FUNCTIONNAME, 24, 32),
        __DIB_FUNCTION_NAME_SRCDST(__FUNCTIONNAME, 32, 32),
    },
};

#undef __DIB_FUNCTION_NAME

