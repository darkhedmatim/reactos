<module name="shaptest" type="win32gui" installbase="bin" installname="shaptest.exe">
	<define name="__USE_W32API" />
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<file>shaptest.c</file>
</module>
