#pragma once


#include "export_window.h"

#include "../nogui/windows_my.h"


class export_window_impl
{
private:
	export_window_impl() noexcept = delete;
	export_window_impl(HWND const& self);
	export_window_impl(export_window_impl const&) = delete;
	export_window_impl(export_window_impl&&) noexcept = delete;
	export_window_impl& operator=(export_window_impl const&) = delete;
	export_window_impl& operator=(export_window_impl&&) noexcept = delete;
	~export_window_impl();
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
private:
	static ATOM g_class;
private:
	HWND const m_self;
};
