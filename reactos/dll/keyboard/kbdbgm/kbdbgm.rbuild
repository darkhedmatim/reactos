<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdbgm" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdbgm.dll" allowwarnings="true">
	<importlibrary definition="kbdbgm.spec" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="_WIN32_WINNT">0x0500</define>
	<file>kbdbgm.c</file>
	<file>kbdbgm.rc</file>
</module>
