#pragma once


#include "tree_window.h"

#include "../nogui/windows_my.h"


class tree_window_impl
{
private:
	tree_window_impl() noexcept = delete;
	tree_window_impl(HWND const& self);
	tree_window_impl(tree_window_impl const&) = delete;
	tree_window_impl(tree_window_impl&&) noexcept = delete;
	tree_window_impl& operator=(tree_window_impl const&) = delete;
	tree_window_impl& operator=(tree_window_impl&&) noexcept = delete;
	~tree_window_impl();
public:
	static void init();
	static void deinit();
	static wchar_t const* get_class_atom();
private:
	static void register_class();
	static void unregister_class();
	static LRESULT CALLBACK class_proc(HWND const hwnd, UINT const msg, WPARAM const wparam, LPARAM const lparam);
	static LRESULT on_wm_create(HWND const& hwnd, WPARAM const& wparam, LPARAM const& lparam);
	static LRESULT on_wm_destroy(HWND const& hwnd, WPARAM const& wparam, LPARAM const& lparam);
	LRESULT on_message(UINT const& msg, WPARAM const& wparam, LPARAM const& lparam);
	LRESULT on_wm_size(WPARAM const& wparam, LPARAM const& lparam);
	LRESULT on_wm_repaint(WPARAM const& wparam, LPARAM const& lparam);
	void repaint();
private:
	static ATOM g_class;
private:
	HWND const m_self;
	HWND m_tree_view;
};
