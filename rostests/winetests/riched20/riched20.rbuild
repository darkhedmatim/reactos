<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="riched20_winetest" type="win32cui" installbase="bin" installname="riched20_winetest.exe" allowwarnings="true">
	<include base="riched20_winetest">.</include>
	<define name="WINVER">0x600</define>
	<define name="_WIN32_WINNT">0x600</define>
	<file>editor.c</file>
	<file>testlist.c</file>
	<library>wine</library>
	<library>ole32</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
</group>
