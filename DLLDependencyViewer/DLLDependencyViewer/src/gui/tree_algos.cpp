#include "tree_algos.h"

#include "processor.h"

#include <cstdint>


void breadth_first_visit(file_info& fi, void(*const callback_fn)(file_info& fi, void* const data), void* const data)
{
	std::uint16_t const n = fi.m_import_table.m_dll_count;
	for(std::uint16_t i = 0; i != n; ++i)
	{
		file_info& child_fi = fi.m_fis[i];
		callback_fn(child_fi, data);
	}
	for(std::uint16_t i = 0; i != n; ++i)
	{
		file_info& child_fi = fi.m_fis[i];
		depth_first_visit(child_fi, callback_fn, data);
	}
}

void depth_first_visit(file_info& fi, void(*const callback_fn)(file_info& fi, void* const data), void* const data)
{
	std::uint16_t const n = fi.m_import_table.m_dll_count;
	for(std::uint16_t i = 0; i != n; ++i)
	{
		file_info& child_fi = fi.m_fis[i];
		callback_fn(child_fi, data);
		depth_first_visit(child_fi, callback_fn, data);
	}
}

void children_first_visit(file_info& fi, void(*const callback_fn)(file_info& fi, void* const data), void* const data)
{
	std::uint16_t const n = fi.m_import_table.m_dll_count;
	for(std::uint16_t i = 0; i != n; ++i)
	{
		file_info& child_fi = fi.m_fis[i];
		children_first_visit(child_fi, callback_fn, data);
		callback_fn(child_fi, data);
	}
}
