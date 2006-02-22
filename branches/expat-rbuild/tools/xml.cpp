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

#include "rbuild/rbuild.h"
#include "xml.h"

using std::string;
using std::vector;
using namespace XMLStorage;


XMLException::XMLException (
	const XMLLocation& location,
	const char* format, ... )
{
	va_list args;
	va_start ( args, format );
	SetExceptionV ( location, format, args );
	va_end ( args );
}

void XMLException::SetExceptionV ( const XMLLocation& location, const char* format, va_list args )
{
	_e = location.str() + ": " + ssvprintf(format,args);
}

void XMLException::SetException ( const XMLLocation& location, const char* format, ... )
{
	va_list args;
	va_start ( args, format );
	SetExceptionV ( location, format, args );
	va_end ( args );
}


string get_attribute(const XMLNode* node, const string& attr_name, const string& default_value)
{
	const XMLNode::AttributeMap& attributes = node->get_attributes();

	XMLNode::AttributeMap::const_iterator found = attributes.find(attr_name);

	if (found != attributes.end())
		return found->second;
	else
		return string();
}

string get_required_attribute(const XMLNode* node, const string& attr_name)
{
	const XMLNode::AttributeMap& attributes = node->get_attributes();

	XMLNode::AttributeMap::const_iterator found = attributes.find(attr_name);

	if (found == attributes.end())
		throw XMLRequiredAttributeNotFoundException(node->get_location(), attr_name, *node);

	return found->second;
}

 // check for non-empty XML nodes
void ensure_empty_content(const XMLNode& node)
{
	const string& cont = node.get_content();
	bool empty = true;

	for(const char* p=cont.c_str(); *p; ++p)
		if (!isspace(*p)) {
			empty = false;
			break;
		}

	if (!empty)
		throw XMLInvalidBuildFileException(node.get_location(), "unexpected content in <%s> node: \"%s\"", node.c_str(), cont.c_str());
}

 // check for unexpected attributes
void ensure_empty_attributes(const XMLNode& node)
{
	if (!node.get_attributes().empty())
		throw XMLInvalidBuildFileException(node.get_location(), "unexpected attributes in <%s> node", node.c_str());
}

 // check for unexpected children elements
void ensure_no_children(const XMLNode& node)
{
	if (!node.get_children().empty())
		throw XMLInvalidBuildFileException(node.get_location(), "<%s> cannot have sub-elements", node.c_str());
}
