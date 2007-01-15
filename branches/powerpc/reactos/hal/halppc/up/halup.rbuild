<module name="halppc_up" type="kernelmodedll">
	<importlibrary definition="../../hal/hal.def" />
	<bootstrap base="reactos" nameoncd="hal.dll" />
	<include base="hal_generic">../include</include>
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="__USE_W32API" />
	<define name="_NTHAL_" />
	<library>halppc_generic</library>
	<library>string</library>
	<library>ntoskrnl</library>
	<file>halinit_up.c</file>
	<file>halup.rc</file>
</module>
