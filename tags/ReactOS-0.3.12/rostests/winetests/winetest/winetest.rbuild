<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="winetest" type="win32gui" installbase="system32" allowwarnings="true">
	<include base="winetest">.</include>
    <define name="__ROS_LONG64__" />
	<library>comctl32</library>
	<library>version</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>advapi32</library>
	<library>ws2_32</library>
	<file>gui.c</file>
	<file>main.c</file>
	<file>send.c</file>
	<file>util.c</file>
	<file>dist.rc</file>
</module>
