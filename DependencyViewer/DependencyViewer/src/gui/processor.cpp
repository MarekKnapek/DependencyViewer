#include "processor.h"

#include "processor_impl.h"

#include "../nogui/assert_my.h"
#include "../nogui/cassert_my.h"

#include <cstring>
#include <type_traits>
#include <utility>


template<typename T>
inline constexpr bool is_simple_type_v =
	std::is_trivial_v                        	<T> &&
	std::is_trivially_copyable_v             	<T> &&
	std::is_standard_layout_v                	<T> &&
	/*std::is_pod_v                            	<T> &&*/
	std::is_aggregate_v                      	<T> &&
	std::is_constructible_v                  	<T> &&
	std::is_trivially_constructible_v        	<T> &&
	std::is_nothrow_constructible_v          	<T> &&
	std::is_default_constructible_v          	<T> &&
	std::is_trivially_default_constructible_v	<T> &&
	std::is_nothrow_default_constructible_v  	<T> &&
	std::is_copy_constructible_v             	<T> &&
	std::is_trivially_copy_constructible_v   	<T> &&
	std::is_nothrow_copy_constructible_v     	<T> &&
	std::is_move_constructible_v             	<T> &&
	std::is_trivially_move_constructible_v   	<T> &&
	std::is_nothrow_move_constructible_v     	<T> &&
	std::is_copy_assignable_v                	<T> &&
	std::is_trivially_copy_assignable_v      	<T> &&
	std::is_nothrow_copy_assignable_v        	<T> &&
	std::is_move_assignable_v                	<T> &&
	std::is_trivially_move_assignable_v      	<T> &&
	std::is_nothrow_move_assignable_v        	<T> &&
	std::is_destructible_v                   	<T> &&
	std::is_trivially_destructible_v         	<T> &&
	std::is_nothrow_destructible_v           	<T> &&
	std::is_swappable_v                      	<T> &&
	std::is_nothrow_swappable_v              	<T>;


static_assert(is_simple_type_v<file_info>, "");


void init(file_info* const fi)
{
	init(fi, 1);
}

void init(file_info* const fi, int const count)
{
	std::memset(fi, 0, count * sizeof(*fi));
}

void main_type::swap(main_type& other) noexcept
{
	using std::swap;
	swap(m_fi, other.m_fi);
	swap(m_modules_list, other.m_modules_list);
	swap(m_mm, other.m_mm);
}


bool process(std::vector<std::wstring> const& file_paths, main_type* const mo_out)
{
	assert(mo_out);
	main_type mo;
	bool const processed = process_impl(file_paths, mo);
	WARN_M_R(processed, L"Failed to process_impl.", false);
	mo_out->swap(mo);
	return true;
}
