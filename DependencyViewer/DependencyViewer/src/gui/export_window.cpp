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

void export_window::repaint()
{
	UINT const msg = static_cast<std::uint32_t>(wm::wm_repaint);
	WPARAM const wparam = 0;
	LPARAM const lparam = 0;
	assert(m_hwnd != nullptr);
	[[maybe_unused]] LRESULT res = SendMessageW(m_hwnd, msg, wparam, lparam);
}

bool export_window::translateaccelerator(MSG& message)
{
	bool translated;
	UINT const msg = static_cast<std::uint32_t>(wm::wm_translateaccelerator);
	WPARAM const wparam = reinterpret_cast<WPARAM>(&translated);
	LPARAM const lparam = reinterpret_cast<LPARAM>(&message);
	assert(m_hwnd != nullptr);
	[[maybe_unused]] LRESULT res = SendMessageW(m_hwnd, msg, wparam, lparam);
	return translated;
}

void export_window::setfi(file_info const* const& fi)
{
	UINT const msg = static_cast<std::uint32_t>(wm::wm_setfi);
	WPARAM const wparam = 0;
	LPARAM const lparam = reinterpret_cast<LPARAM>(fi);
	assert(m_hwnd != nullptr);
	[[maybe_unused]] LRESULT res = SendMessageW(m_hwnd, msg, wparam, lparam);
}

void export_window::setundecorate(bool const& undecorate)
{
	UINT const msg = static_cast<std::uint32_t>(wm::wm_setundecorate);
	WPARAM const wparam = undecorate ? 1 : 0;
	LPARAM const lparam = 0;
	assert(m_hwnd != nullptr);
	[[maybe_unused]] LRESULT res = SendMessageW(m_hwnd, msg, wparam, lparam);
}

void export_window::selectitem(std::uint16_t const& item_idx)
{
	UINT const msg = static_cast<std::uint32_t>(wm::wm_selectitem);
	WPARAM const wparam = item_idx;
	LPARAM const lparam = 0;
	assert(m_hwnd != nullptr);
	[[maybe_unused]] LRESULT res = SendMessageW(m_hwnd, msg, wparam, lparam);
}

void export_window::setcmdmatching(cmd_matching_fn_t const& cmd_matching_fn, cmd_matching_ctx_t const& cmd_matching_ctx)
{
	static_assert(sizeof(cmd_matching_fn) == sizeof(WPARAM), "");
	static_assert(sizeof(cmd_matching_ctx) == sizeof(LPARAM), "");
	UINT const msg = static_cast<std::uint32_t>(wm::wm_setcmdmatching);
	WPARAM const wparam = reinterpret_cast<WPARAM>(cmd_matching_fn);
	LPARAM const lparam = reinterpret_cast<LPARAM>(cmd_matching_ctx);
	assert(m_hwnd != nullptr);
	LRESULT const res = SendMessageW(m_hwnd, msg, wparam, lparam);
}

HWND const& export_window::get_hwnd() const
{
	return m_hwnd;
}
