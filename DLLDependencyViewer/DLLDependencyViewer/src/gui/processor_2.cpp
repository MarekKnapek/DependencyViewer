#include "processor_2.h"


#include <cstring>
#include <type_traits>


template<typename T>
inline constexpr bool is_simple_type_v =
	std::is_trivial_v                        	<T> &&
	std::is_trivially_copyable_v             	<T> &&
	std::is_standard_layout_v                	<T> &&
	std::is_pod_v                            	<T> &&
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


static_assert(is_simple_type_v<file_info_2>, "");


void init(file_info_2* const fi)
{
	init(fi, 1);
}

void init(file_info_2* const fi, int const count)
{
	std::memset(fi, 0, count * sizeof(*fi));
}
