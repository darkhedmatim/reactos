<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdpl1" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdpl1.dll">
	<importlibrary definition="kbdpl1.spec" />
	<include base="ntoskrnl">include</include>
	<file>kbdpl1.c</file>
	<file>kbdpl1.rc</file>
</module>
