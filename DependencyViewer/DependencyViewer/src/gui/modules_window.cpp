#include "modules_window.h"

#include "main.h"
#include "modules_window_impl.h"

#include "../nogui/cassert_my.h"

#include <utility>


modules_window::modules_window() noexcept :
	m_hwnd()
{
}

modules_window::modules_window(HWND const& parent) :
	modules_window()
{
	assert(parent != nullptr);
	m_hwnd = CreateWindowExW(0, modules_window_impl::get_class_atom(), nullptr, WS_CLIPCHILDREN | WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, parent, nullptr, get_instance(), nullptr);
	assert(m_hwnd != nullptr);
}

modules_window::modules_window(modules_window&& other) noexcept :
	modules_window()
{
	swap(other);
}

modules_window& modules_window::operator=(modules_window&& other) noexcept
{
	swap(other);
	return *this;
}

modules_window::~modules_window()
{
}

void modules_window::swap(modules_window& other) noexcept
{
	using std::swap;
	swap(m_hwnd, other.m_hwnd);
}

void modules_window::init()
{
	modules_window_impl::init();
}

void modules_window::deinit()
{
	modules_window_impl::deinit();
}

void modules_window::repaint()
{
	UINT const msg = static_cast<std::uint32_t>(wm::wm_repaint);
	WPARAM const wparam = 0;
	LPARAM const lparam = 0;
	assert(m_hwnd != nullptr);
	[[maybe_unused]] LRESULT res = SendMessageW(m_hwnd, msg, wparam, lparam);
}

bool modules_window::translateaccelerator(MSG& message)
{
	bool translated;
	UINT const msg = static_cast<std::uint32_t>(wm::wm_translateaccelerator);
	WPARAM const wparam = reinterpret_cast<WPARAM>(&translated);
	LPARAM const lparam = reinterpret_cast<LPARAM>(&message);
	assert(m_hwnd != nullptr);
	[[maybe_unused]] LRESULT res = SendMessageW(m_hwnd, msg, wparam, lparam);
	return translated;
}

void modules_window::setmodlist(modules_list_t const& ml)
{
	UINT const msg = static_cast<std::uint32_t>(wm::wm_setmodlist);
	WPARAM const wparam = 0;
	LPARAM const lparam = reinterpret_cast<LPARAM>(&ml);
	assert(m_hwnd != nullptr);
	[[maybe_unused]] LRESULT res = SendMessageW(m_hwnd, msg, wparam, lparam);
}

void modules_window::setcmdmatching(cmd_matching_fn_t const& cmd_matching_fn, cmd_matching_ctx_t const& cmd_matching_ctx)
{
	static_assert(sizeof(cmd_matching_fn) == sizeof(WPARAM), "");
	static_assert(sizeof(cmd_matching_ctx) == sizeof(LPARAM), "");
	UINT const msg = static_cast<std::uint32_t>(wm::wm_setcmdmatching);
	WPARAM const wparam = reinterpret_cast<WPARAM>(cmd_matching_fn);
	LPARAM const lparam = reinterpret_cast<LPARAM>(cmd_matching_ctx);
	assert(m_hwnd != nullptr);
	LRESULT const res = SendMessageW(m_hwnd, msg, wparam, lparam);
}

HWND const& modules_window::get_hwnd() const
{
	return m_hwnd;
}
