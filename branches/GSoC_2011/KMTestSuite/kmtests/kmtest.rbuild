<module name="kmtest" type="win32cui" installbase="system32" installname="kmtest.exe">
	<include base="kmtest">include</include>
	<library>advapi32</library>
	<define name="KMT_USER_MODE" />
	<directory name="kmtest">
		<file>kmtest.c</file>
		<file>service.c</file>
	</directory>
</module>
