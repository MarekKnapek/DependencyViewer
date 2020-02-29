#pragma once


#include "splitter_window2.h"

#include "../nogui/windows_my.h"


template<splitter_window2_orientation orientation>
class splitter_window2_impl
{
private:
	splitter_window2_impl() noexcept = delete;
	splitter_window2_impl(HWND const& self);
	splitter_window2_impl(splitter_window2_impl const&) = delete;
	splitter_window2_impl(splitter_window2_impl&&) noexcept = delete;
	splitter_window2_impl& operator=(splitter_window2_impl const&) = delete;
	splitter_window2_impl& operator=(splitter_window2_impl&&) noexcept = delete;
	~splitter_window2_impl();
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
	LRESULT on_wm_setchildren(WPARAM const& wparam, LPARAM const& lparam);
private:
	static wchar_t const* const s_class_name;
	static wchar_t const* const s_cursor_id;
private:
	static ATOM g_class;
private:
	HWND const m_self;
	HWND m_child_a;
	HWND m_child_b;
	float m_position;
};
