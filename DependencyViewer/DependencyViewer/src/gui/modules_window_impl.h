#pragma once


#include "modules_window.h"

#include "../nogui/windows_my.h"


class modules_window_impl
{
private:
	modules_window_impl() noexcept = delete;
	modules_window_impl(HWND const& self);
	modules_window_impl(modules_window_impl const&) = delete;
	modules_window_impl(modules_window_impl&&) noexcept = delete;
	modules_window_impl& operator=(modules_window_impl const&) = delete;
	modules_window_impl& operator=(modules_window_impl&&) noexcept = delete;
	~modules_window_impl();
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
	LRESULT on_wm_notify(WPARAM const& wparam, LPARAM const& lparam);
	LRESULT on_wm_repaint(WPARAM const& wparam, LPARAM const& lparam);
	void on_getdispinfow(NMHDR& nmhdr);
	void repaint();
private:
	static ATOM g_class;
private:
	HWND const m_self;
	HWND m_list_view;
};
