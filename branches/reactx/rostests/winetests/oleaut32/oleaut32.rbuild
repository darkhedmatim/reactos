<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="oleaut32_winetest" type="win32cui" installbase="bin" installname="oleaut32_winetest.exe" allowwarnings="true">
	<include base="oleaut32_winetest">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<include base="oleaut32_winetest" root="intermediate">.</include>
	<define name="__ROS_LONG64__" />
	<library>wine</library>
	<library>oleaut32</library>
	<library>ole32</library>
	<library>rpcrt4</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>advapi32</library>
	<library>uuid</library>
	<library>ntdll</library>
	<library>tmarshal_interface</library>
	<file>dispatch.c</file>
	<file>olefont.c</file>
	<file>olepicture.c</file>
	<file>safearray.c</file>
	<file>tmarshal.c</file>
	<file>typelib.c</file>
	<file>usrmarshal.c</file>
	<file>varformat.c</file>
	<file>vartest.c</file>
	<file>vartype.c</file>
	<file>tmarshal.rc</file>
	<file>testlist.c</file>
	<dependency>tmarshal_header</dependency>
	<dependency>tmarshal</dependency>
	<dependency>test_tlb</dependency>
	<dependency>test_reg_header</dependency>
	<dependency>test_reg</dependency>
</module>
<module name="tmarshal_header" type="idlheader">
	<dependency>stdole2</dependency>
	<file>tmarshal.idl</file>
</module>
<module name="tmarshal_interface" type="idlinterface">
	<dependency>stdole2</dependency>
	<file>tmarshal.idl</file>
</module>
<module name="test_reg_header" type="idlheader" allowwarnings="true">
	<dependency>stdole2</dependency>
	<file>test_reg.idl</file>
</module>
<module name="test_reg" type="embeddedtypelib" allowwarnings="true">
	<dependency>stdole2</dependency>
	<file>test_reg.idl</file>
</module>
<module name="test_tlb" type="embeddedtypelib" allowwarnings="true">
	<dependency>stdole2</dependency>
	<file>test_tlb.idl</file>
</module>
<module name="tmarshal" type="embeddedtypelib" allowwarnings="true">
	<dependency>stdole2</dependency>
	<file>tmarshal.idl</file>
</module>
</group>
