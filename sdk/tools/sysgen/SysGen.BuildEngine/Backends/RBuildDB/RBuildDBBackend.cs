using System;
using System.IO;
using System.Collections.Generic;
using System.Text;
using System.Xml;

using SysGen.BuildEngine.Backends;
using SysGen.RBuild.Framework;
using SysGen.BuildEngine;

namespace SysGen.BuildEngine.Backends
{
    public class RBuildDBBackend : Backend
    {
        public RBuildDBBackend(SysGenEngine sysgen)
            : base(sysgen)
        {
        }

        protected override string FriendlyName
        {
            get { return "RBuild Database"; }
        }

        private string RBuildDBFile
        {
            get { return Path.Combine(SysGen.BaseDirectory, "rbuilddb.xml"); }
        }

        protected override void Generate()
        {
            // Creates an XML file is not exist
            using (XmlTextWriter writer = new XmlTextWriter(RBuildDBFile, Encoding.ASCII))
            {
                writer.Indentation = 4;
                writer.Formatting = Formatting.Indented;

                // Starts a new document
                writer.WriteStartDocument();
                writer.WriteComment("File autogenerated by SysGen");
                writer.WriteStartElement("catalog");

                writer.WriteStartElement("modules");
                foreach (RBuildModule module in Project.Modules)
                {
                    writer.WriteStartElement("module");
                    writer.WriteAttributeString("name", module.Name);
                    writer.WriteAttributeString("type", module.Type.ToString());
                    writer.WriteAttributeString("base", module.Base);
                    writer.WriteAttributeString("desc", module.Description);
                    writer.WriteAttributeString("path", module.CatalogPath);
                    writer.WriteAttributeString("enabled", module.Enabled.ToString());

                    writer.WriteStartElement("libraries");

                    foreach (RBuildModule library in module.Libraries)
                        writer.WriteElementString("library", library.Name);

                    writer.WriteEndElement();

                    writer.WriteStartElement("dependencies");

                    foreach (RBuildModule dependency in module.Dependencies)
                        writer.WriteElementString("dependency", dependency.Name);

                    writer.WriteEndElement();

                    writer.WriteStartElement("requeriments");

                    foreach (RBuildModule requirement in module.Requeriments)
                        writer.WriteElementString("requires", requirement.Name);

                    writer.WriteEndElement();

                    writer.WriteEndElement();
                }
                writer.WriteEndElement();

                writer.WriteStartElement("languages");
                foreach (RBuildLanguage language in Project.Languages)
                {
                    writer.WriteStartElement("language");
                    writer.WriteAttributeString("name", language.Name);
                    writer.WriteEndElement();
                }
                writer.WriteEndElement();

                writer.WriteStartElement("debugchannels");
                foreach (RBuildDebugChannel language in Project.DebugChannels)
                {
                    writer.WriteStartElement("debugchannel");
                    writer.WriteAttributeString("name", language.Name);
                    writer.WriteEndElement();
                }
                writer.WriteEndElement();
                writer.WriteEndElement();

                writer.WriteEndDocument();
            }
        }
    }
}