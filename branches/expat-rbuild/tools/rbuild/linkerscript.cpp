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
using XMLStorage::XMLNode;

LinkerScript::LinkerScript ( const Project& project,
							 const Module* module,
							 const XMLNode& node )
	: project ( project ),
	  module ( module ),
	  node ( node ),
	  baseModule ( NULL )
{
}

LinkerScript::~LinkerScript ()
{
}

void
LinkerScript::ProcessXML()
{
	const string& base_value = node.get("base");

	if (!base_value.empty())
	{
		bool referenceResolved = false;
		if ( base_value == project.name )
		{
			basePath = ".";
			referenceResolved = true;
		}
		else
		{
			const Module* base = project.LocateModule ( base_value );
			if ( base != NULL )
			{
				baseModule = base;
				basePath = base->GetBasePath ();
				referenceResolved = true;
			}
		}
		if ( !referenceResolved )
		{
			throw XMLInvalidBuildFileException (
				node.get_location(),
				"<linkerscript> attribute 'base' references non-existant project or module '%s'", base_value.c_str() );
		}
		directory = NormalizeFilename ( basePath + sSep + node.get_content() );
	}
	else
		directory = NormalizeFilename ( node.get_content() );
}
