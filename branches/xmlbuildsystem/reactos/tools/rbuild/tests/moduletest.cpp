#include "test.h"

using std::string;

void ModuleTest::Run()
{
	string projectFilename ( RBUILD_BASE "tests/data/module.xml" );
	Project project ( projectFilename );
	ARE_EQUAL(2, project.modules.size());

	Module& module1 = *project.modules[0];
	IS_TRUE(module1.type == BuildTool);
	ARE_EQUAL(2, module1.files.size());
	ARE_EQUAL("." SSEP "dir1" SSEP "file1.c", module1.files[0]->name);
	ARE_EQUAL("." SSEP "dir1" SSEP "file2.c", module1.files[1]->name);

	ARE_EQUAL(0, module1.libraries.size());

	Module& module2 = *project.modules[1];
	IS_TRUE(module2.type == KernelModeDLL);
	ARE_EQUAL(2, module2.files.size());
	ARE_EQUAL("." SSEP "dir2" SSEP "file3.c", module2.files[0]->name);
	ARE_EQUAL("." SSEP "dir2" SSEP "file4.c", module2.files[1]->name);

	ARE_EQUAL(1, module2.libraries.size());
	Library& library1 = *module2.libraries[0];
	ARE_EQUAL("module1", library1.name);

	ARE_EQUAL(1, module2.dependencies.size());
	Dependency& module1dependency = *module2.dependencies[0];
	IS_NOT_NULL(module1dependency.dependencyModule);
	ARE_EQUAL("module1", module1dependency.dependencyModule->name);
}
