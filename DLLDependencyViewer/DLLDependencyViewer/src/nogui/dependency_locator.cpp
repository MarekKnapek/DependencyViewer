#include "dependency_locator.h"


bool locate_dependency(dependency_locator& self)
{
	if(locate_dependency_sxs(self)) return true;
	if(locate_dependency_known_dlls(self)) return true;
	if(locate_dependency_application_dir(self)) return true;
	if(locate_dependency_system32(self)) return true;
	if(locate_dependency_system16(self)) return true;
	if(locate_dependency_windows(self)) return true;
	if(locate_dependency_current_dir(self)) return true;
	if(locate_dependency_environment_path(self)) return true;
	return false;
}


bool locate_dependency_sxs(dependency_locator& self)
{
	return false;
}

bool locate_dependency_known_dlls(dependency_locator& self)
{
	return false;
}

bool locate_dependency_application_dir(dependency_locator& self)
{
	return false;
}

bool locate_dependency_system32(dependency_locator& self)
{
	return false;
}

bool locate_dependency_system16(dependency_locator& self)
{
	return false;
}

bool locate_dependency_windows(dependency_locator& self)
{
	return false;
}

bool locate_dependency_current_dir(dependency_locator& self)
{
	return false;
}

bool locate_dependency_environment_path(dependency_locator& self)
{
	return false;
}
