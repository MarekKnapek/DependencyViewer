#include "tree_algos.h"

#include "processor.h"

#include "../nogui/cassert_my.h"

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
void depth_first_visit(file_info const* const& fi, void(*const& callback_fn)(file_info const* const& fi, void* const& param), void* const& param)
{
	assert(fi);
	assert(callback_fn);
	std::uint16_t const n = fi->m_import_table.m_normal_dll_count + fi->m_import_table.m_delay_dll_count;
	for(std::uint16_t i = 0; i != n; ++i)
	{
		file_info* const& child_fi = fi->m_fis + i;
		assert(child_fi);
		callback_fn(child_fi, param);
		depth_first_visit(child_fi, callback_fn, param);
	}
}

void depth_first_visit(file_info* const& fi, void(*const& callback_fn)(file_info* const& fi, void* const& param), void* const& param)
{
	struct tuple_my
	{
		void(*const& m_callback)(file_info* const& fi, void* const& param);
		void* const& m_param;
	};
	static constexpr auto const fn = []([[maybe_unused]] file_info const* const& fi, void* const& param)
	{
		assert(param);
		tuple_my const* const p = static_cast<tuple_my const*>(param);
		file_info* const fi_ = const_cast<file_info*>(fi);
		p->m_callback(fi_, p->m_param);
	};
	tuple_my p{callback_fn, param};
	depth_first_visit(fi, fn, &p);
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
void children_first_visit(file_info const* const& fi, void(*const& callback_fn)(file_info const* const& fi, void* const& param), void* const& param)
{
	assert(fi);
	assert(callback_fn);
	std::uint16_t const n = fi->m_import_table.m_normal_dll_count + fi->m_import_table.m_delay_dll_count;
	for(std::uint16_t i = 0; i != n; ++i)
	{
		file_info const* const& child_fi = fi->m_fis + i;
		assert(child_fi);
		children_first_visit(child_fi, callback_fn, param);
		callback_fn(child_fi, param);
	}
}

void children_first_visit(file_info* const& fi, void(*const& callback_fn)(file_info* const& fi, void* const& param), void* const& param)
{
	struct tuple_my
	{
		void(*const& m_callback)(file_info* const& fi, void* const& param);
		void* const& m_param;
	};
	static constexpr auto const fn = []([[maybe_unused]] file_info const* const& fi, void* const& param)
	{
		assert(param);
		tuple_my const* const p = static_cast<tuple_my const*>(param);
		file_info* const fi_ = const_cast<file_info*>(fi);
		p->m_callback(fi_, p->m_param);
	};
	tuple_my p{callback_fn, param};
	children_first_visit(fi, fn, &p);
}
