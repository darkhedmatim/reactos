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
#ifndef XML_H
#define XML_H

#ifndef XMLNODE_LOCATION
#define XMLNODE_LOCATION
#endif
#include "xmlstorage.h"
#include <vector>

class XMLException
{
public:
	XMLException ( const XMLStorage::XMLLocation& location, const char* format, ... );
	const std::string& operator *() { return _e; }

protected:
	XMLException() {}
	void SetExceptionV ( const XMLStorage::XMLLocation& location, const char* format, va_list args );
	void SetException ( const XMLStorage::XMLLocation& location, const char* format, ... );

private:
	std::string _e;
};


class XMLRequiredAttributeNotFoundException : public XMLException
{
public:
	XMLRequiredAttributeNotFoundException (
		const XMLStorage::XMLLocation& location,
		const std::string& attributeName,
		const std::string& elementName )
	{
		SetException ( location, "Required attribute '%s' not found in element '%s'",
			attributeName.c_str(),
			elementName.c_str() );
	}
};

class XMLInvalidBuildFileException : public XMLException
{
public:
	XMLInvalidBuildFileException (
		const XMLStorage::XMLLocation& location,
		const char* format,
		... )
	{
		va_list args;
		va_start ( args, format );
		SetExceptionV ( location, format, args );
		va_end ( args );
	}
};

class InvalidAttributeValueException : public XMLInvalidBuildFileException
{
public:
	InvalidAttributeValueException ( const XMLStorage::XMLLocation& location,
									 const std::string& name,
									 const std::string& value );
};

class XMLFileNotFoundException : public XMLException
{
public:
	XMLFileNotFoundException (
		const XMLStorage::XMLLocation& location,
		const std::string& filename )
	{
		SetException ( location, "Can't open file '%s'", filename.c_str() );
	}
};


class Path
{
	std::vector<std::string> path;

public:
	Path(); // initializes path to getcwd();
	Path ( const Path& cwd, const std::string& filename );
	std::string Fixup ( const std::string& filename, bool include_filename ) const;

	std::string RelativeFromWorkingDirectory ();
	static std::string RelativeFromWorkingDirectory ( const std::string& path );
	static std::string RelativeFromDirectory ( const std::string& path, const std::string& base_directory);

	static void Split (
		std::vector<std::string>& out,
		const std::string& path,
		bool include_last );
};


class XMLInclude
{
public:
	XMLStorage::XMLNode *e;
	XMLStorage::XMLNode *parentNode;
	Path path;
	std::string topIncludeFilename;
	bool fileExists;

	XMLInclude (
		XMLStorage::XMLNode* e_,
		XMLStorage::XMLNode* parentNode_,
		const Path& path_,
		const std::string topIncludeFilename_ )
		: e ( e_ ),
		parentNode (parentNode_),
		path ( path_ ),
		topIncludeFilename ( topIncludeFilename_ )
	{
	}
};

class XMLIncludes : public std::stack<XMLInclude>
{
};


 // customized XML reader to look for <xi:include> nodes
struct RBuildReader : public XMLStorage::XMLReader
{
	typedef XMLStorage::XMLReader super;

	RBuildReader(XMLStorage::XMLNode* node, std::istream& in, XMLIncludes& includes_, const Path& path_)
	 :	super(node, in),
		includes(includes_),
		path(path_),
		include_depth(0)
	{
	}

	void	StartElementHandler(const XMLStorage::XS_String& name, const XMLStorage::XMLNode::AttributeMap& attributes);
	void	EndElementHandler();

protected:
	XMLIncludes& includes;
	const Path& path;
	int 	include_depth;
};

struct RbuildDoc : public XMLStorage::XMLDoc
{
	typedef XMLStorage::XMLDoc super;

	bool read(const std::string& xml_path, XMLIncludes& includes, const Path& path_)
	{
		XMLStorage::tifstream in(xml_path.c_str());
		RBuildReader reader(this, in, includes, path_);

		return super::read(reader, XS_String(xml_path));
	}
};

extern std::string get_attribute(const XMLStorage::XMLNode* node, const std::string& attr_name, const std::string& default_value);
extern std::string get_required_attribute(const XMLStorage::XMLNode* node, const std::string& attr_name);
extern void ensure_empty_content(const XMLStorage::XMLNode& node);
extern void ensure_empty_attributes(const XMLStorage::XMLNode& node);
extern void ensure_no_children(const XMLStorage::XMLNode& node);


#endif // XML_H
