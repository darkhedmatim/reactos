<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="riched32_winetest" type="win32cui" installbase="bin" installname="riched32_winetest.exe" allowwarnings="true" entrypoint="0">
	<include base="riched32_winetest">.</include>
	<define name="WINVER">0x600</define>
	<define name="_WIN32_WINNT">0x600</define>
	<file>editor.c</file>
	<file>testlist.c</file>
	<library>wine</library>
	<library>ole32</library>
	<library>user32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
</group>
