#ifndef MINGW_MODULEHANDLER_H
#define MINGW_MODULEHANDLER_H

#include "../backend.h"

class MingwModuleHandler
{
public:
	static std::map<ModuleType,MingwModuleHandler*>* handler_map;
	static int ref;

	MingwModuleHandler ( ModuleType moduletype );
	virtual ~MingwModuleHandler();

	static void SetMakefile ( FILE* f );
	static MingwModuleHandler* LookupHandler ( const std::string& location,
	                                           ModuleType moduletype_ );
	virtual void Process ( const Module& module ) = 0;

protected:
	std::string GetWorkingDirectory () const;
	std::string GetExtension ( const std::string& filename ) const;
	std::string ReplaceExtension ( const std::string& filename,
	                               const std::string& newExtension ) const;
	std::string GetModuleArchiveFilename ( const Module& module ) const;
	std::string GetImportLibraryDependencies ( const Module& module ) const;
	std::string GetModuleDependencies ( const Module& module ) const;
	std::string GetAllDependencies ( const Module& module ) const;
	std::string GetSourceFilenames ( const Module& module ) const;

	std::string GetObjectFilename ( const std::string& sourceFilename ) const;
	std::string GetObjectFilenames ( const Module& module ) const;
	void GenerateMacrosAndTargetsHost ( const Module& module ) const;
	void GenerateMacrosAndTargetsTarget ( const Module& module ) const;
	std::string GetInvocationDependencies ( const Module& module ) const;
	std::string GetInvocationParameters ( const Invoke& invoke ) const;
	void GenerateInvocations ( const Module& module ) const;
	void GeneratePreconditionDependencies ( const Module& module ) const;
	std::string GenerateMacros ( const Module& module,
	                             const std::string& cflags_macro,
	                             const std::string& objs_macro ) const;
	static FILE* fMakefile;
private:
	std::string ConcatenatePaths ( const std::string& path1,
	                               const std::string& path2 ) const;
	std::string GenerateGccDefineParametersFromVector ( const std::vector<Define*>& defines ) const;
	std::string GenerateGccDefineParameters ( const Module& module ) const;
	std::string GenerateGccIncludeParametersFromVector ( const std::vector<Include*>& includes ) const;
	void GenerateMacros ( const Module& module,
	                      const char* op,
	                      const std::vector<File*>& files,
	                      const std::vector<Include*>* includes,
	                      const std::vector<Define*>& defines,
	                      const std::string& cflags_macro,
	                      const std::string& nasmflags_macro,
	                      const std::string& objs_macro) const;
	void GenerateMacros ( const Module& module,
	                      const std::string& cflags_macro,
	                      const std::string& nasmflags_macro,
	                      const std::string& objs_macro) const;
	void GenerateGccModuleIncludeVariable ( const Module& module ) const;
	std::string GenerateGccIncludeParameters ( const Module& module ) const;
	std::string GenerateGccParameters ( const Module& module ) const;
	std::string GenerateNasmParameters ( const Module& module ) const;
	std::string GenerateGccCommand ( const Module& module,
	                                 const std::string& sourceFilename,
	                                 const std::string& cc,
	                                 const std::string& cflagsMacro ) const;
	std::string GenerateGccAssemblerCommand ( const Module& module,
	                                          const std::string& sourceFilename,
	                                          const std::string& cc,
	                                          const std::string& cflagsMacro ) const;
	std::string GenerateNasmCommand ( const Module& module,
	                                  const std::string& sourceFilename,
	                                  const std::string& nasmflagsMacro ) const;
	std::string GenerateCommand ( const Module& module,
	                              const std::string& sourceFilename,
	                              const std::string& cc,
	                              const std::string& cflagsMacro,
	                              const std::string& nasmflagsMacro ) const;
	void GenerateObjectFileTargets ( const Module& module,
	                                 const std::vector<File*>& files,
	                                 const std::string& cc,
	                                 const std::string& cflagsMacro,
	                                 const std::string& nasmflagsMacro ) const;
	void GenerateObjectFileTargets ( const Module& module,
	                                 const std::string& cc,
	                                 const std::string& cflagsMacro,
	                                 const std::string& nasmflagsMacro ) const;
	void GenerateArchiveTarget ( const Module& module,
	                             const std::string& ar,
	                             const std::string& objs_macro ) const;
	void GenerateMacrosAndTargets ( const Module& module,
	                                const std::string& cc,
	                                const std::string& ar ) const;
	std::string GetPreconditionDependenciesName ( const Module& module ) const;
};


class MingwBuildToolModuleHandler : public MingwModuleHandler
{
public:
	MingwBuildToolModuleHandler ();
	virtual void Process ( const Module& module );
private:
	void GenerateBuildToolModuleTarget ( const Module& module );
};


class MingwKernelModuleHandler : public MingwModuleHandler
{
public:
	MingwKernelModuleHandler ();
	virtual void Process ( const Module& module );
private:
	void GenerateKernelModuleTarget ( const Module& module );
};


class MingwStaticLibraryModuleHandler : public MingwModuleHandler
{
public:
	MingwStaticLibraryModuleHandler ();
	virtual void Process ( const Module& module );
private:
	void GenerateStaticLibraryModuleTarget ( const Module& module );
};


class MingwKernelModeDLLModuleHandler : public MingwModuleHandler
{
public:
	MingwKernelModeDLLModuleHandler ();
	virtual void Process ( const Module& module );
private:
	void GenerateKernelModeDLLModuleTarget ( const Module& module );
};


class MingwNativeDLLModuleHandler : public MingwModuleHandler
{
public:
	MingwNativeDLLModuleHandler ();
	virtual void Process ( const Module& module );
private:
	void GenerateNativeDLLModuleTarget ( const Module& module );
};

#endif /* MINGW_MODULEHANDLER_H */
