#ifndef __RBUILD_H
#define __RBUILD_H

#include "pch.h"

#include "ssprintf.h"
#include "exception.h"
#include "XML.h"

#ifdef WIN32
#define EXEPOSTFIX ".exe"
#define CSEP '\\'
#define CBAD_SEP '/'
#define SSEP "\\"
#define SBAD_SEP "/"
#else
#define EXEPOSTFIX ""
#define CSEP '/'
#define CBAD_SEP '\\'
#define SSEP "/"
#define SBAD_SEP "\\"
#endif

class Project;
class Module;
class Include;
class Define;
class File;
class Library;
class Invoke;
class InvokeFile;
class Dependency;
class ImportLibrary;
class If;
class LinkerFlag;
class Property;

class Project
{
	std::string xmlfile;
	XMLElement *node, *head;
public:
	std::string name;
	std::string makefile;
	std::vector<Module*> modules;
	std::vector<Include*> includes;
	std::vector<Define*> defines;
	std::vector<LinkerFlag*> linkerFlags;
	std::vector<Property*> properties;
	std::vector<If*> ifs;

	//Project ();
	Project ( const std::string& filename );
	~Project ();
	void ProcessXML ( const std::string& path );
	Module* LocateModule ( const std::string& name );
	const Module* LocateModule ( const std::string& name ) const;
private:
	void ReadXml ();
	void ProcessXMLSubElement ( const XMLElement& e,
	                            const std::string& path,
	                            If* pIf = NULL );

	// disable copy semantics
	Project ( const Project& );
	Project& operator = ( const Project& );
};


enum ModuleType
{
	BuildTool,
	StaticLibrary,
	Kernel,
	KernelModeDLL,
	KernelModeDriver,
	NativeDLL,
	Win32DLL,
	Win32GUI
};


class Module
{
public:
	const Project& project;
	const XMLElement& node;
	std::string name;
	std::string extension;
	std::string path;
	ModuleType type;
	ImportLibrary* importLibrary;
	std::vector<File*> files;
	std::vector<Library*> libraries;
	std::vector<Include*> includes;
	std::vector<Define*> defines;
	std::vector<Invoke*> invocations;
	std::vector<Dependency*> dependencies;
	std::vector<If*> ifs;
	std::vector<LinkerFlag*> linkerFlags;

	Module ( const Project& project,
	         const XMLElement& moduleNode,
	         const std::string& modulePath );
	~Module ();
	ModuleType GetModuleType ( const std::string& location,
	                           const XMLAttribute& attribute );
	bool HasImportLibrary () const;
	std::string GetTargetName () const;
	std::string GetDependencyPath () const;
	std::string GetBasePath () const;
	std::string GetPath () const;
	std::string GetPathWithPrefix ( const std::string& prefix ) const;
	std::string GetTargets () const;
	std::string GetInvocationTarget ( const int index ) const;
	void ProcessXML();
private:
	std::string GetDefaultModuleExtension () const;
	void ProcessXMLSubElement ( const XMLElement& e,
	                            const std::string& path,
	                            If* pIf = NULL );
};


class Include
{
public:
	const Project& project;
	const Module* module;
	const XMLElement& node;
	std::string directory;
	std::string basePath;

	Include ( const Project& project,
	          const XMLElement& includeNode );
	Include ( const Project& project,
	          const Module* module,
	          const XMLElement& includeNode );
	~Include ();
	void ProcessXML();
private:
	void Initialize();
};


class Define
{
public:
	const Project& project;
	const Module* module;
	const XMLElement& node;
	std::string name;
	std::string value;

	Define ( const Project& project,
	         const XMLElement& defineNode );
	Define ( const Project& project,
	         const Module* module,
	         const XMLElement& defineNode );
	~Define();
	void ProcessXML();
private:
	void Initialize();
};


class File
{
public:
	std::string name;
	bool first;

	File ( const std::string& _name, bool _first );

	void ProcessXML();
};


class Library
{
public:
	const XMLElement& node;
	const Module& module;
	std::string name;

	Library ( const XMLElement& _node,
	          const Module& _module,
	          const std::string& _name );

	void ProcessXML();
};


class Invoke
{
public:
	const XMLElement& node;
	const Module& module;
	const Module* invokeModule;
	std::vector<InvokeFile*> input;
	std::vector<InvokeFile*> output;

	Invoke ( const XMLElement& _node,
	         const Module& _module );

	void ProcessXML();
	std::string GetTargets () const;
private:
	void ProcessXMLSubElement ( const XMLElement& e );
	void ProcessXMLSubElementInput ( const XMLElement& e );
	void ProcessXMLSubElementOutput ( const XMLElement& e );
};


class InvokeFile
{
public:
	const XMLElement& node;
	std::string name;
	std::string switches;

	InvokeFile ( const XMLElement& _node,
	             const std::string& _name );

	void ProcessXML ();
};


class Dependency
{
public:
	const XMLElement& node;
	const Module& module;
	const Module* dependencyModule;

	Dependency ( const XMLElement& _node,
	             const Module& _module );

	void ProcessXML();
};


class ImportLibrary
{
public:
	const XMLElement& node;
	const Module& module;
	std::string basename;
	std::string definition;

	ImportLibrary ( const XMLElement& _node,
	                const Module& module );

	void ProcessXML ();
};


class If
{
public:
	const XMLElement& node;
	const Project& project;
	const Module* module;
	std::string property, value;
	std::vector<File*> files;
	std::vector<Include*> includes;
	std::vector<Define*> defines;
	std::vector<Property*> properties;
	std::vector<If*> ifs;

	If ( const XMLElement& node_,
	     const Project& project_,
	     const Module* module_ );
	~If();

	void ProcessXML();
};


class LinkerFlag
{
public:
	const Project& project;
	const Module* module;
	const XMLElement& node;
	std::string flag;

	LinkerFlag ( const Project& project,
	             const XMLElement& linkerFlagNode );
	LinkerFlag ( const Project& project,
	             const Module* module,
	             const XMLElement& linkerFlagNode );
	~LinkerFlag ();
	void ProcessXML();
private:
	void Initialize();
};


class Property
{
public:
	const XMLElement& node;
	const Project& project;
	const Module* module;
	std::string name, value;

	Property ( const XMLElement& node_,
	           const Project& project_,
	           const Module* module_ );

	void ProcessXML();
};

extern std::string
FixSeparator ( const std::string& s );

extern std::string
NormalizeFilename ( const std::string& filename );

#endif /* __RBUILD_H */
