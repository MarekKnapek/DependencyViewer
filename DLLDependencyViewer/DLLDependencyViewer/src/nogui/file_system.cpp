#include "file_system.h"

#include <windows.h>


bool file_exists(wchar_t const* const& file_path)
{
  DWORD const got_attrib = GetFileAttributesW(file_path);
	return (got_attrib != INVALID_FILE_ATTRIBUTES) && ((got_attrib & FILE_ATTRIBUTE_DIRECTORY) != 0);
}
