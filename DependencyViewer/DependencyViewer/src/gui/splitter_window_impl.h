#pragma once


#include "splitter_window.h"

#include "../nogui/windows_my.h"


template<splitter_window_orientation orientation>
class splitter_window_impl
{
private:
	splitter_window_impl() noexcept = delete;
	splitter_window_impl(HWND const& self);
	splitter_window_impl(splitter_window_impl const&) = delete;
	splitter_window_impl(splitter_window_impl&&) noexcept = delete;
	splitter_window_impl& operator=(splitter_window_impl const&) = delete;
	splitter_window_impl& operator=(splitter_window_impl&&) noexcept = delete;
	~splitter_window_impl();
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
	LRESULT on_wm_mousemove(WPARAM const& wparam, LPARAM const& lparam);
	LRESULT on_wm_lbuttondown(WPARAM const& wparam, LPARAM const& lparam);
	LRESULT on_wm_lbuttonup(WPARAM const& wparam, LPARAM const& lparam);
	LRESULT on_wm_setchildren(WPARAM const& wparam, LPARAM const& lparam);
	LRESULT on_wm_setposition(WPARAM const& wparam, LPARAM const& lparam);
	void refresh_children();
private:
	static wchar_t const* const s_class_name;
	static wchar_t const* const s_cursor_id;
	static int g_debug_instances;
private:
	static ATOM g_class;
private:
	HWND const m_self;
	HWND m_child_a;
	HWND m_child_b;
	float m_position;
	bool m_sizing;
};
