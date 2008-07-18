<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="reactos" type="win32gui">
	<bootstrap installbase="$(CDOUTPUT)" />
	<include base="reactos">.</include>
	<define name="_WIN32_IE">0x0501</define>
	<define name="_WIN32_WINNT">0x0501</define>
	<library>kernel32</library>
	<library>gdi32</library>
	<library>user32</library>
	<library>comctl32</library>
	<library>setupapi</library>
	<file>reactos.c</file>
	<file>reactos.rc</file>
</module>
