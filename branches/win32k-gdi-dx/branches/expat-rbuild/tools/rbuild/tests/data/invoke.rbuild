<?xml version="1.0" ?>
<rbuild xmlns:xi="http://www.w3.org/2001/XInclude">
<project name="Project" makefile="Makefile">
	<directory name="dir1">
		<module name="module1" type="buildtool">
			<file>file1.c</file>
			<invoke>
				<output>
					<outputfile>file1.c</outputfile>
				</output>
			</invoke>
		</module>
	</directory>
</project>
</rbuild>
