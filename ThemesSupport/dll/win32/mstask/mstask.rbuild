<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="mstask" type="win32dll" baseaddress="${BASEADDRESS_MSTASK}" installbase="system32" installname="mstask.dll" allowwarnings="true">
	<autoregister infsection="OleControlDlls" type="DllRegisterServer" />
	<importlibrary definition="mstask.spec" />
	<include base="mstask">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<redefine name="WINVER">0x600</redefine>
	<redefine name="_WIN32_WINNT">0x600</redefine>
	<file>factory.c</file>
	<file>mstask_main.c</file>
	<file>task.c</file>
	<file>task_scheduler.c</file>
	<file>task_trigger.c</file>
	<file>rsrc.rc</file>
	<library>mstask_local_interface</library>
	<library>wine</library>
	<library>uuid</library>
	<library>ole32</library>
	<library>ntdll</library>
</module>
<module name="mstask_local_interface" type="idlinterface">
	<file>mstask_local.idl</file>
</module>
</group>
