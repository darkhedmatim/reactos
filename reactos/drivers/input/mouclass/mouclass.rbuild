<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="mouclass" type="kernelmodedriver" installbase="system32/drivers" installname="mouclass.sys">
	<include base="mouclass">.</include>
	<library>pseh</library>
	<library>ntoskrnl</library>
	<library>hal</library>
	<file>misc.c</file>
	<file>mouclass.c</file>
	<file>mouclass.rc</file>
	<pch>mouclass.h</pch>
</module>
