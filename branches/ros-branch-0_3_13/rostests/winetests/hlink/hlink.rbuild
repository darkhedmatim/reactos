<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="hlink_winetest" type="win32cui" installbase="bin" installname="hlink_winetest.exe" allowwarnings="true">
	<include base="hlink_winetest">.</include>
	<define name="__ROS_LONG64__" />
	<file>browse_ctx.c</file>
	<file>hlink.c</file>
	<file>testlist.c</file>
	<library>wine</library>
	<library>hlink</library>
	<library>ole32</library>
	<library>uuid</library>
	<library>ntdll</library>
</module>
</group>
