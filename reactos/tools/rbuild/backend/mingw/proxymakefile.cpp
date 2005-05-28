#include "../../pch.h"

#include "mingw.h"
#include <assert.h>

using std::string;
using std::vector;

ProxyMakefile::ProxyMakefile ( const Project& project )
	: project ( project )
{
}

ProxyMakefile::~ProxyMakefile ()
{
}

void
ProxyMakefile::GenerateProxyMakefiles ( bool verbose )
{
	for ( size_t i = 0; i < project.modules.size (); i++ )
	{
		GenerateProxyMakefileForModule ( *project.modules[i],
		                                 verbose );
	}
}

string
ProxyMakefile::GeneratePathToParentDirectory ( int numberOfParentDirectories )
{
	string path = "";
	for ( int i = 0; i < numberOfParentDirectories; i++ )
	{
		if ( path != "" )
			path += SSEP;
		path += "..";
	}
	return path;
}

string
ProxyMakefile::GetPathToTopDirectory ( Module& module )
{
	int numberOfDirectories = 1;
	string basePath = NormalizeFilename ( module.GetBasePath () );
	for ( size_t i = 0; i < basePath.length (); i++ )
	{
		if ( basePath[i] == CSEP )
			numberOfDirectories++;
	}
	return GeneratePathToParentDirectory ( numberOfDirectories );
}

void
ProxyMakefile::GenerateProxyMakefileForModule ( Module& module,
                                                bool verbose )
{
	char* buf;
	char* s;

	if ( verbose )
	{
		printf ( "\nGenerating proxy makefile for %s",
		         module.name.c_str () );
	}

	string proxyMakefile = NormalizeFilename ( module.GetBasePath () + SSEP "makefile" );
	string pathToTopDirectory = GetPathToTopDirectory ( module );
	string defaultTarget = module.name;

	buf = (char*) malloc ( 10*1024 );
	if ( buf == NULL )
		throw OutOfMemoryException ();

	s = buf;
	s = s + sprintf ( s, "# This file is automatically generated.\n" );
	s = s + sprintf ( s, "\n" );
	s = s + sprintf ( s, "TOP = %s\n", pathToTopDirectory.c_str () );
	s = s + sprintf ( s, "DEFAULT = %s\n", defaultTarget.c_str () );
	s = s + sprintf ( s, "include $(TOP)/proxy.mak\n" );

	FileSupportCode::WriteIfChanged ( buf, proxyMakefile );

	free ( buf );
}
