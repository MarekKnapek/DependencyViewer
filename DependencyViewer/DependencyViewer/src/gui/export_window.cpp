#include "export_window.h"

#include "main.h"
#include "export_window_impl.h"

#include "../nogui/cassert_my.h"

#include <utility>


export_window::export_window() noexcept :
	m_hwnd()
{
}

export_window::export_window(HWND const& parent) :
	export_window()
{
	assert(parent != nullptr);
	m_hwnd = CreateWindowExW(0, export_window_impl::get_class_atom(), nullptr, WS_CLIPCHILDREN | WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, parent, nullptr, get_instance(), nullptr);
	assert(m_hwnd != nullptr);
}

export_window::export_window(export_window&& other) noexcept :
	export_window()
{
	swap(other);
}

export_window& export_window::operator=(export_window&& other) noexcept
{
	swap(other);
	return *this;
}

export_window::~export_window()
{
}

void export_window::swap(export_window& other) noexcept
{
	using std::swap;
	swap(m_hwnd, other.m_hwnd);
}

void export_window::init()
{
	export_window_impl::init();
}

void export_window::deinit()
{
	export_window_impl::deinit();
}

HWND const& export_window::get_hwnd() const
{
	return m_hwnd;
}
