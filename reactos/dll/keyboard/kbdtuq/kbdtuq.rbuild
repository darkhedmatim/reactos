<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdtuq" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdtuq.dll">
	<importlibrary definition="kbdtuq.spec" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<file>kbdtuq.c</file>
	<file>kbdtuq.rc</file>
</module>
