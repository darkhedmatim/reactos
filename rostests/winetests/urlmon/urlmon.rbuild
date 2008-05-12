<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="urlmon_winetest" type="win32cui" installbase="bin" installname="urlmon_winetest.exe" allowwarnings="true">
	<include base="urlmon_winetest">.</include>
	<define name="WINVER">0x600</define>
	<define name="_WIN32_WINNT">0x600</define>
	<file>generated.c</file>
	<file>misc.c</file>
	<file>protocol.c</file>
	<file>stream.c</file>
	<file>url.c</file>
	<file>testlist.c</file>
	<library>wine</library>
	<library>urlmon</library>
	<library>ole32</library>
	<library>user32</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>uuid</library>
	<library>ntdll</library>
</module>
</group>
