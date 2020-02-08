#include "fnv1a.h"

#include <cstdint>


#ifdef _M_IX86
static constexpr std::uint32_t const s_fnv1_offset = 2166136261uLL;
static constexpr std::uint32_t const s_fnv1_prime = 16777619uLL;
#else
#ifdef _M_X64
static constexpr std::uint64_t const s_fnv1_offset = 14695981039346656037uLL;
static constexpr std::uint64_t const s_fnv1_prime = 1099511628211uLL;
#else
#error Unknown architecture.
#endif
#endif


void fnv1a_hash_init(fnv1a_state& state)
{
	state.m_state = s_fnv1_offset;
}

void fnv1a_hash_process(fnv1a_state& state, void const* const ptr, int const len)
{
	std::uint8_t const* const data = static_cast<std::uint8_t const*>(ptr);
	for(int i = 0; i != len; ++i)
	{
		state.m_state ^= data[i];
		state.m_state *= s_fnv1_prime;
	}
}

std::size_t fnv1a_hash_finish(fnv1a_state const& state)
{
	return state.m_state;
}
