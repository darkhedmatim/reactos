INT
Test_SetDCPenColor(PTESTINFO pti)
{
	HDC hScreenDC, hDC;

	// Test an incorrect DC
	SetLastError(ERROR_SUCCESS);
	RTEST(SetDCPenColor(0, RGB(0,0,0)) == CLR_INVALID);
	TEST(GetLastError() == ERROR_INVALID_PARAMETER);

	// Get the Screen DC
	hScreenDC = GetDC(NULL);
	if (hScreenDC == NULL) return FALSE;

	// Test the screen DC
	SetDCPenColor(hScreenDC, RGB(1,2,3));
	TEST(SetDCPenColor(hScreenDC, RGB(4,5,6)) == RGB(1,2,3));

	// Create a new DC
	hDC = CreateCompatibleDC(hScreenDC);
	ReleaseDC(0, hScreenDC);
	if (hDC == NULL)
	{
		return FALSE;
	}

	// Select the DC_PEN and check if the pen returned by a new call is DC_PEN
	SelectObject(hDC, GetStockObject(DC_PEN));
	TEST(SelectObject(hDC, GetStockObject(BLACK_PEN)) == GetStockObject(DC_PEN));

	// Test an incorrect color, yes windows sets the color!
	SetDCPenColor(hDC, 0x21123456);
	TEST(SetDCPenColor(hDC, RGB(0,0,0)) == 0x21123456);

	// Test CLR_INVALID, it sets CLR_INVALID!
	SetDCPenColor(hDC, CLR_INVALID);
	RTEST(SetDCPenColor(hDC, RGB(0,0,0)) == CLR_INVALID);

	// Delete the DC
	DeleteDC(hDC);

	return APISTATUS_NORMAL;
}
