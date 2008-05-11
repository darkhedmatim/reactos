<?xml version="1.0" ?>
<rbuild xmlns:xi="http://www.w3.org/2001/XInclude">
<project name="Project" makefile="Makefile">
	<module name="module1" type="test">
		<component name="ntdll.dll">
			<symbol newname="RtlAllocateHeap">HeapAlloc@12</symbol>
			<symbol>LdrAccessResource@16</symbol>
		</component>
	</module>
</project>
</rbuild>
