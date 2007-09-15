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
#include "backend/backend.h"

using std::string;
using std::vector;

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
	string defaultIntermediate =
		string( "obj-" ) + GetEnvironmentVariablePathOrDefault ( "ROS_CDOUTPUT", "i386" );
	return GetEnvironmentVariablePathOrDefault ( "ROS_INTERMEDIATE",
	                                             defaultIntermediate );
}

/* static */ string
Environment::GetOutputPath ()
{
	string defaultOutput =
		string( "output-" ) + GetEnvironmentVariablePathOrDefault ( "ROS_CDOUTPUT", "i386" );
	return GetEnvironmentVariablePathOrDefault ( "ROS_OUTPUT",
	                                             defaultOutput );
}

/* static */ string
Environment::GetInstallPath ()
{
	string defaultInstall =
		string( "reactos." ) + GetEnvironmentVariablePathOrDefault ( "ROS_CDOUTPUT", "" );
	return GetEnvironmentVariablePathOrDefault ( "ROS_INSTALL",
	                                             defaultInstall );
}

/* static */ string
Environment::GetCdOutputPath ()
{
	return GetEnvironmentVariablePathOrDefault ( "ROS_CDOUTPUT",
	                                             "reactos" );
}

/* static */ string
Environment::GetAutomakeFile ( const std::string& defaultFile )
{
	return GetEnvironmentVariablePathOrDefault ( "ROS_AUTOMAKE",
	                                             defaultFile );
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
                   const string& filename,
                   const std::map<std::string, std::string>* properties )
	: xmlfile (filename),
	  node (NULL),
	  head (NULL),
	  configuration (configuration)
{
	_backend = NULL;

	if ( properties )
	{
		std::map<string, string>::const_iterator it;
		for (it = properties->begin (); it != properties->end (); it++)
		{
			const Property *existing = LookupProperty( it->first );
			if ( !existing )
			{
				Property* property = new Property ( *this, NULL, it->first, it->second );
				non_if_data.properties.push_back (property );
			}
		}
	}

	ReadXml();
}

Project::~Project ()
{
	size_t i;
	if ( _backend )
		delete _backend;
#ifdef NOT_NEEDED_SINCE_THESE_ARE_CLEANED_BY_IFABLE_DATA
	for ( i = 0; i < modules.size (); i++ )
		delete modules[i];
#endif
	for ( i = 0; i < linkerFlags.size (); i++ )
		delete linkerFlags[i];
	for ( i = 0; i < cdfiles.size (); i++ )
		delete cdfiles[i];
	for ( i = 0; i < installfiles.size (); i++ )
		delete installfiles[i];
	delete head;
}

const Property*
Project::LookupProperty ( const string& name ) const
{
	for ( size_t i = 0; i < non_if_data.properties.size (); i++ )
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

	FileSupportCode::WriteIfChanged ( buf, Environment::GetIntermediatePath() + sSep + "include" + sSep + "roscfg.h" );

	free ( buf );
}

void
Project::ExecuteInvocations ()
{
	for ( size_t i = 0; i < modules.size (); i++ )
		modules[i]->InvokeModule ();
}

void
Project::ReadXml ()
{
	Path path;
	head = XMLLoadFile ( xmlfile, path, xmlbuildfiles );
	node = NULL;
	for ( size_t i = 0; i < head->subElements.size (); i++ )
	{
		if ( head->subElements[i]->name == "project" )
		{
			node = head->subElements[i];
			string path;
			ProcessXML ( path );
			return;
		}
	}

	if (node == NULL)
		node = head->subElements[0];

	throw XMLInvalidBuildFileException (
		node->location,
		"Document contains no 'project' tag." );
}

void
Project::ProcessXML ( const string& path )
{
	const XMLAttribute *att;
	if ( node->name != "project" )
		throw Exception ( "internal tool error: Project::ProcessXML() called with non-<project> node" );

	att = node->GetAttribute ( "name", false );
	if ( !att )
		name = "Unnamed";
	else
		name = att->value;

	att = node->GetAttribute ( "makefile", true );
	assert(att);
	makefile = Environment::GetAutomakeFile ( att->value );

	size_t i;
	for ( i = 0; i < node->subElements.size (); i++ )
	{
		ParseContext parseContext;
		ProcessXMLSubElement ( *node->subElements[i], path, parseContext );
	}
	
	non_if_data.ProcessXML ();

	non_if_data.ExtractModules( modules );

	for ( i = 0; i < non_if_data.ifs.size (); i++ )
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
	for ( i = 0; i < linkerFlags.size (); i++ )
		linkerFlags[i]->ProcessXML ();
	for ( i = 0; i < modules.size (); i++ )
		modules[i]->ProcessXML ();
	for ( i = 0; i < cdfiles.size (); i++ )
		cdfiles[i]->ProcessXML ();
	for ( i = 0; i < installfiles.size (); i++ )
		installfiles[i]->ProcessXML ();
}

void
Project::ProcessXMLSubElement ( const XMLElement& e,
                                const string& path,
                                ParseContext& parseContext )
{
	bool subs_invalid = false;
	If* pOldIf = parseContext.ifData;
	
	string subpath(path);
	if ( e.name == "module" )
	{
		Module* module = new Module ( *this, e, path );
		if ( LocateModule ( module->name ) )
			throw XMLInvalidBuildFileException (
				node->location,
				"module name conflict: '%s' (originally defined at %s)",
				module->name.c_str(),
				module->node.location.c_str() );
		if ( parseContext.ifData )
		    parseContext.ifData->data.modules.push_back( module );
		else
		    non_if_data.modules.push_back ( module );
		return; // defer processing until later
	}
	else if ( e.name == "cdfile" )
	{
		CDFile* cdfile = new CDFile ( *this, e, path );
		cdfiles.push_back ( cdfile );
		subs_invalid = true;
	}
	else if ( e.name == "installfile" )
	{
		InstallFile* installfile = new InstallFile ( *this, e, path );
		installfiles.push_back ( installfile );
		subs_invalid = true;
	}
	else if ( e.name == "directory" )
	{
		const XMLAttribute* att = e.GetAttribute ( "name", true );
		assert(att);
		subpath = GetSubPath ( *this, e.location, path, att->value );
	}
	else if ( e.name == "include" )
	{
		Include* include = new Include ( *this, &e );
		if ( parseContext.ifData )
			parseContext.ifData->data.includes.push_back ( include );
		else
			non_if_data.includes.push_back ( include );
		subs_invalid = true;
	}
	else if ( e.name == "define" )
	{
		Define* define = new Define ( *this, e );
		if ( parseContext.ifData )
			parseContext.ifData->data.defines.push_back ( define );
		else
			non_if_data.defines.push_back ( define );
		subs_invalid = true;
	}
	else if ( e.name == "compilerflag" )
	{
		CompilerFlag* pCompilerFlag = new CompilerFlag ( *this, e );
		if ( parseContext.ifData )
			parseContext.ifData->data.compilerFlags.push_back ( pCompilerFlag );
		else
			non_if_data.compilerFlags.push_back ( pCompilerFlag );
		subs_invalid = true;
	}
	else if ( e.name == "linkerflag" )
	{
		linkerFlags.push_back ( new LinkerFlag ( *this, e ) );
		subs_invalid = true;
	}
	else if ( e.name == "if" )
	{
		parseContext.ifData = new If ( e, *this, NULL );
		if ( pOldIf )
			pOldIf->data.ifs.push_back ( parseContext.ifData );
		else
			non_if_data.ifs.push_back ( parseContext.ifData );
		subs_invalid = false;
	}
	else if ( e.name == "ifnot" )
	{
		parseContext.ifData = new If ( e, *this, NULL, true );
		if ( pOldIf )
			pOldIf->data.ifs.push_back ( parseContext.ifData );
		else
			non_if_data.ifs.push_back ( parseContext.ifData );
		subs_invalid = false;
	}
	else if ( e.name == "property" )
	{
		Property* property = new Property ( e, *this, NULL );
		if ( parseContext.ifData )
			parseContext.ifData->data.properties.push_back ( property );
		else
			non_if_data.properties.push_back ( property );
	}
	if ( subs_invalid && e.subElements.size() )
	{
		throw XMLInvalidBuildFileException (
			e.location,
			"<%s> cannot have sub-elements",
			e.name.c_str() );
	}
	for ( size_t i = 0; i < e.subElements.size (); i++ )
		ProcessXMLSubElement ( *e.subElements[i], subpath, parseContext );

	parseContext.ifData = pOldIf;
}

Module*
Project::LocateModule ( const string& name )
{
	for ( size_t i = 0; i < modules.size (); i++ )
	{
		if (modules[i]->name == name)
			return modules[i];
	}

	return NULL;
}

const Module*
Project::LocateModule ( const string& name ) const
{
	for ( size_t i = 0; i < modules.size (); i++ )
	{
		if ( modules[i]->name == name )
			return modules[i];
	}

	return NULL;
}

const std::string&
Project::GetProjectFilename () const
{
	return xmlfile;
}
