<module name="kmtest_old" type="kernelmodedriver" installbase="system32/drivers" installname="kmtest_old.sys">
	<bootstrap base="$(CDOUTPUT)" />
	<include base="ReactOS">include/reactos/drivers</include>
	<library>ntoskrnl</library>
	<library>hal</library>
	<library>pseh</library>
	<file>kmtest.c</file>
	<file>deviface_test.c</file>
	<file>drvobj_test.c</file>
	<file>devobj_test.c</file>
	<file>reghelper.c</file>
	<file>ntos_ex.c</file>
	<file>ntos_io.c</file>
	<file>ntos_ke.c</file>
	<file>ntos_ob.c</file>
	<file>ntos_pools.c</file>
	<file>ntos_fsrtl.c</file>
	<file>kmtest.rc</file>
</module>
