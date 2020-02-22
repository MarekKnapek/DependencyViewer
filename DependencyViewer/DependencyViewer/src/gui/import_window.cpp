#include "import_window.h"

#include "main.h"
#include "import_window_impl.h"

#include "../nogui/cassert_my.h"

#include <utility>


import_window::import_window() noexcept :
	m_hwnd()
{
}

import_window::import_window(HWND const& parent) :
	import_window()
{
	assert(parent != nullptr);
	m_hwnd = CreateWindowExW(0, import_window_impl::get_class_atom(), nullptr, WS_CLIPCHILDREN | WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, parent, nullptr, get_instance(), nullptr);
	assert(m_hwnd != nullptr);
}

import_window::import_window(import_window&& other) noexcept :
	import_window()
{
	swap(other);
}

import_window& import_window::operator=(import_window&& other) noexcept
{
	swap(other);
	return *this;
}

import_window::~import_window()
{
}

void import_window::swap(import_window& other) noexcept
{
	using std::swap;
	swap(m_hwnd, other.m_hwnd);
}

void import_window::init()
{
	import_window_impl::init();
}

void import_window::deinit()
{
	import_window_impl::deinit();
}

void import_window::setfi(file_info const* const& fi)
{
	assert(m_hwnd != nullptr);
	UINT const msg = static_cast<std::uint32_t>(wm::wm_setfi);
	WPARAM const wparam = 0;
	LPARAM const lparam = reinterpret_cast<LPARAM>(fi);
	[[maybe_unused]] LRESULT res = SendMessageW(m_hwnd, msg, wparam, lparam);
}

void import_window::setundecorate(bool const& undecorate)
{
	assert(m_hwnd != nullptr);
	UINT const msg = static_cast<std::uint32_t>(wm::wm_setundecorate);
	WPARAM const wparam = undecorate ? 1 : 0;
	LPARAM const lparam = 0;
	[[maybe_unused]] LRESULT res = SendMessageW(m_hwnd, msg, wparam, lparam);
}

HWND const& import_window::get_hwnd() const
{
	return m_hwnd;
}
