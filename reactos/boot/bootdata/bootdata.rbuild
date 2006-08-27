<group>
<cdfile>autorun.inf</cdfile>
<cdfile>icon.ico</cdfile>
<cdfile>readme.txt</cdfile>
<cdfile base="reactos">hivecls.inf</cdfile>
<cdfile base="reactos">hivedef.inf</cdfile>
<cdfile base="reactos">hivesft.inf</cdfile>
<cdfile base="reactos">hivesys.inf</cdfile>
<cdfile base="reactos">txtsetup.sif</cdfile>
<!--<cdfile base="reactos">unattend.inf</cdfile>-->
<directory name="bootcd">
	<xi:include href="bootcd/bootcd.rbuild" />
</directory>
<directory name="livecd">
	<xi:include href="livecd/livecd.rbuild" />
</directory>
<directory name="bootcdregtest">
	<xi:include href="bootcdregtest/bootcdregtest.rbuild" />
</directory>
<directory name="livecdregtest">
	<xi:include href="livecdregtest/livecdregtest.rbuild" />
</directory>
</group>
