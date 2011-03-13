<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="winhttp_winetest" type="win32cui" installbase="bin" installname="winhttp_winetest.exe" allowwarnings="true">
	<include base="winhttp_winetest">.</include>
    <define name="__ROS_LONG64__" />
	<file>notification.c</file>
	<file>testlist.c</file>
	<file>url.c</file>
	<file>winhttp.c</file>
	<library>wine</library>
	<library>winhttp</library>
	<library>crypt32</library>
	<library>advapi32</library>
	<library>ws2_32</library>
	<library>ntdll</library>
</module>
</group>
