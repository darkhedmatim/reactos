<module name="gdi32api" type="win32cui" allowwarnings="true">
	<include base="gdi32api">.</include>
	<library>apitest</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>shell32</library>
	<file>gdi32api.c</file>
	<file>testlist.c</file>
</module>
