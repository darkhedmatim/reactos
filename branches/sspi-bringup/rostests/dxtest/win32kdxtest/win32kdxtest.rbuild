<module name="win32kdxtest" type="win32cui" installbase="bin" installname="win32kdxtest.exe">
	<include base="win32kdxtest">.</include>
	<library>user32</library>
	<library>gdi32</library>
	<library>d3d8thk</library>
	<file>main.c</file>
	<file>NtGdiDdCreateDirectDrawObject.c</file>
	<file>NtGdiDdDeleteDirectDrawObject.c</file>
	<file>NtGdiDdQueryDirectDrawObject.c</file>
	<file>NtGdiDdGetScanLine.c</file>
	<file>NtGdiDdWaitForVerticalBlank.c</file>
	<file>NtGdiDdCanCreateSurface.c</file>
	<file>dump.c</file>
</module>