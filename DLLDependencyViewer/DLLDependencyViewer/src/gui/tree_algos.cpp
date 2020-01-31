#include "tree_algos.h"

#include "processor.h"

#include <cstdint>


/*
*
*                                ( - )
*                  +---------------+---------------+
*                ( 1 )                           ( 8 )
*          +-------+-------+               +-------+-------+
*        ( 2 )           ( 5 )           ( 9 )           ( 12)
*      +---+---+       +---+---+       +---+---+       +---+---+
*    ( 3 )   ( 4 )   ( 6 )   ( 7 )   ( 10)   ( 11)   ( 13)   ( 14)
*
*/
void depth_first_visit(file_info& fi, void(*const callback_fn)(file_info& fi, void* const data), void* const data)
{
	std::uint16_t const n = fi.m_import_table.m_normal_dll_count + fi.m_import_table.m_delay_dll_count;
	for(std::uint16_t i = 0; i != n; ++i)
	{
		file_info& child_fi = fi.m_fis[i];
		callback_fn(child_fi, data);
		depth_first_visit(child_fi, callback_fn, data);
	}
}

/*
*
*                                ( - )
*                  +---------------+---------------+
*                ( 7 )                           ( 14)
*          +-------+-------+               +-------+-------+
*        ( 3 )           ( 6 )           ( 10)           ( 13)
*      +---+---+       +---+---+       +---+---+       +---+---+
*    ( 1 )   ( 2 )   ( 4 )   ( 5 )   ( 8 )   ( 9 )   ( 11)   ( 12)
*
*/
void children_first_visit(file_info& fi, void(*const callback_fn)(file_info& fi, void* const data), void* const data)
{
	std::uint16_t const n = fi.m_import_table.m_normal_dll_count + fi.m_import_table.m_delay_dll_count;
	for(std::uint16_t i = 0; i != n; ++i)
	{
		file_info& child_fi = fi.m_fis[i];
		children_first_visit(child_fi, callback_fn, data);
		callback_fn(child_fi, data);
	}
}
