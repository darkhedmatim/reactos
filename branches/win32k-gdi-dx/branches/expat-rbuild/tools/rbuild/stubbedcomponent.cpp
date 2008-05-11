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

StubbedComponent::StubbedComponent ( const Module* module_,
									 const XMLNode& stubbedComponentNode )
	: module(module_),
	  node(stubbedComponentNode)
{
	name = node.get("name");
}

StubbedComponent::~StubbedComponent ()
{
	for ( size_t i = 0; i < symbols.size(); i++ )
		delete symbols[i];
}

void
StubbedComponent::ProcessXML ()
{
	const XMLNode::Children& children = node.get_children();

	for(XMLNode::Children::const_iterator it=children.begin(); it!=children.end(); ++it)
		ProcessXMLSubElement(**it);

	for (size_t i = 0; i < symbols.size (); i++ )
		symbols[i]->ProcessXML ();
}

void
StubbedComponent::ProcessXMLSubElement ( const XMLNode& e )
{
	bool subs_invalid = false;
	if ( e == "symbol" )
	{
		symbols.push_back ( new StubbedSymbol ( e ) );
		subs_invalid = false;
	}

	if (subs_invalid && !e.get_children().empty())
	{
		throw XMLInvalidBuildFileException (
			e.get_location(),
			"<%s> cannot have sub-elements",
			e.c_str() );
	}

	const XMLNode::Children& children = e.get_children();

	for(XMLNode::Children::const_iterator it=children.begin(); it!=children.end(); ++it)
		ProcessXMLSubElement ( **it );
}



StubbedSymbol::StubbedSymbol ( const XMLNode& stubbedSymbolNode )
	: node(stubbedSymbolNode)
{
}

StubbedSymbol::~StubbedSymbol ()
{
}

void
StubbedSymbol::ProcessXML ()
{
	if (node.get_content().empty())
	{
		throw XMLInvalidBuildFileException (
			node.get_location(),
			"<symbol> is empty." );
	}
	symbol = node.get_content();

	strippedName = StripSymbol ( symbol );

	newname = get_attribute(&node, "newname", strippedName);
}

string
StubbedSymbol::StripSymbol ( string symbol )
{
	size_t start = 0;
	while ( start < symbol.length () && symbol[start] == '@')
		start++;
	size_t end = symbol.length () - 1;
	while ( end > 0 && isdigit ( symbol[end] ) )
		end--;
	if ( end > 0 && symbol[end] == '@' )
		end--;
	if ( end > 0 )
		return symbol.substr ( start, end - start + 1 );
	else
		return "";
}
