#include "file_system.h"

#include <filesystem>


bool file_exists(wchar_t const* const& file_path)
{
	return std::experimental::filesystem::exists(file_path);
}
