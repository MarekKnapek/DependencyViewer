#include "splitter_window2.h"

#include "main.h"
#include "splitter_window2_impl.h"

#include "../nogui/cassert_my.h"

#include <utility>


template<splitter_window2_orientation orientation>
splitter_window2<orientation>::splitter_window2() noexcept :
	m_hwnd()
{
}

template<splitter_window2_orientation orientation>
splitter_window2<orientation>::splitter_window2(HWND const& parent) :
	splitter_window2()
{
	assert(parent != nullptr);
	m_hwnd = CreateWindowExW(0, splitter_window2_impl<orientation>::get_class_atom(), nullptr, WS_CLIPCHILDREN | WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, parent, nullptr, get_instance(), nullptr);
	assert(m_hwnd != nullptr);
}

template<splitter_window2_orientation orientation>
splitter_window2<orientation>::splitter_window2(splitter_window2&& other) noexcept :
	splitter_window2()
{
	swap(other);
}

template<splitter_window2_orientation orientation>
splitter_window2<orientation>& splitter_window2<orientation>::operator=(splitter_window2&& other) noexcept
{
	swap(other);
	return *this;
}

template<splitter_window2_orientation orientation>
splitter_window2<orientation>::~splitter_window2()
{
}

template<splitter_window2_orientation orientation>
void splitter_window2<orientation>::swap(splitter_window2& other) noexcept
{
	using std::swap;
	swap(m_hwnd, other.m_hwnd);
}

template<splitter_window2_orientation orientation>
void splitter_window2<orientation>::init()
{
	splitter_window2_impl<orientation>::init();
}

template<splitter_window2_orientation orientation>
void splitter_window2<orientation>::deinit()
{
	splitter_window2_impl<orientation>::deinit();
}

template<splitter_window2_orientation orientation>
void splitter_window2<orientation>::setchildren(HWND const& child_a, HWND const& child_b)
{
	UINT const msg = static_cast<std::uint32_t>(wm::wm_setchildren);
	WPARAM const wparam = reinterpret_cast<WPARAM>(child_a);
	LPARAM const lparam = reinterpret_cast<LPARAM>(child_b);
	assert(m_hwnd != nullptr);
	LRESULT const res = SendMessageW(m_hwnd, msg, wparam, lparam);
}

template<splitter_window2_orientation orientation>
HWND const& splitter_window2<orientation>::get_hwnd() const
{
	return m_hwnd;
}


template class splitter_window2<splitter_window2_orientation::horizontal>;
template class splitter_window2<splitter_window2_orientation::vertical>;
