<module name="kbdse" type="kernelmodedll" entrypoint="0" installbase="system32" installname="kbdse.dll" allowwarnings="true">
	<importlibrary definition="kbdse.def" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="__USE_W32API" />
	<define name="_WIN32_WINNT">0x0500</define>
	<file>kbdse.c</file>
	<file>kbdse.rc</file>
</module>
