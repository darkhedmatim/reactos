<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="mstask_winetest" type="win32cui" installbase="bin" installname="mstask_winetest.exe" allowwarnings="true">
	<include base="mstask_winetest">.</include>
    <define name="__ROS_LONG64__" />
	<file>task.c</file>
	<file>task_scheduler.c</file>
	<file>task_trigger.c</file>
	<file>testlist.c</file>
	<library>wine</library>
	<library>ole32</library>
	<library>ntdll</library>
</module>
</group>
