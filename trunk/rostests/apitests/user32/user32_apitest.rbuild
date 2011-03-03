<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="user32_apitest" type="win32cui" installbase="bin" installname="user32_apitest.exe">
	<include base="user32_apitest">.</include>
	<library>wine</library>
	<library>gdi32</library>
	<library>user32</library>
	<library>pseh</library>
	<file>testlist.c</file>
	<file>user32_apitest.rc</file>

	<file>InitializeLpkHooks.c</file>
	<file>RealGetWindowClass.c</file>
	<file>ScrollDC.c</file>
	<file>ScrollWindowEx.c</file>
	<file>GetSystemMetrics.c</file>
	<file>GetIconInfo.c</file>
	<file>GetPeekMessage.c</file>

</module>
</group>
