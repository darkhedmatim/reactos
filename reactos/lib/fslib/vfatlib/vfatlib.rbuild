<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="vfatlib" type="staticlibrary">
	<include base="vfatlib">.</include>
	<define name="_DISABLE_TIDENTS" />
	<file>fat12.c</file>
	<file>fat16.c</file>
	<file>fat32.c</file>
	<file>vfatlib.c</file>
</module>
