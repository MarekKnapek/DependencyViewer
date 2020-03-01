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

void toolbar_window::setfullpathspressed(bool const& pressed)
{
	UINT const msg = static_cast<std::uint32_t>(wm::wm_setfullpathspressed);
	WPARAM const wparam = pressed ? 1 : 0;
	LPARAM const lparam = 0;
	assert(m_hwnd != nullptr);
	LRESULT const res = SendMessageW(m_hwnd, msg, wparam, lparam);
}

void toolbar_window::setundecoratepressed(bool const& pressed)
{
	UINT const msg = static_cast<std::uint32_t>(wm::wm_setundecoratepressed);
	WPARAM const wparam = pressed ? 1 : 0;
	LPARAM const lparam = 0;
	assert(m_hwnd != nullptr);
	LRESULT const res = SendMessageW(m_hwnd, msg, wparam, lparam);
}

void toolbar_window::setpropertiesavail(bool const& available)
{
	UINT const msg = static_cast<std::uint32_t>(wm::wm_setpropertiesavail);
	WPARAM const wparam = available ? 1 : 0;
	LPARAM const lparam = 0;
	assert(m_hwnd != nullptr);
	LRESULT const res = SendMessageW(m_hwnd, msg, wparam, lparam);
}

void toolbar_window::setcmdopen(cmd_open_fn_t const& cmd_open_fn, cmd_open_ctx_t const& cmd_open_ctx)
{
	static_assert(sizeof(cmd_open_fn) == sizeof(WPARAM), "");
	static_assert(sizeof(cmd_open_ctx) == sizeof(LPARAM), "");
	UINT const msg = static_cast<std::uint32_t>(wm::wm_setcmdopen);
	WPARAM const wparam = reinterpret_cast<WPARAM>(cmd_open_fn);
	LPARAM const lparam = reinterpret_cast<LPARAM>(cmd_open_ctx);
	assert(m_hwnd != nullptr);
	LRESULT const res = SendMessageW(m_hwnd, msg, wparam, lparam);
}

void toolbar_window::setcmdfullpaths(cmd_fullpaths_fn_t const& cmd_fullpaths_fn, cmd_fullpaths_ctx_t const& cmd_fullpaths_ctx)
{
	static_assert(sizeof(cmd_fullpaths_fn) == sizeof(WPARAM), "");
	static_assert(sizeof(cmd_fullpaths_ctx) == sizeof(LPARAM), "");
	UINT const msg = static_cast<std::uint32_t>(wm::wm_setcmdfullpaths);
	WPARAM const wparam = reinterpret_cast<WPARAM>(cmd_fullpaths_fn);
	LPARAM const lparam = reinterpret_cast<LPARAM>(cmd_fullpaths_ctx);
	assert(m_hwnd != nullptr);
	LRESULT const res = SendMessageW(m_hwnd, msg, wparam, lparam);
}

void toolbar_window::setcmdundecorate(cmd_undecorate_fn_t const& cmd_undecorate_fn, cmd_undecorate_ctx_t const& cmd_undecorate_ctx)
{
	static_assert(sizeof(cmd_undecorate_fn) == sizeof(WPARAM), "");
	static_assert(sizeof(cmd_undecorate_ctx) == sizeof(LPARAM), "");
	UINT const msg = static_cast<std::uint32_t>(wm::wm_setcmdundecorate);
	WPARAM const wparam = reinterpret_cast<WPARAM>(cmd_undecorate_fn);
	LPARAM const lparam = reinterpret_cast<LPARAM>(cmd_undecorate_ctx);
	assert(m_hwnd != nullptr);
	LRESULT const res = SendMessageW(m_hwnd, msg, wparam, lparam);
}

void toolbar_window::setcmdproperties(cmd_properties_fn_t const& cmd_properties_fn, cmd_properties_ctx_t const& cmd_properties_ctx)
{
	static_assert(sizeof(cmd_properties_fn) == sizeof(WPARAM), "");
	static_assert(sizeof(cmd_properties_ctx) == sizeof(LPARAM), "");
	UINT const msg = static_cast<std::uint32_t>(wm::wm_setcmdproperties);
	WPARAM const wparam = reinterpret_cast<WPARAM>(cmd_properties_fn);
	LPARAM const lparam = reinterpret_cast<LPARAM>(cmd_properties_ctx);
	assert(m_hwnd != nullptr);
	LRESULT const res = SendMessageW(m_hwnd, msg, wparam, lparam);
}

HWND const& toolbar_window::get_hwnd() const
{
	return m_hwnd;
}
