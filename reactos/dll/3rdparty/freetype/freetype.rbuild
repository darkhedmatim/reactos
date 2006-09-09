<module name="freetype" type="kernelmodedll" entrypoint="0" baseaddress="${BASEADDRESS_FREETYPE}" installbase="system32" installname="freetype.dll" entrypoint="0" allowwarnings="true">
	<importlibrary definition="freetype.def" />
	<include base="freetype">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="_SEH_NO_NATIVE_NLG" />
	<define name="__NTDRIVER__" />
	<define name="__NO_CTYPE_INLINES" />
	<define name="__USE_W32API" />
	<if property="NSWPAT" value="1">
		<define name="TT_CONFIG_OPTION_BYTECODE_INTERPRETER" />
	</if>
	<library>ntoskrnl</library>
	<library>hal</library>
	<directory name="i386">
		<file>setjmplongjmp.s</file>
	</directory>
	<directory name="src">
		<directory name="base">
			<file>ftsystem.c</file>
			<file>ftdebug.c</file>
			<file>ftinit.c</file>
			<file>ftbase.c</file>
			<file>ftbbox.c</file>
			<file>ftbdf.c</file>
			<file>ftbitmap.c</file>
			<file>ftglyph.c</file>
			<file>ftmm.c</file>
			<file>ftotval.c</file>
			<file>ftpfr.c</file>
			<file>ftstroke.c</file>
			<file>ftsynth.c</file>
			<file>fttype1.c</file>
			<file>ftwinfnt.c</file>
			<file>ftxf86.c</file>
		</directory>
		<directory name="autofit">
			<file>autofit.c</file>
		</directory>
		<directory name="bdf">
			<file>bdf.c</file>
		</directory>
		<directory name="cache">
			<file>ftcache.c</file>
		</directory>
		<directory name="cff">
			<file>cff.c</file>
		</directory>
		<directory name="cid">
			<file>type1cid.c</file>
		</directory>
		<directory name="gzip">
			<file>ftgzip.c</file>
		</directory>
		<directory name="lzw">
			<file>ftlzw.c</file>
		</directory>
		<directory name="otvalid">
			<file>otvbase.c</file>
			<file>otvcommn.c</file>
			<file>otvgpos.c</file>
			<file>otvgsub.c</file>
			<file>otvgdef.c</file>
			<file>otvjstf.c</file>
			<file>otvmod.c</file>
		</directory>
		<directory name="pcf">
			<file>pcf.c</file>
		</directory>
		<directory name="pfr">
			<file>pfr.c</file>
		</directory>
		<directory name="psaux">
			<file>psaux.c</file>
		</directory>
		<directory name="pshinter">
			<file>pshinter.c</file>
		</directory>
		<directory name="psnames">
			<file>psmodule.c</file>
		</directory>
		<directory name="raster">
			<file>raster.c</file>
		</directory>
		<directory name="sfnt">
			<file>sfnt.c</file>
		</directory>
		<directory name="smooth">
			<file>smooth.c</file>
		</directory>
		<directory name="truetype">
			<file>truetype.c</file>
		</directory>
		<directory name="type1">
			<file>type1.c</file>
		</directory>
		<directory name="type42">
			<file>type42.c</file>
		</directory>
		<directory name="winfonts">
			<file>winfnt.c</file>
		</directory>
	</directory>
	<file>rosglue.c</file>
	<file>freetype.rc</file>
</module>
