<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdvntc" type="kernelmodedll" entrypoint="0" installbase="system32" installname="kbdvntc.dll" allowwarnings="true">
	<importlibrary definition="kbdvntc.def" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="_WIN32_WINNT">0x0500</define>
	<file>kbdvntc.c</file>
	<file>kbdvntc.rc</file>
</module>
