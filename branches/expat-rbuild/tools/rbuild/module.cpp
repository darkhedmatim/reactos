/*
 * Copyright (C) 2005 Casper S. Hornstrup
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "pch.h"

#include <assert.h>

#include "rbuild.h"

using std::string;
using std::vector;
using namespace XMLStorage;

string
Right ( const string& s, size_t n )
{
	if ( n > s.size() )
		return s;
	return string ( &s[s.size()-n] );
}

string
Replace ( const string& s, const string& find, const string& with )
{
	string ret;
	const char* p = s.c_str();
	while ( p )
	{
		const char* p2 = strstr ( p, find.c_str() );
		if ( !p2 )
			break;
		if ( p2 > p )
			ret += string ( p, p2-p );
		ret += with;
		p = p2 + find.size();
	}
	if ( *p )
		ret += p;
	return ret;
}

string
ChangeSeparator ( const string& s,
				  const char fromSeparator,
				  const char toSeparator )
{
	string s2(s);
	char* p = strchr ( &s2[0], fromSeparator );
	while ( p )
	{
		*p++ = toSeparator;
		p = strchr ( p, fromSeparator );
	}
	return s2;
}

string
FixSeparator ( const string& s )
{
	return ChangeSeparator ( s, cBadSep, cSep );
}

string
FixSeparatorForSystemCommand ( const string& s )
{
	string s2(s);
	char* p = strchr ( &s2[0], DEF_CBAD_SEP );
	while ( p )
	{
		*p++ = DEF_CSEP;
		p = strchr ( p, DEF_CBAD_SEP );
	}
	return s2;
}

string
DosSeparator ( const string& s )
{
	string s2(s);
	char* p = strchr ( &s2[0], '/' );
	while ( p )
	{
		*p++ = '\\';
		p = strchr ( p, '/' );
	}
	return s2;
}

string
ReplaceExtension (
	const string& filename,
	const string& newExtension )
{
	size_t index = filename.find_last_of ( '/' );
	if ( index == string::npos )
		index = 0;
	size_t index2 = filename.find_last_of ( '\\' );
	if ( index2 != string::npos && index2 > index )
		index = index2;
	string tmp = filename.substr( index /*, filename.size() - index*/ );
	size_t ext_index = tmp.find_last_of( '.' );
	if ( ext_index != string::npos )
		return filename.substr ( 0, index + ext_index ) + newExtension;
	return filename + newExtension;
}

string
GetSubPath (
	const XMLLocation& location,
	const string& path,
	const string& att_value )
{
	if ( !att_value.size() )
		throw XMLInvalidBuildFileException (
			location,
			"<directory> tag has empty 'name' attribute" );
	if ( strpbrk ( att_value.c_str (), "/\\?*:<>|" ) )
		throw XMLInvalidBuildFileException (
			location,
			"<directory> tag has invalid characters in 'name' attribute" );
	if ( !path.size() )
		return att_value;
	return FixSeparator(path + cSep + att_value);
}

string
GetExtension ( const string& filename )
{
	size_t index = filename.find_last_of ( '/' );
	if (index == string::npos) index = 0;
	string tmp = filename.substr( index, filename.size() - index );
	size_t ext_index = tmp.find_last_of( '.' );
	if (ext_index != string::npos)
		return filename.substr ( index + ext_index, filename.size() );
	return "";
}

string
GetDirectory ( const string& filename )
{
	size_t index = filename.find_last_of ( cSep );
	if ( index == string::npos )
		return "";
	else
		return filename.substr ( 0, index );
}

string
GetFilename ( const string& filename )
{
	size_t index = filename.find_last_of ( cSep );
	if ( index == string::npos )
		return filename;
	else
		return filename.substr ( index + 1, filename.length () - index );
}

string
NormalizeFilename ( const string& filename )
{
	if ( filename == "" )
		return "";
	Path path;
	string normalizedPath = path.Fixup ( filename, true );
	string relativeNormalizedPath = path.RelativeFromWorkingDirectory ( normalizedPath );
	return FixSeparator ( relativeNormalizedPath );
}

bool
GetBooleanValue ( const string& value )
{
	if ( value == "1" )
		return true;
	else
		return false;
}

string
ToLower ( string filename )
{
	for ( size_t i = 1; i < filename.length (); i++ )
		filename[i] = tolower ( filename[i] );
	return filename;
}

void IfableData::ExtractModules( std::vector<Module*> &modules )
{
	size_t i;
	for ( i = 0; i < this->modules.size (); i++ )
		modules.push_back(this->modules[i]);
}

IfableData::~IfableData()
{
	size_t i;
	for ( i = 0; i < includes.size (); i++ )
		delete includes[i];
	for ( i = 0; i < defines.size (); i++ )
		delete defines[i];
	for ( i = 0; i < libraries.size (); i++ )
		delete libraries[i];
	for ( i = 0; i < properties.size (); i++ )
		delete properties[i];
	for ( i = 0; i < compilerFlags.size (); i++ )
		delete compilerFlags[i];
	for ( i = 0; i < modules.size(); i++ )
		delete modules[i];
	for ( i = 0; i < ifs.size (); i++ )
		delete ifs[i];
	for ( i = 0; i < compilationUnits.size (); i++ )
		delete compilationUnits[i];
}

void IfableData::ProcessXML ()
{
	size_t i;
	for ( i = 0; i < includes.size (); i++ )
		includes[i]->ProcessXML ();
	for ( i = 0; i < defines.size (); i++ )
		defines[i]->ProcessXML ();
	for ( i = 0; i < libraries.size (); i++ )
		libraries[i]->ProcessXML ();
	for ( i = 0; i < properties.size(); i++ )
		properties[i]->ProcessXML ();
	for ( i = 0; i < compilerFlags.size(); i++ )
		compilerFlags[i]->ProcessXML ();
	for ( i = 0; i < ifs.size (); i++ )
		ifs[i]->ProcessXML ();
	for ( i = 0; i < compilationUnits.size (); i++ )
		compilationUnits[i]->ProcessXML ();
}

Module::Module ( const Project& project,
				 const XMLNode& moduleNode,
				 const string& modulePath )
	: project (project),
	  node (moduleNode),
	  importLibrary (NULL),
	  bootstrap (NULL),
	  autoRegister(NULL),
	  linkerScript (NULL),
	  pch (NULL),
	  cplusplus (false),
	  host (HostDefault)
{
	if ( node != "module" )
		throw InvalidOperationException ( __FILE__,
										  __LINE__,
										  "Module created with non-<module> node" );

	xmlbuildFile = Path::RelativeFromWorkingDirectory ( modulePath/*@@ moduleNode.xmlFile->filename ()*/ );

	path = FixSeparator ( modulePath );

	enabled = XMLBool(&moduleNode, "if", true);
	enabled = !XMLBool(&moduleNode, "ifnot", !enabled);

	name = moduleNode.get("name");

	type = GetModuleType(node.get_location(), "type", moduleNode.get("type"));

	extension = moduleNode.get("extension");
	if (extension.empty())
		extension = GetDefaultModuleExtension ();

	isUnicode = XMLBool(&moduleNode, "unicode", false);

	entrypoint = moduleNode.get("entrypoint");
	if (entrypoint.empty())
		entrypoint = GetDefaultModuleEntrypoint();

	baseaddress = moduleNode.get("baseaddress");
	if (baseaddress.empty())
		baseaddress = GetDefaultModuleBaseaddress();

	mangledSymbols = XMLBool(&moduleNode, "mangledsymbols", false);

	const string& host_str = moduleNode.get("host");
	if (!host_str.empty())
		host = XMLBool(host_str.c_str())? HostTrue: HostFalse;

	prefix = moduleNode.get("prefix");

	installBase = moduleNode.get("installbase");

	installName = moduleNode.get("installname");

	useWRC = XMLBool(&moduleNode, "usewrc", true);	// default: usewrc="true"

	const string& warnings = moduleNode.get("warnings");
	if (!warnings.empty())
	{
		printf ( "%s: WARNING: 'warnings' attribute of <module> is deprecated, use 'allowwarnings' instead\n",
			moduleNode.get_location().str().c_str() );
	}

	allowWarnings = XMLBool(&moduleNode, "allowwarnings");

	aliasedModuleName = moduleNode.get("aliasof");

	if ( type == BootProgram )
		payload = get_required_attribute(&moduleNode, "payload");
}

Module::~Module ()
{
	size_t i;
	for ( i = 0; i < invocations.size(); i++ )
		delete invocations[i];
	for ( i = 0; i < dependencies.size(); i++ )
		delete dependencies[i];
	for ( i = 0; i < compilerFlags.size(); i++ )
		delete compilerFlags[i];
	for ( i = 0; i < linkerFlags.size(); i++ )
		delete linkerFlags[i];
	for ( i = 0; i < stubbedComponents.size(); i++ )
		delete stubbedComponents[i];
	if ( linkerScript )
		delete linkerScript;
	if ( pch )
		delete pch;
}

void
Module::ProcessXML(const std::string& xml_path)
{
	size_t i;

	if ( type == Alias )
	{
		if ( aliasedModuleName == name )
		{
			throw XMLInvalidBuildFileException (
				node.get_location(),
				"module '%s' cannot link against itself",
				name.c_str() );
		}
		const Module* m = project.LocateModule ( aliasedModuleName );
		if ( !m )
		{
			throw XMLInvalidBuildFileException (
				node.get_location(),
				"module '%s' trying to alias non-existant module '%s'",
				name.c_str(),
				aliasedModuleName.c_str() );
		}
	}

	const XMLNode::Children& children = node.get_children();
	for(XMLNode::Children::const_iterator it=children.begin(); it!=children.end(); ++it)
	{
		ParseContext parseContext;
		ProcessXMLSubElement (**it, path, parseContext, xml_path);
	}
	for (i = 0; i < invocations.size(); i++ )
		invocations[i]->ProcessXML ();
	for ( i = 0; i < dependencies.size(); i++ )
		dependencies[i]->ProcessXML ();
	for ( i = 0; i < compilerFlags.size(); i++ )
		compilerFlags[i]->ProcessXML();
	for ( i = 0; i < linkerFlags.size(); i++ )
		linkerFlags[i]->ProcessXML();
	for ( i = 0; i < stubbedComponents.size(); i++ )
		stubbedComponents[i]->ProcessXML();
	non_if_data.ProcessXML();
	if ( linkerScript )
		linkerScript->ProcessXML();
	if ( pch )
		pch->ProcessXML();
	if ( autoRegister )
		autoRegister->ProcessXML();
}

void
Module::ProcessXMLSubElement ( const XMLNode& e,
							   const string& path,
							   ParseContext& parseContext,
							   const std::string& xml_path )
{
	If* pOldIf = parseContext.ifData;
	CompilationUnit* pOldCompilationUnit = parseContext.compilationUnit;
	string subpath ( path );
	if ( e == "file" && !e.get_content().empty() )
	{
		bool first = XMLBool(&e, "first");
		string switches = e.get("switches");

		if ( !cplusplus )
		{
			// check for c++ file
			string ext = GetExtension ( e.get_content() );
			if ( !_stricmp ( ext.c_str(), ".cpp" ) )
				cplusplus = true;
			else if ( !_stricmp ( ext.c_str(), ".cc" ) )
				cplusplus = true;
			else if ( !_stricmp ( ext.c_str(), ".cxx" ) )
				cplusplus = true;
		}
		File* pFile = new File ( FixSeparator ( path + cSep + e.get_content() ),
								 first,
								 switches,
								 false );
		if ( parseContext.compilationUnit )
			parseContext.compilationUnit->files.push_back ( pFile );
		else
		{
			CompilationUnit* pCompilationUnit = new CompilationUnit ( pFile );
			if ( parseContext.ifData )
				parseContext.ifData->data.compilationUnits.push_back ( pCompilationUnit );
			else
				non_if_data.compilationUnits.push_back ( pCompilationUnit );
		}
		if ( parseContext.ifData )
			parseContext.ifData->data.files.push_back ( pFile );
		else
			non_if_data.files.push_back ( pFile );
		ensure_no_children(e);
	}
	else if ( e == "library" && !e.get_content().empty() )
	{
		Library* pLibrary = new Library ( e, *this, e.get_content() );
		if ( parseContext.ifData )
			parseContext.ifData->data.libraries.push_back ( pLibrary );
		else
			non_if_data.libraries.push_back ( pLibrary );
		ensure_empty_attributes(e);
		ensure_no_children(e);
	}
	else if ( e == "directory" )
	{
		subpath = GetSubPath(e.get_location(), path, e.get("name"));
		ensure_empty_content(e);
	}
	else if ( e == "include" )
	{
		Include* include = new Include ( project, this, &e );
		if ( parseContext.ifData )
			parseContext.ifData->data.includes.push_back ( include );
		else
			non_if_data.includes.push_back ( include );
		ensure_no_children(e);
	}
	else if ( e == "define" )
	{
		Define* pDefine = new Define ( project, this, e );
		if ( parseContext.ifData )
			parseContext.ifData->data.defines.push_back ( pDefine );
		else
			non_if_data.defines.push_back ( pDefine );
		ensure_no_children(e);
	}
	else if ( e == "invoke" )
	{
		if ( parseContext.ifData )
		{
			throw XMLInvalidBuildFileException (
				e.get_location(),
				"<invoke> is not a valid sub-element of <if>" );
		}
		invocations.push_back ( new Invoke ( e, *this ) );
		ensure_empty_content(e);
	}
	else if ( e == "dependency" )
	{
		if ( parseContext.ifData )
		{
			throw XMLInvalidBuildFileException (
				e.get_location(),
				"<dependency> is not a valid sub-element of <if>" );
		}
		dependencies.push_back ( new Dependency ( e, *this ) );
		ensure_no_children(e);
		ensure_empty_attributes(e);
	}
	else if ( e == "importlibrary" )
	{
		if ( parseContext.ifData )
		{
			throw XMLInvalidBuildFileException (
				e.get_location(),
				"<importlibrary> is not a valid sub-element of <if>" );
		}
		if ( importLibrary )
		{
			throw XMLInvalidBuildFileException (
				e.get_location(),
				"Only one <importlibrary> is valid per module" );
		}
		importLibrary = new ImportLibrary ( e, *this );
		ensure_empty_content(e);
		ensure_no_children(e);
	}
	else if ( e == "if" )
	{
		parseContext.ifData = new If ( e, project, this );
		if ( pOldIf )
			pOldIf->data.ifs.push_back ( parseContext.ifData );
		else
			non_if_data.ifs.push_back ( parseContext.ifData );
		ensure_empty_content(e);
	}
	else if ( e == "ifnot" )
	{
		parseContext.ifData = new If ( e, project, this, true );
		if ( pOldIf )
			pOldIf->data.ifs.push_back ( parseContext.ifData );
		else
			non_if_data.ifs.push_back ( parseContext.ifData );
		ensure_empty_content(e);
	}
	else if ( e == "compilerflag" )
	{
		CompilerFlag* pCompilerFlag = new CompilerFlag ( project, this, e );
		if ( parseContext.ifData )
			parseContext.ifData->data.compilerFlags.push_back ( pCompilerFlag );
		else
			non_if_data.compilerFlags.push_back ( pCompilerFlag );
		ensure_empty_attributes(e);
		ensure_no_children(e);
	}
	else if ( e == "linkerflag" )
	{
		linkerFlags.push_back ( new LinkerFlag ( project, this, e ) );
		ensure_empty_attributes(e);
		ensure_no_children(e);
	}
	else if ( e == "linkerscript" )
	{
		if ( linkerScript )
		{
			throw XMLInvalidBuildFileException (
				e.get_location(),
				"Only one <linkerscript> is valid per module" );
		}
		linkerScript = new LinkerScript ( project, this, e );
		ensure_empty_content(e);
		ensure_no_children(e);
	}
	else if ( e == "component" )
	{
		stubbedComponents.push_back ( new StubbedComponent ( this, e ) );
		ensure_empty_content(e);
	}
	else if ( e == "property" )
	{
		throw XMLInvalidBuildFileException (
			e.get_location(),
			"<property> is not a valid sub-element of <module>" );
	}
	else if ( e == "bootstrap" )
	{
		bootstrap = new Bootstrap ( project, this, e );
		ensure_empty_content(e);
		ensure_no_children(e);
	}
	else if ( e == "pch" )
	{
		if ( parseContext.ifData )
		{
			throw XMLInvalidBuildFileException (
				e.get_location(),
				"<pch> is not a valid sub-element of <if>" );
		}
		if ( pch )
		{
			throw XMLInvalidBuildFileException (
				e.get_location(),
				"Only one <pch> is valid per module" );
		}
		pch = new PchFile (
			e, *this, File ( FixSeparator ( path + cSep + e.get_content() ), false, "", true ) );
		ensure_empty_attributes(e);
		ensure_no_children(e);
	}
	else if ( e == "compilationunit" )
	{
		if ( project.configuration.CompilationUnitsEnabled )
		{
			CompilationUnit* pCompilationUnit = new CompilationUnit ( &project, this, &e );
			if ( parseContext.ifData )
				parseContext.ifData->data.compilationUnits.push_back ( pCompilationUnit );
			else
				non_if_data.compilationUnits.push_back ( pCompilationUnit );
			parseContext.compilationUnit = pCompilationUnit;
		}
		ensure_empty_content(e);
	}
	else if ( e == "autoregister" )
	{
		if ( autoRegister != NULL)
		{
			throw XMLInvalidBuildFileException (
				e.get_location(),
				"there can be only one <%s> element for a module",
				e.c_str() );
		}
		autoRegister = new AutoRegister ( project, this, e );
		ensure_empty_content(e);
		ensure_no_children(e);
	}
	else if (e == "library")
	{
		ensure_empty_content(e);
	}
	else if (e == "symbol")
	{
	}
	else if (e == "xi:include")
	{
		const string& href = get_required_attribute(&e, "href");
		ensure_empty_content(e);
	}
	else if (e == "rbuild")
	{
		ensure_empty_content(e);
	}
	else
	{
		throw XMLInvalidBuildFileException (
			e.get_location(),
			"<%s> unexpected XML node",
			e.c_str() );
	}

	const XMLNode::Children& children = e.get_children();
	for(XMLNode::Children::const_iterator it=children.begin(); it!=children.end(); ++it)
		ProcessXMLSubElement ( **it, subpath, parseContext, xml_path );

	parseContext.ifData = pOldIf;
	parseContext.compilationUnit = pOldCompilationUnit;
}

ModuleType
Module::GetModuleType ( const XMLLocation& location, const std::string& attrib_name, const std::string& attrib_value )
{
	if ( attrib_value == "buildtool" )
		return BuildTool;
	if ( attrib_value == "staticlibrary" )
		return StaticLibrary;
	if ( attrib_value == "objectlibrary" )
		return ObjectLibrary;
	if ( attrib_value == "kernel" )
		return Kernel;
	if ( attrib_value == "kernelmodedll" )
		return KernelModeDLL;
	if ( attrib_value == "kernelmodedriver" )
		return KernelModeDriver;
	if ( attrib_value == "nativedll" )
		return NativeDLL;
	if ( attrib_value == "nativecui" )
		return NativeCUI;
	if ( attrib_value == "win32dll" )
		return Win32DLL;
	if ( attrib_value == "win32cui" )
		return Win32CUI;
	if ( attrib_value == "win32gui" )
		return Win32GUI;
	if ( attrib_value == "bootloader" )
		return BootLoader;
	if ( attrib_value == "bootsector" )
		return BootSector;
	if ( attrib_value == "bootprogram" )
		return BootProgram;
	if ( attrib_value == "iso" )
		return Iso;
	if ( attrib_value == "liveiso" )
		return LiveIso;
	if ( attrib_value == "test" )
		return Test;
	if ( attrib_value == "rpcserver" )
		return RpcServer;
	if ( attrib_value == "rpcclient" )
		return RpcClient;
	if ( attrib_value == "alias" )
		return Alias;
	throw InvalidAttributeValueException ( location,
										   attrib_name,
										   attrib_value );
}

string
Module::GetDefaultModuleExtension () const
{
	switch (type)
	{
		case BuildTool:
			return ExePostfix;
		case StaticLibrary:
			return ".a";
		case ObjectLibrary:
			return ".o";
		case Kernel:
		case NativeCUI:
		case Win32CUI:
		case Win32GUI:
			return ".exe";
		case KernelModeDLL:
		case NativeDLL:
		case Win32DLL:
			return ".dll";
		case KernelModeDriver:
		case BootLoader:
			return ".sys";
		case BootSector:
			return ".o";
		case Iso:
		case LiveIso:
			return ".iso";
		case Test:
			return ".exe";
		case RpcServer:
			return ".o";
		case RpcClient:
			return ".o";
		case Alias:
			return "";
		case BootProgram:
			return "";
	}
	throw InvalidOperationException ( __FILE__,
									  __LINE__ );
}

string
Module::GetDefaultModuleEntrypoint () const
{
	switch ( type )
	{
		case Kernel:
			return "_NtProcessStartup";
		case KernelModeDLL:
			return "_DriverEntry@8";
		case NativeDLL:
			return "_DllMainCRTStartup@12";
		case NativeCUI:
			return "_NtProcessStartup@4";
		case Win32DLL:
			return "_DllMain@12";
		case Win32CUI:
		case Test:
			if ( isUnicode )
				return "_wmainCRTStartup";
			else
				return "_mainCRTStartup";
		case Win32GUI:
			if ( isUnicode )
				return "_wWinMainCRTStartup";
			else
				return "_WinMainCRTStartup";
		case KernelModeDriver:
			return "_DriverEntry@8";
		case BuildTool:
		case StaticLibrary:
		case ObjectLibrary:
		case BootLoader:
		case BootSector:
		case Iso:
		case LiveIso:
		case RpcServer:
		case RpcClient:
		case Alias:
		case BootProgram:
			return "";
	}
	throw InvalidOperationException ( __FILE__,
									  __LINE__ );
}

string
Module::GetDefaultModuleBaseaddress () const
{
	switch ( type )
	{
		case Kernel:
			return "0x80000000";
		case Win32DLL:
			return "0x10000000";
		case NativeDLL:
		case NativeCUI:
		case Win32CUI:
		case Test:
			return "0x00400000";
		case Win32GUI:
			return "0x00400000";
		case KernelModeDLL:
		case KernelModeDriver:
			return "0x00010000";
		case BuildTool:
		case StaticLibrary:
		case ObjectLibrary:
		case BootLoader:
		case BootSector:
		case Iso:
		case LiveIso:
		case RpcServer:
		case RpcClient:
		case Alias:
		case BootProgram:
			return "";
	}
	throw InvalidOperationException ( __FILE__,
									  __LINE__ );
}

bool
Module::HasImportLibrary () const
{
	return importLibrary != NULL;
}

bool
Module::IsDLL () const
{
	switch ( type )
	{
		case Kernel:
		case KernelModeDLL:
		case NativeDLL:
		case Win32DLL:
		case KernelModeDriver:
			return true;
		case NativeCUI:
		case Win32CUI:
		case Test:
		case Win32GUI:
		case BuildTool:
		case StaticLibrary:
		case ObjectLibrary:
		case BootLoader:
		case BootSector:
		case BootProgram:
		case Iso:
		case LiveIso:
		case RpcServer:
		case RpcClient:
		case Alias:
			return false;
	}
	throw InvalidOperationException ( __FILE__,
									  __LINE__ );
}

bool
Module::GenerateInOutputTree () const
{
	switch ( type )
	{
		case Kernel:
		case KernelModeDLL:
		case NativeDLL:
		case Win32DLL:
		case KernelModeDriver:
		case NativeCUI:
		case Win32CUI:
		case Test:
		case Win32GUI:
		case BuildTool:
		case BootLoader:
		case BootSector:
		case BootProgram:
		case Iso:
		case LiveIso:
			return true;
		case StaticLibrary:
		case ObjectLibrary:
		case RpcServer:
		case RpcClient:
		case Alias:
			return false;
	}
	throw InvalidOperationException ( __FILE__,
									  __LINE__ );
}

string
Module::GetTargetName () const
{
	return name + extension;
}

string
Module::GetDependencyPath () const
{
	if ( HasImportLibrary () )
		return ReplaceExtension ( GetPathWithPrefix ( "lib" ), ".a" );
	else
		return GetPath();
}

string
Module::GetBasePath () const
{
	return path;
}

string
Module::GetPath () const
{
	if ( path.length() > 0 )
		return path + cSep + GetTargetName ();
	else
		return GetTargetName ();
}

string
Module::GetPathWithPrefix ( const string& prefix ) const
{
	return path + cSep + prefix + GetTargetName ();
}

string
Module::GetPathToBaseDir () const
{
	string temp_path = path;
	string result = "..\\";
	while(temp_path.find ('\\') != string::npos)
	{
		temp_path.erase (0, temp_path.find('\\')+1);
		result += "..\\";
	}
	return result;
}

string
Module::GetInvocationTarget ( const int index ) const
{
	return ssprintf ( "%s_invoke_%d",
					  name.c_str (),
					  index );
}

bool
Module::HasFileWithExtension (
	const IfableData& data,
	const std::string& extension ) const
{
	size_t i;
	for ( i = 0; i < data.compilationUnits.size (); i++ )
	{
		CompilationUnit* compilationUnit = data.compilationUnits[i];
		if ( compilationUnit->HasFileWithExtension ( extension ) )
			return true;
	}
	for ( i = 0; i < data.ifs.size (); i++ )
	{
		if ( HasFileWithExtension ( data.ifs[i]->data, extension ) )
			return true;
	}
	return false;
}

void
Module::InvokeModule () const
{
	for ( size_t i = 0; i < invocations.size (); i++ )
	{
		Invoke& invoke = *invocations[i];
		string command = FixSeparatorForSystemCommand(invoke.invokeModule->GetPath ()) + " " + invoke.GetParameters ();
		printf ( "Executing '%s'\n\n", command.c_str () );
		int exitcode = system ( command.c_str () );
		if ( exitcode != 0 )
			throw InvocationFailedException ( command,
											  exitcode );
	}
}


File::File ( const string& _name, bool _first,
			 std::string _switches,
			 bool _isPreCompiledHeader )
	: name(_name),
	  first(_first),
	  switches(_switches),
	  isPreCompiledHeader(_isPreCompiledHeader)
{
}

void
File::ProcessXML()
{
}


Library::Library ( const XMLNode& _node,
				   const Module& _module,
				   const string& _name )
	: node(_node),
	  module(_module),
	  name(_name),
	  importedModule(_module.project.LocateModule(_name))
{
	if ( module.name == name )
	{
		throw XMLInvalidBuildFileException (
			node.get_location(),
			"module '%s' cannot link against itself",
			name.c_str() );
	}
	if ( !importedModule )
	{
		throw XMLInvalidBuildFileException (
			node.get_location(),
			"module '%s' trying to import non-existant module '%s'",
			module.name.c_str(),
			name.c_str() );
	}
}

void
Library::ProcessXML()
{
	if ( !module.project.LocateModule ( name ) )
	{
		throw XMLInvalidBuildFileException (
			node.get_location(),
			"module '%s' is trying to link against non-existant module '%s'",
			module.name.c_str(),
			name.c_str() );
	}
}


Invoke::Invoke ( const XMLNode& _node,
				 const Module& _module )
	: node (_node),
	  module (_module)
{
}

void
Invoke::ProcessXML()
{
	const string module_str = node.get("module");
	if (module_str.empty())
		invokeModule = &module;
	else
	{
		invokeModule = module.project.LocateModule(module_str);
		if ( invokeModule == NULL )
		{
			throw XMLInvalidBuildFileException (
				node.get_location(),
				"module '%s' is trying to invoke non-existant module '%s'",
				module.name.c_str(),
				module_str.c_str() );
		}
	}

	const XMLNode::Children& children = node.get_children();
	for(XMLNode::Children::const_iterator it=children.begin(); it!=children.end(); ++it)
		ProcessXMLSubElement(**it);
}

void
Invoke::ProcessXMLSubElement ( const XMLNode& e )
{
	if ( e == "input" )
	{
		const XMLNode::Children& children = e.get_children();
		for(XMLNode::Children::const_iterator it=children.begin(); it!=children.end(); ++it)
			ProcessXMLSubElementInput(**it);
	}
	else if ( e == "output" )
	{
		const XMLNode::Children& children = node.get_children();
		for(XMLNode::Children::const_iterator it=children.begin(); it!=children.end(); ++it)
			ProcessXMLSubElementOutput(**it);
	}
}

void
Invoke::ProcessXMLSubElementInput ( const XMLNode& e )
{
	if ( e == "inputfile" && !e.get_content().empty() )
	{
		input.push_back ( new InvokeFile (
			e, FixSeparator ( module.path + cSep + e.get_content() ) ) );
		ensure_no_children(e);
	}
}

void
Invoke::ProcessXMLSubElementOutput ( const XMLNode& e )
{
	if ( e == "outputfile" && !e.get_content().empty() )
	{
		output.push_back ( new InvokeFile (
			e, FixSeparator ( module.path + cSep + e.get_content() ) ) );
		ensure_no_children(e);
	}
}

void
Invoke::GetTargets ( string_list& targets ) const
{
	for ( size_t i = 0; i < output.size (); i++ )
	{
		InvokeFile& file = *output[i];
		targets.push_back ( NormalizeFilename ( file.name ) );
	}
}

string
Invoke::GetParameters () const
{
	string parameters ( "" );
	size_t i;
	for ( i = 0; i < output.size (); i++ )
	{
		if ( parameters.length () > 0)
			parameters += " ";
		InvokeFile& invokeFile = *output[i];
		if ( invokeFile.switches.length () > 0 )
		{
			parameters += invokeFile.switches + " ";
		}
		parameters += invokeFile.name;
	}

	for ( i = 0; i < input.size (); i++ )
	{
		if ( parameters.length () > 0 )
			parameters += " ";
		InvokeFile& invokeFile = *input[i];
		if ( invokeFile.switches.length () > 0 )
		{
			parameters += invokeFile.switches;
			parameters += " ";
		}
		parameters += invokeFile.name ;
	}

	return parameters;
}


InvokeFile::InvokeFile ( const XMLNode& _node,
						 const string& _name )
	: node (_node),
	  name (_name)
{
	switches = _node.get("switches");
}

void
InvokeFile::ProcessXML()
{
}


Dependency::Dependency ( const XMLNode& _node,
						 const Module& _module )
	: node (_node),
	  module (_module),
	  dependencyModule (NULL)
{
}

void
Dependency::ProcessXML()
{
	dependencyModule = module.project.LocateModule (node.get_content());
	if ( dependencyModule == NULL )
	{
		throw XMLInvalidBuildFileException (
			node.get_location(),
			"module '%s' depend on non-existant module '%s'",
			module.name.c_str(),
			node.get_content().c_str() );
	}
}


ImportLibrary::ImportLibrary ( const XMLNode& _node,
							   const Module& _module )
	: node (_node),
	  module (_module)
{
	basename = get_attribute(&_node, "basename", module.name);

	definition = FixSeparator(_node.get("definition"));
}


If::If ( const XMLNode& node_,
		 const Project& project_,
		 const Module* module_,
		 const bool negated_ )
	: node(node_), project(project_), module(module_), negated(negated_)
{
	property = get_required_attribute(&node, "property");
	value = get_required_attribute(&node, "value");
}

If::~If ()
{
}

void
If::ProcessXML()
{

}


Property::Property ( const XMLNode& node_,
					 const Project& project_,
					 const Module* module_ )
	: node(node_), project(project_), module(module_)
{
	name = get_required_attribute(&node, "name");
	value = get_required_attribute(&node, "value");
}

void
Property::ProcessXML()
{
}


PchFile::PchFile (
	const XMLNode& node_,
	const Module& module_,
	const File file_ )
	: node(node_), module(module_), file(file_)
{
}

void
PchFile::ProcessXML()
{
}


AutoRegister::AutoRegister ( const Project& project_,
							 const Module* module_,
							 const XMLNode& node_ )
	: project(project_),
	  module(module_),
	  node(node_)
{
	Initialize();
}

AutoRegister::~AutoRegister ()
{
}

bool
AutoRegister::IsSupportedModuleType ( ModuleType type )
{
	switch ( type )
	{
		case Win32DLL:
			return true;
		case Kernel:
		case KernelModeDLL:
		case NativeDLL:
		case NativeCUI:
		case Win32CUI:
		case Win32GUI:
		case KernelModeDriver:
		case BootSector:
		case BootLoader:
		case BootProgram:
		case BuildTool:
		case StaticLibrary:
		case ObjectLibrary:
		case Iso:
		case LiveIso:
		case Test:
		case RpcServer:
		case RpcClient:
		case Alias:
			return false;
	}
	throw InvalidOperationException ( __FILE__,
									  __LINE__ );
}

AutoRegisterType
AutoRegister::GetAutoRegisterType( string type )
{
	if ( type == "DllRegisterServer" )
		return DllRegisterServer;
	if ( type == "DllInstall" )
		return DllInstall;
	if ( type == "Both" )
		return Both;
	throw XMLInvalidBuildFileException (
		node.get_location(),
		"<autoregister> type attribute must be DllRegisterServer, DllInstall or Both." );
}

void
AutoRegister::Initialize ()
{
	if ( !IsSupportedModuleType ( module->type ) )
	{
		throw XMLInvalidBuildFileException (
			node.get_location(),
			"<autoregister> is not applicable for this module type." );
	}

	infSection = get_required_attribute(&node, "infsection");

	const string& type_str = get_required_attribute(&node, "type");
	type = GetAutoRegisterType(type_str);
}

void
AutoRegister::ProcessXML()
{
}
