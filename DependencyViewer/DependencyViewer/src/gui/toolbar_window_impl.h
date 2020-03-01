#pragma once


#include "toolbar_window.h"

#include "../nogui/windows_my.h"


class toolbar_window_impl
{
private:
	toolbar_window_impl() noexcept = delete;
	toolbar_window_impl(HWND const& self);
	toolbar_window_impl(toolbar_window_impl const&) = delete;
	toolbar_window_impl(toolbar_window_impl&&) noexcept = delete;
	toolbar_window_impl& operator=(toolbar_window_impl const&) = delete;
	toolbar_window_impl& operator=(toolbar_window_impl&&) noexcept = delete;
	~toolbar_window_impl();
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
	LRESULT on_wm_notify(WPARAM const& wparam, LPARAM const& lparam);
	void on_getinfotipw(NMHDR& nmhdr);
private:
	static ATOM g_class;
private:
	HWND const m_self;
	HWND m_toolbar;
};
