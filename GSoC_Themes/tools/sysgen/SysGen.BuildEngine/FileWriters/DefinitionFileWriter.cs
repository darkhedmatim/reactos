using System;
using System.IO;
using System.Collections.Generic;
using System.Text;

using SysGen.RBuild.Framework;

namespace SysGen.BuildEngine.Framework
{
    public class DefinitionFileWriter : AutoGeneratedFileWriter
    {
        public DefinitionFileWriter(RBuildProject project, string file)
            : base(project , file)
        {
        }

        public override void WriteFile()
        {
        }
    }
}
