#include "memory_manager.h"

#include <utility>


memory_manager::memory_manager() noexcept :
	m_alc(),
	m_strs(),
	m_wstrs()
{
}

memory_manager::memory_manager(memory_manager&& other) noexcept :
	memory_manager()
{
	swap(other);
}

memory_manager& memory_manager::operator=(memory_manager&& other) noexcept
{
	swap(other);
	return *this;
}

memory_manager::~memory_manager() noexcept
{
}

void memory_manager::swap(memory_manager& other) noexcept
{
	using std::swap;
	swap(m_alc, other.m_alc);
	swap(m_strs, other.m_strs);
	swap(m_wstrs, other.m_wstrs);
}
