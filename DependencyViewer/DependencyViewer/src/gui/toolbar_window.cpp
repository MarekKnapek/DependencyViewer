#include "toolbar_window.h"

#include "main.h"
#include "toolbar_window_impl.h"

#include "../nogui/cassert_my.h"

#include <utility>


toolbar_window::toolbar_window() noexcept :
	m_hwnd()
{
}

toolbar_window::toolbar_window(HWND const& parent) :
	toolbar_window()
{
	assert(parent != nullptr);
	m_hwnd = CreateWindowExW(0, toolbar_window_impl::get_class_atom(), nullptr, WS_CLIPCHILDREN | WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, parent, nullptr, get_instance(), nullptr);
	assert(m_hwnd != nullptr);
}

toolbar_window::toolbar_window(toolbar_window&& other) noexcept :
	toolbar_window()
{
	swap(other);
}

toolbar_window& toolbar_window::operator=(toolbar_window&& other) noexcept
{
	swap(other);
	return *this;
}

toolbar_window::~toolbar_window()
{
}

void toolbar_window::swap(toolbar_window& other) noexcept
{
	using std::swap;
	swap(m_hwnd, other.m_hwnd);
}

void toolbar_window::init()
{
	toolbar_window_impl::init();
}

void toolbar_window::deinit()
{
	toolbar_window_impl::deinit();
}

HWND const& toolbar_window::get_hwnd() const
{
	return m_hwnd;
}
