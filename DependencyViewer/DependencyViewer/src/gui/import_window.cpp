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

void import_window::repaint()
{
	UINT const msg = static_cast<std::uint32_t>(wm::wm_repaint);
	WPARAM const wparam = 0;
	LPARAM const lparam = 0;
	assert(m_hwnd != nullptr);
	[[maybe_unused]] LRESULT res = SendMessageW(m_hwnd, msg, wparam, lparam);
}

bool import_window::translateaccelerator(MSG& message)
{
	bool translated;
	UINT const msg = static_cast<std::uint32_t>(wm::wm_translateaccelerator);
	WPARAM const wparam = reinterpret_cast<WPARAM>(&translated);
	LPARAM const lparam = reinterpret_cast<LPARAM>(&message);
	assert(m_hwnd != nullptr);
	[[maybe_unused]] LRESULT res = SendMessageW(m_hwnd, msg, wparam, lparam);
	return translated;
}

void import_window::setfi(file_info const* const& fi)
{
	UINT const msg = static_cast<std::uint32_t>(wm::wm_setfi);
	WPARAM const wparam = 0;
	LPARAM const lparam = reinterpret_cast<LPARAM>(fi);
	assert(m_hwnd != nullptr);
	[[maybe_unused]] LRESULT res = SendMessageW(m_hwnd, msg, wparam, lparam);
}

void import_window::setundecorate(bool const& undecorate)
{
	UINT const msg = static_cast<std::uint32_t>(wm::wm_setundecorate);
	WPARAM const wparam = undecorate ? 1 : 0;
	LPARAM const lparam = 0;
	assert(m_hwnd != nullptr);
	[[maybe_unused]] LRESULT res = SendMessageW(m_hwnd, msg, wparam, lparam);
}

void import_window::selectitem(std::uint16_t const& item_idx)
{
	UINT const msg = static_cast<std::uint32_t>(wm::wm_selectitem);
	WPARAM const wparam = item_idx;
	LPARAM const lparam = 0;
	assert(m_hwnd != nullptr);
	[[maybe_unused]] LRESULT res = SendMessageW(m_hwnd, msg, wparam, lparam);
}

void import_window::setcmdmatching(cmd_matching_fn_t const& cmd_matching_fn, cmd_matching_ctx_t const& cmd_matching_ctx)
{
	static_assert(sizeof(cmd_matching_fn) == sizeof(WPARAM), "");
	static_assert(sizeof(cmd_matching_ctx) == sizeof(LPARAM), "");
	UINT const msg = static_cast<std::uint32_t>(wm::wm_setcmdmatching);
	WPARAM const wparam = reinterpret_cast<WPARAM>(cmd_matching_fn);
	LPARAM const lparam = reinterpret_cast<LPARAM>(cmd_matching_ctx);
	assert(m_hwnd != nullptr);
	LRESULT const res = SendMessageW(m_hwnd, msg, wparam, lparam);
}

HWND const& import_window::get_hwnd() const
{
	return m_hwnd;
}
