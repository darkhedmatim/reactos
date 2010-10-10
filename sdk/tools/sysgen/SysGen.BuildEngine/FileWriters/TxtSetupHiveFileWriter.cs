using System;
using System.IO;
using System.Collections.Generic;
using System.Text;

using SysGen.RBuild.Framework;

namespace SysGen.BuildEngine.Framework
{
    public class TxtSetupHiveFileWriter : AutoGeneratedInfFileWriter
    {
        public TxtSetupHiveFileWriter(RBuildProject project, string file)
            : base(project , file)
        {
        }

        public override void WriteFile()
        {
            WriteHeader();
            WriteDirectories();
        }

        protected override void WriteHeader()
        {
            WriteSection("Version");
            WriteLine("Signature = \"$ReactOS$\"");
            WriteLine();
        }

        public void WriteDirectories()
        {
            WriteLine("[AddReg]");

            if (Project.Platform.Shell != null)
            {
                WriteLine("HKLM,\"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\\",\"Shell\",0x00020000,\"{0}\"",
                    Project.Platform.Shell.PlatformInstall.FullPath);
            }

            if (Project.Platform.Screensaver != null)
            {
                WriteLine("HKCU,\"Control Panel\\Desktop\",\"SCRNSAVE.EXE\",0x00000000,\"{0}\"",
                    Project.Platform.Screensaver.PlatformInstall.FullPath);
            }

            if (Project.Platform.Wallpaper != null)
            {
                WriteLine("HKCU,\"Control Panel\\Desktop\",\"Wallpaper\",0x00000000,\"{0}\"",
                    Project.Platform.Wallpaper.PlatformInstall.FullPath);
            }

            if (Project.Platform.DebugChannels.Count > 0)
            {
                WriteLine("HKCU,\"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment\",\"DEBUGCHANNEL\",0x00020000,\"{0}\"",
                    Project.Platform.DebugChannels.Text);
            }
        }
    }
}
