<module name="gettype" type="win32cui" installbase="system32" installname="gettype.exe" unicode="yes">
	<include base="gettype">.</include>
	<define name="_WIN32_WINNT">0x0501</define>
	<library>kernel32</library>
	<library>shell32</library>
	<library>mpr</library>
	<library>netapi32</library>
	<file>gettype.c</file>
</module>
