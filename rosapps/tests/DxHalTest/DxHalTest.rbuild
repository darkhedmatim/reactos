<module name="dxhaltest" type="win32cui" installbase="bin" installname="dxhaltest.exe">
	<define name="_WIN32_IE">0x0501</define>
	<define name="_WIN32_WINNT">0x0501</define>
	<define name="__USE_W32API" />
	<library>kernel32</library>
	<library>gdi32</library>
	<file>main.c</file>
</module>
