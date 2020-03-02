#pragma once


#include "master_window.h"

#include "../nogui/windows_my.h"


class master_window_impl
{
private:
	master_window_impl() noexcept = delete;
	master_window_impl(HWND const& self);
	master_window_impl(master_window_impl const&) = delete;
	master_window_impl(master_window_impl&&) noexcept = delete;
	master_window_impl& operator=(master_window_impl const&) = delete;
	master_window_impl& operator=(master_window_impl&&) noexcept = delete;
	~master_window_impl();
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
	static int g_debug_instances;
private:
	HWND const m_self;
};
