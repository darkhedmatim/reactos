<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="gdi32_apitest" type="win32cui" installbase="bin" installname="gdi32_apitest.exe">
	<include base="gdi32_apitest">.</include>
	<library>wine</library>
	<library>gdi32</library>
	<library>user32</library>
	<library>pseh</library>
	<file>testlist.c</file>

	<file>AddFontResource.c</file>
	<file>AddFontResourceEx.c</file>
	<file>BeginPath.c</file>
	<file>CombineTransform.c</file>
	<file>CreateBitmapIndirect.c</file>
	<file>CreateCompatibleDC.c</file>
	<file>CreateFont.c</file>
	<file>CreateFontIndirect.c</file>
	<file>CreatePen.c</file>
	<file>CreateRectRgn.c</file>
	<file>EngAcquireSemaphore.c</file>
	<file>EngCreateSemaphore.c</file>
	<file>EngDeleteSemaphore.c</file>
	<file>EngReleaseSemaphore.c</file>
	<file>ExtCreatePen.c</file>
	<file>GdiConvertBitmap.c</file>
	<file>GdiConvertBrush.c</file>
	<file>GdiConvertDC.c</file>
	<file>GdiConvertFont.c</file>
	<file>GdiConvertPalette.c</file>
	<file>GdiConvertRegion.c</file>
	<file>GdiDeleteLocalDC.c</file>
	<file>GdiGetCharDimensions.c</file>
	<file>GdiGetLocalBrush.c</file>
	<file>GdiGetLocalDC.c</file>
	<file>GdiReleaseLocalDC.c</file>
	<file>GdiSetAttrs.c</file>
	<file>GetClipRgn.c</file>
	<file>GetCurrentObject.c</file>
	<file>GetDIBits.c</file>
	<file>GetObject.c</file>
	<file>GetStockObject.c</file>
	<file>GetTextExtentExPoint.c</file>
	<file>GetTextFace.c</file>
	<file>MaskBlt.c</file>
	<file>SelectObject.c</file>
	<file>SetDCPenColor.c</file>
	<file>SetMapMode.c</file>
	<file>SetSysColors.c</file>
	<file>SetWindowExtEx.c</file>
	<file>SetWorldTransform.c</file>
</module>
</group>
