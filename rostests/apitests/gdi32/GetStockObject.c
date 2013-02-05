/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for GetStockObject
 * PROGRAMMERS:     Timo Kreuzer
 */

#include <wine/test.h>
#include <wingdi.h>
#include <winddi.h>
#include <include/ntgdityp.h>
#include <include/ntgdihdl.h>

#define TEST(x) ok(x, #x"\n")
#define RTEST(x) ok(x, #x"\n")

void Test_GetStockObject()
{
	/* Test limits and error */
	SetLastError(ERROR_SUCCESS);
	RTEST(GetStockObject(0) != NULL);
	TEST(GetStockObject(20) != NULL);
	RTEST(GetStockObject(21) != NULL);
	RTEST(GetStockObject(-1) == NULL);
	RTEST(GetStockObject(9) == NULL);
	RTEST(GetStockObject(22) == NULL);
	RTEST(GetLastError() == ERROR_SUCCESS);

	/* Test for the stock bit */
	RTEST((UINT_PTR)GetStockObject(WHITE_BRUSH) && GDI_HANDLE_STOCK_MASK);

	/* Test for correct types */
	RTEST(GDI_HANDLE_GET_TYPE(GetStockObject(WHITE_BRUSH)) == GDI_OBJECT_TYPE_BRUSH); /* 0 */
	RTEST(GDI_HANDLE_GET_TYPE(GetStockObject(LTGRAY_BRUSH)) == GDI_OBJECT_TYPE_BRUSH); /* 1 */
	RTEST(GDI_HANDLE_GET_TYPE(GetStockObject(GRAY_BRUSH)) == GDI_OBJECT_TYPE_BRUSH); /* 1 */
	RTEST(GDI_HANDLE_GET_TYPE(GetStockObject(DKGRAY_BRUSH)) == GDI_OBJECT_TYPE_BRUSH); /* 1 */
	RTEST(GDI_HANDLE_GET_TYPE(GetStockObject(BLACK_BRUSH)) == GDI_OBJECT_TYPE_BRUSH); /* 1 */
	RTEST(GDI_HANDLE_GET_TYPE(GetStockObject(NULL_BRUSH)) == GDI_OBJECT_TYPE_BRUSH); /* 1 */
	RTEST(GDI_HANDLE_GET_TYPE(GetStockObject(WHITE_PEN)) == GDI_OBJECT_TYPE_PEN); /* 6 */
	RTEST(GDI_HANDLE_GET_TYPE(GetStockObject(BLACK_PEN)) == GDI_OBJECT_TYPE_PEN); /* 7 */
	RTEST(GDI_HANDLE_GET_TYPE(GetStockObject(NULL_PEN)) == GDI_OBJECT_TYPE_PEN); /* 8 */
	RTEST(GDI_HANDLE_GET_TYPE(GetStockObject(OEM_FIXED_FONT)) == GDI_OBJECT_TYPE_FONT); /* 10 */
	RTEST(GDI_HANDLE_GET_TYPE(GetStockObject(ANSI_FIXED_FONT)) == GDI_OBJECT_TYPE_FONT); /* 11 */
	TEST(GDI_HANDLE_GET_TYPE(GetStockObject(ANSI_VAR_FONT)) == GDI_OBJECT_TYPE_FONT); /* 12 */
	RTEST(GDI_HANDLE_GET_TYPE(GetStockObject(SYSTEM_FONT)) == GDI_OBJECT_TYPE_FONT); /* 13 */
	RTEST(GDI_HANDLE_GET_TYPE(GetStockObject(DEVICE_DEFAULT_FONT)) == GDI_OBJECT_TYPE_FONT); /* 14 */
	RTEST(GDI_HANDLE_GET_TYPE(GetStockObject(DEFAULT_PALETTE)) == GDI_OBJECT_TYPE_PALETTE); /* 15 */
	RTEST(GDI_HANDLE_GET_TYPE(GetStockObject(SYSTEM_FIXED_FONT)) == GDI_OBJECT_TYPE_FONT); /* 16 */
	RTEST(GDI_HANDLE_GET_TYPE(GetStockObject(DEFAULT_GUI_FONT)) == GDI_OBJECT_TYPE_FONT); /* 17 */
	RTEST(GDI_HANDLE_GET_TYPE(GetStockObject(DC_BRUSH)) == GDI_OBJECT_TYPE_BRUSH); /* 18 */
	RTEST(GDI_HANDLE_GET_TYPE(GetStockObject(DC_PEN)) == GDI_OBJECT_TYPE_PEN); /* 19 */
	TEST(GDI_HANDLE_GET_TYPE(GetStockObject(20)) == GDI_OBJECT_TYPE_COLORSPACE); /* 20 */
	RTEST(GDI_HANDLE_GET_TYPE(GetStockObject(21)) == GDI_OBJECT_TYPE_BITMAP); /* 21 */
}

START_TEST(GetStockObject)
{
    Test_GetStockObject();
}

