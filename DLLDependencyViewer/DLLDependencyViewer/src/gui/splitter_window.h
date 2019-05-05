#pragma once


#include <windows.h>


enum class splitter_window_orientation
{
	horizontal,
	vertical
};


template<splitter_window_orientation orientation>
class splitter_window
{
public:
	static void register_class();
public:
	splitter_window(HWND const parent);
	splitter_window(splitter_window const&) = delete;
	splitter_window(splitter_window&&) = delete;
	splitter_window& operator=(splitter_window const&) = delete;
	splitter_window& operator=(splitter_window&&) = delete;
	~splitter_window();
public:
	HWND get_hwnd() const;
public:
	void set_elements(HWND const first, HWND const second);
private:
	static LRESULT CALLBACK class_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
private:
	LRESULT on_message(UINT msg, WPARAM wparam, LPARAM lparam);
	LRESULT on_wm_destroy(WPARAM wparam, LPARAM lparam);
	LRESULT on_wm_size(WPARAM wparam, LPARAM lparam);
	LRESULT on_wm_mousemove(WPARAM wparam, LPARAM lparam);
	LRESULT on_wm_lbuttondown(WPARAM wparam, LPARAM lparam);
	LRESULT on_wm_lbuttonup(WPARAM wparam, LPARAM lparam);
private:
	static wchar_t const s_window_class_name[];
	static wchar_t const* const s_cursor_id;
private:
	static ATOM m_s_class;
private:
	HWND m_hwnd;
	HWND m_first;
	HWND m_second;
	float m_position;
	bool m_sizing;
};

typedef splitter_window<splitter_window_orientation::horizontal> splitter_window_hor;
typedef splitter_window<splitter_window_orientation::vertical> splitter_window_ver;
