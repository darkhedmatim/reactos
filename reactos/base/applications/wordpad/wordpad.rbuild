<module name="wordpad" type="win32gui" installbase="system32" installname="wordpad.exe">
	<include base="wordpad">.</include>
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x0501</define>
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>comdlg32</library>
	<library>advapi32</library>
	<library>shell32</library>
	<library>comctl32</library>
	<file>wordpad.c</file>
	<file>rsrc.rc</file>
</module>
