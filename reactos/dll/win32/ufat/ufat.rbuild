<module name="ufat" type="win32dll" baseaddress="${BASEADDRESS_UFAT}" installbase="system32" installname="ufat.dll">
	<importlibrary definition="ufat.spec" />
	<include base="ufat">.</include>
	<library>vfatlib</library>
	<library>ntdll</library>
	<file>ufat.c</file>
	<file>ufat.rc</file>
</module>
