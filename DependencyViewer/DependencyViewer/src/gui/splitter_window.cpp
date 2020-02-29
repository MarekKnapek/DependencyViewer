#include "splitter_window.h"

#include "main.h"
#include "splitter_window_impl.h"

#include "../nogui/cassert_my.h"

#include <utility>


template<splitter_window_orientation orientation>
splitter_window<orientation>::splitter_window() noexcept :
	m_hwnd()
{
}

template<splitter_window_orientation orientation>
splitter_window<orientation>::splitter_window(HWND const& parent) :
	splitter_window()
{
	assert(parent != nullptr);
	m_hwnd = CreateWindowExW(0, splitter_window_impl<orientation>::get_class_atom(), nullptr, WS_CLIPCHILDREN | WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, parent, nullptr, get_instance(), nullptr);
	assert(m_hwnd != nullptr);
}

template<splitter_window_orientation orientation>
splitter_window<orientation>::splitter_window(splitter_window&& other) noexcept :
	splitter_window()
{
	swap(other);
}

template<splitter_window_orientation orientation>
splitter_window<orientation>& splitter_window<orientation>::operator=(splitter_window&& other) noexcept
{
	swap(other);
	return *this;
}

template<splitter_window_orientation orientation>
splitter_window<orientation>::~splitter_window()
{
}

template<splitter_window_orientation orientation>
void splitter_window<orientation>::swap(splitter_window& other) noexcept
{
	using std::swap;
	swap(m_hwnd, other.m_hwnd);
}

template<splitter_window_orientation orientation>
void splitter_window<orientation>::init()
{
	splitter_window_impl<orientation>::init();
}

template<splitter_window_orientation orientation>
void splitter_window<orientation>::deinit()
{
	splitter_window_impl<orientation>::deinit();
}

template<splitter_window_orientation orientation>
void splitter_window<orientation>::setchildren(HWND const& child_a, HWND const& child_b)
{
	UINT const msg = static_cast<std::uint32_t>(wm::wm_setchildren);
	WPARAM const wparam = reinterpret_cast<WPARAM>(child_a);
	LPARAM const lparam = reinterpret_cast<LPARAM>(child_b);
	assert(m_hwnd != nullptr);
	LRESULT const res = SendMessageW(m_hwnd, msg, wparam, lparam);
}

template<splitter_window_orientation orientation>
void splitter_window<orientation>::setposition(float const& position)
{
	UINT const msg = static_cast<std::uint32_t>(wm::wm_setposition);
	WPARAM const wparam = 0;
	LPARAM const lparam = reinterpret_cast<LPARAM>(&position);
	assert(m_hwnd != nullptr);
	LRESULT const res = SendMessageW(m_hwnd, msg, wparam, lparam);
}

template<splitter_window_orientation orientation>
HWND const& splitter_window<orientation>::get_hwnd() const
{
	return m_hwnd;
}


template class splitter_window<splitter_window_orientation::horizontal>;
template class splitter_window<splitter_window_orientation::vertical>;
