<module name="kbdja" type="kernelmodedll" entrypoint="0" installbase="system32" installname="kbdja.dll" allowwarnings="true">
	<importlibrary definition="kbdja.def" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="__USE_W32API" />
	<define name="_WIN32_WINNT">0x0500</define>
	<file>kbdja.c</file>
	<file>kbdja.rc</file>
</module>
