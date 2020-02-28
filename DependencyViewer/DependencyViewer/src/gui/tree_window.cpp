#include "tree_window.h"

#include "main.h"
#include "tree_window_impl.h"

#include "../nogui/cassert_my.h"

#include <utility>


tree_window::tree_window() noexcept :
	m_hwnd()
{
}

tree_window::tree_window(HWND const& parent) :
	tree_window()
{
	assert(parent != nullptr);
	m_hwnd = CreateWindowExW(0, tree_window_impl::get_class_atom(), nullptr, WS_CLIPCHILDREN | WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, parent, nullptr, get_instance(), nullptr);
	assert(m_hwnd != nullptr);
}

tree_window::tree_window(tree_window&& other) noexcept :
	tree_window()
{
	swap(other);
}

tree_window& tree_window::operator=(tree_window&& other) noexcept
{
	swap(other);
	return *this;
}

tree_window::~tree_window()
{
}

void tree_window::swap(tree_window& other) noexcept
{
	using std::swap;
	swap(m_hwnd, other.m_hwnd);
}

void tree_window::init()
{
	tree_window_impl::init();
}

void tree_window::deinit()
{
	tree_window_impl::deinit();
}

HWND const& tree_window::get_hwnd() const
{
	return m_hwnd;
}
