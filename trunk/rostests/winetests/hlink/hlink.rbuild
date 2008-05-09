<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="hlink_winetest" type="win32cui" installbase="bin" installname="hlink_winetest.exe" allowwarnings="true">
	<include base="hlink_winetest">.</include>
	<define name="WINVER">0x600</define>
	<define name="_WIN32_WINNT">0x600</define>
	<file>hlink.c</file>
	<file>testlist.c</file>
	<library>wine</library>
	<library>hlink</library>
	<library>ole32</library>
	<library>kernel32</library>
	<library>uuid</library>
	<library>ntdll</library>
</module>
</group>
