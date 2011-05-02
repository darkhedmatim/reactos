using System;
using System.IO;
using System.Collections.Generic;
using System.Text;

using SysGen.Framework;
using SysGen.RBuild.Framework;

namespace SysGen.Framework
{
    public class BuildNumberFileWriter : AutoGeneratedCFileWriter
    {
        public BuildNumberFileWriter(RBuildProject project, string file)
            : base(project , file)
        {
        }

        public override void WriteFile()
        {
            WriteHeader();
            WriteCompilationUnit();
            WriteFooter();
        }

        private void WriteBuildNumber()
        {
            WriteLine("#ifndef _INC_REACTOS_BUILDNO");
            WriteLine("#define _INC_REACTOS_BUILDNO");
            WriteLine("#define KERNEL_VERSION_BUILD	20080427");
            WriteLine("#define KERNEL_VERSION_BUILD_HEX	0x8187");
            WriteLine("#define KERNEL_VERSION_BUILD_STR	\"20080427-r33159\"");
            WriteLine("#define KERNEL_VERSION_BUILD_RC	\"20080427-r33159\0\"");
            WriteLine("#define KERNEL_RELEASE_RC	\"0.4-SVN\0\"");
            WriteLine("#define KERNEL_RELEASE_STR	\"0.4-SVN\"");
            WriteLine("#define KERNEL_VERSION_RC	\"0.4-SVN\0\"");
            WriteLine("#define KERNEL_VERSION_STR	\"0.4-SVN\"");
            WriteLine("#define REACTOS_DLL_VERSION_MAJOR	42");
            WriteLine("#define REACTOS_DLL_RELEASE_RC	\"42.4-SVN\0\"");
            WriteLine("#define REACTOS_DLL_RELEASE_STR	\"42.4-SVN\"");
            WriteLine("#define REACTOS_DLL_VERSION_RC	\"42.4-SVN\0\"");
            WriteLine("#define REACTOS_DLL_VERSION_STR	\"42.4-SVN\"");
            WriteLine("#endif");
        }
    }
}