<module name="serialui_winetest" type="win32cui" installbase="bin" installname="serialui_winetest.exe" allowwarnings="true">
	<include base="serialui_winetest">.</include>
	<define name="__ROS_LONG64__" />
	<file>confdlg.c</file>
	<file>testlist.c</file>
	<library>wine</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
