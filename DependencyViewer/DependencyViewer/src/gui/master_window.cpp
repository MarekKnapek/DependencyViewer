#include "master_window.h"

#include "main.h"
#include "master_window_impl.h"

#include "../nogui/cassert_my.h"

#include <utility>


#if defined _M_IX86
static constexpr wchar_t const s_master_window_title[] = L"DependencyViewer (x86)";
#elif defined _M_X64
static constexpr wchar_t const s_master_window_title[] = L"DependencyViewer (x64)";
#endif


master_window::master_window() noexcept :
	m_hwnd()
{
}

master_window::master_window(std::nullptr_t const&) :
	master_window()
{
	m_hwnd = CreateWindowExW(0, master_window_impl::get_class_atom(), s_master_window_title, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr, get_instance(), nullptr);
	assert(m_hwnd != nullptr);
}

master_window::master_window(master_window&& other) noexcept :
	master_window()
{
	swap(other);
}

master_window& master_window::operator=(master_window&& other) noexcept
{
	swap(other);
	return *this;
}

master_window::~master_window()
{
}

void master_window::swap(master_window& other) noexcept
{
	using std::swap;
	swap(m_hwnd, other.m_hwnd);
}

void master_window::init()
{
	master_window_impl::init();
}

void master_window::deinit()
{
	master_window_impl::deinit();
}

HWND const& master_window::get_hwnd() const
{
	return m_hwnd;
}
