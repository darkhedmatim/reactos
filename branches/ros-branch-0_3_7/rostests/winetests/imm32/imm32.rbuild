<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="imm32_winetest" type="win32cui" installbase="bin" installname="imm32_winetest.exe" allowwarnings="true">
	<include base="imm32_winetest">.</include>
	<define name="WINVER">0x600</define>
	<define name="_WIN32_WINNT">0x600</define>
	<file>imm32.c</file>
	<file>testlist.c</file>
	<library>wine</library>
	<library>imm32</library>
	<library>user32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
</group>
