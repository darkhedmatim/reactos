using System;
using System.IO;
using System.Collections.Generic;
using System.Text;

using SysGen.RBuild.Framework;

namespace SysGen.BuildEngine.Framework
{
    public class HeaderRosCfgFileWriter : AutoGeneratedFileWriter
    {
        public HeaderRosCfgFileWriter(RBuildProject project, string file)
            : base(project , file)
        {
        }

        public override void WriteFile()
        {
            WriteLine("/* This file is autogenerated */");
            WriteLine();

            // Adds a blank line
            WriteLine();
        }
    }
}
