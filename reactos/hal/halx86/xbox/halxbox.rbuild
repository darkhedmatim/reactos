<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="halxbox" type="kernelmodedll" entrypoint="0" allowwarnings="true">
	<importlibrary definition="../../hal/hal.def" />
	<include base="hal_generic">../include</include>
	<include base="halxbox">.</include>
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="UP" />
	<define name="_NTHAL_" />
	<library>hal_generic</library>
	<library>hal_generic_up</library>
	<library>ntoskrnl</library>
	<file>halinit_xbox.c</file>
	<file>part_xbox.c</file>
	<file>pci_xbox.c</file>
	<file>halxbox.rc</file>
	<pch>halxbox.h</pch>
</module>
