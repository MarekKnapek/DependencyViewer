#pragma once


#include <cstddef>


struct fnv1a_state
{
	std::size_t m_state;
};


void fnv1a_hash_init(fnv1a_state& state);
void fnv1a_hash_process(fnv1a_state& state, void const* const ptr, int const len);
std::size_t fnv1a_hash_finish(fnv1a_state const& state);
