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

#include <iostream>

#include "rbuild.h"
#include "backend/backend.h"

using std::string;
using std::vector;
using namespace XMLStorage;

#ifdef WIN32
#define getcwd _getcwd
#endif//WIN32


string working_directory;

void
InitWorkingDirectory()
{
	// store the current directory for path calculations
	working_directory.resize ( MAX_PATH );
	working_directory[0] = 0;
	getcwd ( &working_directory[0], working_directory.size() );
	working_directory.resize ( strlen ( working_directory.c_str() ) );
}


Path::Path()
{
	if ( !working_directory.size() )
		InitWorkingDirectory();
	string s ( working_directory );
	const char* p = strtok ( &s[0], "/\\" );
	while ( p )
	{
		if ( *p )
			path.push_back ( p );
		p = strtok ( NULL, "/\\" );
	}
}

Path::Path ( const Path& cwd, const string& file )
{
	string s ( cwd.Fixup ( file, false ) );
	const char* p = strtok ( &s[0], "/\\" );
	while ( p )
	{
		if ( *p )
			path.push_back ( p );
		p = strtok ( NULL, "/\\" );
	}
}

string
Path::Fixup ( const string& file, bool include_filename ) const
{
	if ( strchr ( "/\\", file[0] )
#ifdef WIN32
		// this squirreliness is b/c win32 has drive letters and *nix doesn't...
		|| file[1] == ':'
#endif//WIN32
		)
	{
		return file;
	}
	vector<string> pathtmp ( path );
	string tmp ( file );
	const char* prev = strtok ( &tmp[0], "/\\" );
	const char* p = strtok ( NULL, "/\\" );
	while ( p )
	{
		if ( !strcmp ( prev, "." ) )
			; // do nothing
		else if ( !strcmp ( prev, ".." ) )
		{
			// this squirreliness is b/c win32 has drive letters and *nix doesn't...
#ifdef WIN32
			if ( pathtmp.size() > 1 )
#else
			if ( pathtmp.size() )
#endif
				pathtmp.resize ( pathtmp.size() - 1 );
		}
		else
			pathtmp.push_back ( prev );
		prev = p;
		p = strtok ( NULL, "/\\" );
	}
	if ( include_filename )
		pathtmp.push_back ( prev );

	// reuse tmp variable to return recombined path
	tmp.resize(0);
	for( size_t i = 0; i < pathtmp.size(); i++ )
	{
		// this squirreliness is b/c win32 has drive letters and *nix doesn't...
#ifdef WIN32
		if ( i ) tmp += "/";
#else
		tmp += "/";
#endif
		tmp += pathtmp[i];
	}
	return tmp;
}

string
Path::RelativeFromWorkingDirectory ()
{
	string out = "";
	for( size_t i = 0; i < path.size(); i++ )
	{
		out += "/" + path[i];
	}
	return RelativeFromWorkingDirectory ( out );
}

string
Path::RelativeFromWorkingDirectory ( const string& path )
{
	return Path::RelativeFromDirectory ( path, working_directory );
}

string
Path::RelativeFromDirectory (
	const string& path,
	const string& base_directory )
{
	vector<string> vbase, vpath, vout;
	Path::Split ( vbase, base_directory, true );
	Path::Split ( vpath, path, true );
#ifdef WIN32
	// this squirreliness is b/c win32 has drive letters and *nix doesn't...
	// not possible to do relative across different drive letters
	{
		char path_driveletter = (path[1] == ':') ? toupper(path[0]) : 0;
		char base_driveletter = (base_directory[1] == ':') ? toupper(base_directory[0]) : 0;
		if ( path_driveletter != base_driveletter )
			return path;
	}
#endif
	size_t i = 0;
	while ( i < vbase.size() && i < vpath.size() && vbase[i] == vpath[i] )
		++i;

	// did we go through all of the path?
	if ( vbase.size() == vpath.size() && i == vpath.size() )
		return ".";

	if ( i < vbase.size() )
	{
		// path goes above our base directory, we will need some ..'s
		for( size_t j = i; j < vbase.size(); j++ )
			vout.push_back ( ".." );
	}

	while ( i < vpath.size() )
		vout.push_back ( vpath[i++] );

	// now merge vout into a string again
	string out = vout[0];
	for( i = 1; i < vout.size(); i++ )
	{
		out += "/" + vout[i];
	}
	return out;
}

void
Path::Split (
	vector<string>& out,
	const string& path,
	bool include_last )
{
	string s ( path );
	const char* prev = strtok ( &s[0], "/\\" );
	const char* p = strtok ( NULL, "/\\" );
	out.clear();
	while ( p )
	{
		if ( strcmp ( prev, "." ) )
			out.push_back ( prev );
		prev = p;
		p = strtok ( NULL, "/\\" );
	}
	if ( include_last && strcmp ( prev, "." ) )
		out.push_back ( prev );
	// special-case where path only has "."
	// don't move this check up higher as it might miss
	// some funny paths...
	if ( !out.size() && !strcmp ( prev, "." ) )
		out.push_back ( "." );
}


void RBuildReader::StartElementHandler(const XS_String& name, const XMLNode::AttributeMap& attributes)
{
	XMLNode* parentNode = _pos;

	super::StartElementHandler(name, attributes);

	if (include_depth)
		++include_depth;
	else if (name == "xi:include") {
		include_depth = 1;

		XMLNode::AttributeMap::const_iterator found = attributes.find("href");

		if (found == attributes.end())
			throw XMLRequiredAttributeNotFoundException(get_location(), "href", name);

		string includeFile = path.Fixup(found->second, true);
		string topIncludeFile = Path::RelativeFromWorkingDirectory(includeFile);

		includes.push(XMLInclude(_pos, parentNode, path, topIncludeFile));
	}
}

void RBuildReader::EndElementHandler()
{
	super::EndElementHandler();

	if (include_depth)
		include_depth--;
}


#ifdef _MSC_VER
unsigned __int64
#else
unsigned long long
#endif
filelen ( FILE* f )
{
#ifdef WIN32
	return _filelengthi64 ( _fileno(f) );
#else
# ifdef __FreeBSD__
	struct stat file_stat;
	if ( fstat(fileno(f), &file_stat) != 0 )
# else
	struct stat64 file_stat;
	if ( fstat64(fileno(f), &file_stat) != 0 )
# endif // __FreeBSD__
		return 0;
	return file_stat.st_size;
#endif // WIN32
}


/* static */ string
Environment::GetVariable ( const string& name )
{
	char* value = getenv ( name.c_str () );
	if ( value != NULL && strlen ( value ) > 0 )
		return ssprintf ( "%s",
						  value );
	else
		return "";
}

/* static */ string
Environment::GetEnvironmentVariablePathOrDefault ( const string& name,
												   const string& defaultValue )
{
	const string& environmentVariableValue = Environment::GetVariable ( name );
	if ( environmentVariableValue.length () > 0 )
		return NormalizeFilename ( environmentVariableValue );
	else
		return defaultValue;
}

/* static */ string
Environment::GetIntermediatePath ()
{
	return GetEnvironmentVariablePathOrDefault ( "ROS_INTERMEDIATE",
												 "obj-i386" );
}

/* static */ string
Environment::GetOutputPath ()
{
	return GetEnvironmentVariablePathOrDefault ( "ROS_OUTPUT",
												 "output-i386" );
}

/* static */ string
Environment::GetInstallPath ()
{
	return GetEnvironmentVariablePathOrDefault ( "ROS_INSTALL",
												 "reactos" );
}

ParseContext::ParseContext ()
	: ifData (NULL),
	  compilationUnit (NULL)
{
}


FileLocation::FileLocation ( Directory* directory,
							 std::string filename )
							 : directory (directory),
							   filename (filename)
{
}


Project::Project ( const Configuration& configuration,
				   const string& filename )
	: xmlfile (filename),
	  backend(NULL),
	  configuration (configuration)
{
	ReadXml();
}

Project::~Project ()
{
	size_t i;
	delete backend;
#ifdef NOT_NEEDED_SINCE_THESE_ARE_CLEANED_BY_IFABLE_DATA
	for( i = 0; i < modules.size (); i++ )
		delete modules[i];
#endif
	for( i = 0; i < linkerFlags.size (); i++ )
		delete linkerFlags[i];
	for( i = 0; i < cdfiles.size (); i++ )
		delete cdfiles[i];
	for( i = 0; i < installfiles.size (); i++ )
		delete installfiles[i];

	while(!docs.empty()) {
		delete docs.top();
		docs.pop();
	}
}

const Property*
Project::LookupProperty ( const string& name ) const
{
	for( size_t i = 0; i < non_if_data.properties.size (); i++ )
	{
		const Property* property = non_if_data.properties[i];
		if ( property->name == name )
			return property;
	}
	return NULL;
}

string
Project::ResolveNextProperty ( string& s ) const
{
	size_t i = s.find ( "${" );
	if ( i == string::npos )
		i = s.find ( "$(" );
	if ( i != string::npos )
	{
		string endCharacter;
		if ( s[i + 1] == '{' )
			endCharacter = "}";
		else
			endCharacter = ")";
		size_t j = s.find ( endCharacter );
		if ( j != string::npos )
		{
			int propertyNameLength = j - i - 2;
			string propertyName = s.substr ( i + 2, propertyNameLength );
			const Property* property = LookupProperty ( propertyName );
			if ( property != NULL )
				return s.replace ( i, propertyNameLength + 3, property->value );
		}
	}
	return s;
}

string
Project::ResolveProperties ( const string& s ) const
{
	string s2 = s;
	string s3;
	do
	{
		s3 = s2;
		s2 = ResolveNextProperty ( s3 );
	} while ( s2 != s3 );
	return s2;
}

void
Project::SetConfigurationOption ( char* s,
								  string name,
								  string* alternativeName )
{
	const Property* property = LookupProperty ( name );
	if ( property != NULL && property->value.length () > 0 )
	{
		s = s + sprintf ( s,
						  "#define %s=%s\n",
						  property->name.c_str (),
						  property->value.c_str () );
	}
	else if ( property != NULL )
	{
		s = s + sprintf ( s,
						  "#define %s\n",
						  property->name.c_str () );
	}
	else if ( alternativeName != NULL )
	{
		s = s + sprintf ( s,
						  "#define %s\n",
						  alternativeName->c_str () );
	}
}

void
Project::SetConfigurationOption ( char* s,
								  string name )
{
	SetConfigurationOption ( s, name, NULL );
}

void
Project::WriteConfigurationFile ()
{
	char* buf;
	char* s;

	buf = (char*) malloc ( 10*1024 );
	if ( buf == NULL )
		throw OutOfMemoryException ();

	s = buf;
	s = s + sprintf ( s, "/* Automatically generated. " );
	s = s + sprintf ( s, "Edit config.xml to change configuration */\n" );
	s = s + sprintf ( s, "#ifndef __INCLUDE_CONFIG_H\n" );
	s = s + sprintf ( s, "#define __INCLUDE_CONFIG_H\n" );

	SetConfigurationOption ( s, "ARCH" );
	SetConfigurationOption ( s, "OPTIMIZED" );
	SetConfigurationOption ( s, "MP", new string ( "UP" ) );
	SetConfigurationOption ( s, "ACPI" );
	SetConfigurationOption ( s, "_3GB" );

	s = s + sprintf ( s, "#endif /* __INCLUDE_CONFIG_H */\n" );

#ifdef _ROS_
	FileSupportCode::WriteIfChanged ( buf, "include" + sSep + "roscfg.h" );
#endif

	free ( buf );
}

void
Project::ExecuteInvocations ()
{
	fprintf( stderr, "ExecuteInvocations\n" );
	for( size_t i = 0; i < modules.size (); i++ )
		modules[i]->InvokeModule ();
}

void
Project::ReadXml ()
{
	Path path;

	RbuildDoc* pDoc = XMLLoadFile(xmlfile, path);

	XMLPos pos(pDoc);

//	pDoc->write(std::cout);

	if (pos.go("rbuild/project")) {
		ProcessXML(pos, "");
	} else
		throw XMLInvalidBuildFileException (
			pos->get_location(),
			"Document contains no 'project' tag." );
}

void
Project::ProcessXML(XMLPos& pos, const string& path)
{
	size_t i;

	if ( *pos != "project" )
		throw Exception ( "internal tool error: Project::ProcessXML() called with non-<project> node" );

	name = get_attribute(pos, "name", "Unnamed");

	makefile = get_required_attribute(pos, "makefile");

	const XMLNode::Children& children = pos->get_children();
	for(XMLNode::Children::const_iterator it=children.begin(); it!=children.end(); ++it)
	{
		ParseContext parseContext;
		ProcessXMLSubElement(**it, path, parseContext);
	}

	non_if_data.ProcessXML();

	non_if_data.ExtractModules( modules );

	for(i = 0; i < non_if_data.ifs.size (); i++)
	{
		const Property *property =
			LookupProperty( non_if_data.ifs[i]->property );

		if( !property ) continue;

		bool conditionTrue =
			(non_if_data.ifs[i]->negated &&
			 (property->value != non_if_data.ifs[i]->value)) ||
			(property->value == non_if_data.ifs[i]->value);
		if ( conditionTrue )
			non_if_data.ifs[i]->data.ExtractModules( modules );
		else
		{
			If * if_data = non_if_data.ifs[i];
			non_if_data.ifs.erase ( non_if_data.ifs.begin () + i );
			delete if_data;
			i--;
		}
	}
	for( i = 0; i < linkerFlags.size (); i++ )
		linkerFlags[i]->ProcessXML ();
	for( i = 0; i < modules.size (); i++ )
		modules[i]->ProcessXML (path);
	for( i = 0; i < cdfiles.size (); i++ )
		cdfiles[i]->ProcessXML ();
	for( i = 0; i < installfiles.size (); i++ )
		installfiles[i]->ProcessXML ();
}

static XMLNode*
XMLLoadInclude (
	XMLInclude& include,
	XMLIncludes& includes,
	std::stack<RbuildDoc*>& docs,
	const Path& path )
{
	const string& href = get_required_attribute(include.e, "href");

	string filename ( include.path.Fixup(href, true) );
/* top_href is not used anywhere.
	string top_file ( Path::RelativeFromWorkingDirectory ( filename ) );
	include.e->put("top_href", top_file);
*/
	RbuildDoc* pDoc = new RbuildDoc;

	Path path2(include.path, href);

	if (pDoc->read(filename, includes, path2))
	{
		// include file OK
		include.fileExists = true;

		 // copy XML data into new node
		pDoc->assign("xi:included");

		return pDoc;
	}
	else
	{
		docs.push(pDoc);

		 // problems reading the include file?
		if (pDoc->_last_error != XML_ERROR_NO_ELEMENTS)
			throw Exception(pDoc->_last_error_msg.c_str());

		// The include file doesn't exist or it is empty.
		include.fileExists = false;

		// look for xi:fallback element
		XMLPos pos(include.e);

		if (pos.go_down("xi:fallback")) {
			// now look for xi:include below...
			if (pos.go_down("xi:include")) {
				const string& href2 = get_required_attribute(pos, "href");

				string includeFile = include.path.Fixup(href2, true);
				string topIncludeFile = Path::RelativeFromWorkingDirectory(includeFile);
				XMLInclude* fallbackInclude = new XMLInclude(pos, include.parentNode, include.path, topIncludeFile);

				return XMLLoadInclude(*fallbackInclude, includes, docs, path);
			} else
				throw XMLInvalidBuildFileException (
					pos->get_location(),
					"<xi:fallback> must have a <xi:include> sub-element" );
		}

		return NULL;
	}
}

RbuildDoc* Project::XMLLoadFile(const string& filename, const Path& path)
{
	XMLIncludes new_includes;

	RbuildDoc* pDoc = new RbuildDoc;
	docs.push(pDoc);

	if (!pDoc->read(filename.c_str(), new_includes, path))
		throw Exception(pDoc->_last_error_msg.c_str());

	while(!new_includes.empty())
	{
		XMLInclude include = new_includes.top();
		new_includes.pop();

		XMLNode* e = include.e;

		XMLNode* e2 = XMLLoadInclude(include, new_includes, docs, path);
		if (!e2)	// could not load neither the include file nor its fallback include
			throw XMLFileNotFoundException(e->get_location(), include.topIncludeFilename);

		if (!include.parentNode)
			throw XMLException(e->get_location(), "internal tool error: xi:include doesn't have a parent");

		XMLNode::Children& children = include.parentNode->get_children();
		XMLNode::Children::iterator it2 = children.begin();
		for(; it2!=children.end(); ++it2) {
			if (*it2 == e)
				break;
		}

		if (it2 == children.end())
			throw XMLException(e->get_location(), "internal tool error: couldn't find xi:include in parent's sub-elements");

		e2->assign(*e); // copy node name
		e2->get_attributes() = e->get_attributes(); // copy node attributes

		 // replace inclusion tree with the imported tree
		children.erase(it2);
		children.push_back(e2);

		include.e = NULL;
		delete e;

		xmlbuildfiles.push(include);
	}

	return pDoc;
}

void
Project::ProcessXMLSubElement ( const XMLNode& e,
								const string& path,
								ParseContext& parseContext )
{
	If* pOldIf = parseContext.ifData;

	string subpath = path;

	if (!e.empty())
	{
		if ( e == "module" )
		{
			Module* module = new Module ( *this, e, path );
			if ( LocateModule ( module->name ) )
				throw XMLInvalidBuildFileException (
					e.get_location(),
					"module name conflict: '%s' (originally defined at %s)",
					module->name.c_str(),
					module->node.get_location().str().c_str() );
			if ( parseContext.ifData )
				parseContext.ifData->data.modules.push_back( module );
			else
				non_if_data.modules.push_back ( module );

			ensure_empty_content(e);

			return; // defer processing until later
		}
		else if ( e == "cdfile" )
		{
			CDFile* cdfile = new CDFile ( *this, e, path );
			cdfiles.push_back ( cdfile );
			ensure_no_children(e);
		}
		else if ( e == "installfile" )
		{
			InstallFile* installfile = new InstallFile ( *this, e, path );
			installfiles.push_back ( installfile );
			ensure_no_children(e);
		}
		else if ( e == "directory" )
		{
			subpath = GetSubPath(e.get_location(), path, get_required_attribute(&e, "name"));
			ensure_empty_content(e);
		}
		else if ( e == "include" )
		{
			Include* include = new Include ( *this, &e );
			if ( parseContext.ifData )
				parseContext.ifData->data.includes.push_back ( include );
			else
				non_if_data.includes.push_back ( include );
			ensure_no_children(e);
		}
		else if ( e == "define" )
		{
			Define* define = new Define ( *this, e );
			if ( parseContext.ifData )
				parseContext.ifData->data.defines.push_back ( define );
			else
				non_if_data.defines.push_back ( define );
			ensure_no_children(e);
		}
		else if ( e == "compilerflag" )
		{
			CompilerFlag* pCompilerFlag = new CompilerFlag ( *this, e );
			if ( parseContext.ifData )
				parseContext.ifData->data.compilerFlags.push_back ( pCompilerFlag );
			else
				non_if_data.compilerFlags.push_back ( pCompilerFlag );
			ensure_empty_attributes(e);
			ensure_no_children(e);
		}
		else if ( e == "linkerflag" )
		{
			linkerFlags.push_back ( new LinkerFlag ( *this, e ) );
			ensure_empty_attributes(e);
			ensure_no_children(e);
		}
		else if ( e == "if" )
		{
			parseContext.ifData = new If ( e, *this, NULL );
			if ( pOldIf )
				pOldIf->data.ifs.push_back ( parseContext.ifData );
			else
				non_if_data.ifs.push_back ( parseContext.ifData );
			ensure_empty_content(e);
		}
		else if ( e == "ifnot" )
		{
			parseContext.ifData = new If ( e, *this, NULL, true );
			if ( pOldIf )
				pOldIf->data.ifs.push_back ( parseContext.ifData );
			else
				non_if_data.ifs.push_back ( parseContext.ifData );
			ensure_empty_content(e);
		}
		else if ( e == "property" )
		{
			Property* property = new Property ( e, *this, NULL );
			if ( parseContext.ifData )
				parseContext.ifData->data.properties.push_back ( property );
			else
				non_if_data.properties.push_back ( property );
			ensure_empty_content(e);
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
	}

	 // recurse into depth
	const XMLNode::Children& children = e.get_children();
	for(XMLNode::Children::const_iterator it=children.begin(); it!=children.end(); ++it)
		ProcessXMLSubElement ( **it, subpath, parseContext );

	parseContext.ifData = pOldIf;
}

Module*
Project::LocateModule ( const string& name )
{
	for( size_t i = 0; i < modules.size (); i++ )
	{
		if (modules[i]->name == name)
			return modules[i];
	}

	return NULL;
}

const Module*
Project::LocateModule ( const string& name ) const
{
	for( size_t i = 0; i < modules.size (); i++ )
	{
		if ( modules[i]->name == name )
			return modules[i];
	}

	return NULL;
}

std::string
Project::GetProjectFilename () const
{
	return xmlfile;
}
